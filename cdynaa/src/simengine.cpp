/*
file: simengine.cpp
*/
#include <string>
#include <unordered_map>
//#include "../include/entity.h"
#include "../include/dynaatypes.h"
#include "../include/simengine.h"
#include "../include/coreexceptions.h"

using namespace std;
using namespace dynaa;


void SimulationEngine::init() {
    simulationTime = 0;
    currentState = Idle;
    scheduler.reset();
    hRegistry.reset();
    // Diagnostic counters
    //m_countInstants = 0;
    m_countEvents = 0;
    m_countHandlerCalls = 0;
}

void SimulationEngine::run()  {
    if (currentState == Stopped) {
        throw SimulationStoppedException();
    }

    if (currentState == Running) {
        return;
    }

    //SimulationEngine.coreLogger.info("Starting simulation run from " + this.currentSimulationTime);
    currentState = Running;
    while ((scheduler.getNextInstant() != SchedulerInspectorInterface::NO_NEXT_INSTANT)
            && (currentState == Running)) {
        runStep();
    }

    // Does not update state and time if simulation stopped
    if (currentState == Running) {
        currentState = Idle;
    }
}

SchedulerUserInterface& SimulationEngine::getScheduler() {
    return scheduler;
}

SchedulerInspectorInterface& SimulationEngine::getEventInspector() {
    return scheduler;
}

HandlerRegistrationInterface& SimulationEngine::getHandlerRegistry() {
    return hRegistry;
}

HandlerQueryInterface& SimulationEngine::getHandlerInspector() {
    return hRegistry;
}

void SimulationEngine::stop() {
    currentState = Stopped;
}

/**
 * Run the simulation until the specified point in time
 *
 * @param endTime
 *            a double denoting the point in time at which the simulation
 *            will stop
 * @throws SimulationStoppedException
 * @see #run()
 */
void SimulationEngine::run(const SimTime endTime) {
    if (currentState == Stopped) {
        throw SimulationStoppedException();
    }

    if (currentState == Running) {
        return;
    }

    if (endTime < simulationTime) {
        return;
    }

    currentState = Running;
    while ((scheduler.getNextInstant() != SchedulerInspectorInterface::NO_NEXT_INSTANT)
            && (simulationTime < endTime) && (currentState == Running)) {
        runStep();
    }

    // Does not update state and time if simulation stopped
    if (currentState == Running) {
        simulationTime = endTime;
        currentState = Idle;
    }
}

/**
 * Proceed one step in the simulation. This involves fetching the events
 * that occur at the current point in time, and calling all corresponding
 * handlers for the events. After this the simulation time is updated to the
 * next point in time at which there are events. Note that this might be the
 * same point in time if any of the handlers schedule a new event at the
 * current time point.
 */
void SimulationEngine::runStep() {
    EventSet currentEvents = scheduler.removeEventsAt(simulationTime);
    while (!currentEvents.empty()) {
        Event event = *currentEvents.begin();
        currentEvents.erase(currentEvents.begin());
        m_countEvents += 1;
        for (const RegisteredHandler<EventHandler>& obs : hRegistry.getEventHandlersOf(event)) {
            try {
                event.source();
                obs.handler->handle(event);
                // TODO differentiate calls of event expressions
                m_countHandlerCalls += 1;
            } catch (InvalidEventSourceException & e) {
                // Does not invoke the observer
                cout << "Warning!  Tried to trigger an event whose source was invalid(already de-allocated). Ignoring the event." <<
                     endl; // TODO: replace cout by an entry in logging engine.
            }
            // At any point in the simulation the simulation can be
            // STOPPED. This has to cause a hard break
            if (currentState == Stopped) {
                return;
            }
        }
    }
    // Does not update the simulation time if simulation stopped
    const SimTime nextInstant = scheduler.getNextInstant();
    simulationTime = (nextInstant == SchedulerInspectorInterface::NO_NEXT_INSTANT
                      ?  simulationTime : nextInstant);
}

void SimulationEngine::reset() {
    if (currentState == Running) {
        throw SimulationInterruptedException();
    }
    init();
}

SimState SimulationEngine::getState() const {
    return currentState;
};

void SimulationEngine::unschedule(const Event event) {
    scheduler.unschedule(event);
}

std::unordered_map<std::string, long> SimulationEngine::getDiagnosticInfo() const {
    std::unordered_map<std::string, long> info = {
        {"sim_time", simulationTime},
        {"num_current_instants", scheduler.getNumberOfInstants()},
        {"num_current_events", scheduler.getNumberOfEvents()},
        // TODO differentiate expression handlers?
        {"num_reg_handlers", hRegistry.getNumberOfHandlers()},
        {"num_past_events", m_countEvents},
        {"num_handler_calls", m_countHandlerCalls}
    };
    return info;
};

SimTime SimulationEngine::getCurrentTime() const {
    return simulationTime;
}

SimulationEngine& SimulationEngine::getInstance() {
    static SimulationEngine singletonSimulationEngine;
    return singletonSimulationEngine;
}
