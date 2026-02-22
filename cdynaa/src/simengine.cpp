/*
file: simengine.cpp
*/
#include <string>
#include <unordered_map>
#include <iostream>
#include <unordered_set>
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
    diagnostics.reset();
}

void SimulationEngine::run()  {
    if (currentState == Stopped) {
        throw SimulationStoppedException();
    }

    if (currentState == Running) {
        return;
    }

    currentState = Running;
    while ((scheduler.getNextEventTime() != SchedulerInspectorInterface::NO_NEXT_EVENT)
            && (currentState == Running)) {
        stepInstant();
    }

    // Does not update state if simulation stopped
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

//TODO: HandlerQueryInterface must be better structured
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
    SimTime next_event_time = scheduler.getNextEventTime();
    while ((next_event_time  != SchedulerInspectorInterface::NO_NEXT_EVENT)
            && (simulationTime < endTime)
            && (next_event_time  < endTime)
            && (currentState == Running)) {
        stepInstant();
        next_event_time = scheduler.getNextEventTime();
    }


    // Does not update state and time if simulation stopped
    if (currentState == Running) {
        simulationTime = endTime;
        currentState = Idle;
    }
}

void SimulationEngine::runInstant(unsigned numberOfInstants) {
    if (currentState == Stopped) {
        throw SimulationStoppedException();
    }

    if (currentState == Running) {
        return;
    }

    currentState = Running;

    while ((scheduler.getNextEventTime() != SchedulerInspectorInterface::NO_NEXT_EVENT)
            && numberOfInstants--
            && (currentState == Running)) {
        stepInstant();
    }

    // Does not update state and time if simulation stopped
    if (currentState == Running) {
        currentState = Idle;
    }
}

void SimulationEngine::runEvent(unsigned numberOfEvents) {
    if (currentState == Stopped) {
        throw SimulationStoppedException();
    }

    if (currentState == Running) {
        return;
    }

    currentState = Running;
    while (numberOfEvents--
            && (currentState == Running)
            && (scheduler.getNextEventTime() != SchedulerInspectorInterface::NO_NEXT_EVENT)) {
        stepEvent(scheduler.popNextEvent());
        if (currentState == Stopped) {
            return;
        } else {
            updateSimulationTime();
        }
    }
    // Does not update state and time if simulation stopped
    currentState = Idle;
}

// The pause feature is currently not supported. Implementation started but needs test 
// and debugging. For now, the method is commented out to avoid confusion, but kept in the codebase to avoid losing the work done on it.
// void SimulationEngine::pause() {
//     currentState = Idle;
// }

/**
 * Proceed one step in the simulation. This involves fetching the events
 * that occur at the current point in time, and calling all corresponding
 * handlers for the events. After this the simulation time is updated to the
 * next point in time at which there are events. Note that this might be the
 * same point in time if any of the handlers schedule a new event at the
 * current time point.
 */
void SimulationEngine::stepInstant() {
    updateSimulationTime(); // Update simulation time to the actual event, it is
    // necessary when new events are scheduled between run
    // commands, or between the initialization of the engine
    // and the first run command. It is not necessary when
    // simulating single events.
    bool sameInstant = true;
    while (sameInstant && currentState==Running) {
        stepEvent(scheduler.popNextEvent());
        sameInstant = (simulationTime == scheduler.getNextEventTime());
    }
    if (currentState != Stopped) {
        updateSimulationTime();
    }
}

void SimulationEngine::stepEvent(Event event) {
    diagnostics.incrementEventsCounter();
    for (const HandlerToken obs : hRegistry.getEventHandlersOf(event)) {
        try {
            event.source();
            bool handler_fired_on_update = hRegistry.updateHandler(obs, event, getCurrentTime());
            if (handler_fired_on_update) {
                hRegistry.activateHandler(obs, event);
                diagnostics.incrementHandlersCounter();
            }
        } catch (InvalidEventSourceException & e) {
            // Does not invoke the observer
            cout << "Warning!  Tried to trigger an event whose source was invalid (already de-allocated). Ignoring the event." <<
                 endl; // TODO: replace cout by an entry in logging engine.
        }
    }
    Diagnostics::getInstance().addDiagnosticsRecord(event);
    return;
}

void SimulationEngine::updateSimulationTime() {
    const SimTime schedulerTime = scheduler.getNextEventTime();
    simulationTime = (schedulerTime == SchedulerInspectorInterface::NO_NEXT_EVENT
                      ?  simulationTime : schedulerTime);
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

void SimulationEngine::unschedule(const Event& event) {
    scheduler.unschedule(event);
}

SimTime SimulationEngine::getCurrentTime() const {
    return simulationTime;
}

SimulationEngine& SimulationEngine::getInstance() {
    static SimulationEngine singletonSimulationEngine;
    return singletonSimulationEngine;
}

DiagnosticsInterface& SimulationEngine::getDiagnostics() {
    return diagnostics;
}