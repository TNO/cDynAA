// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_SIMENGINE_
#define INCLUDED_SIMENGINE_
/* Definition of the Event class
 */

#include <string>
#include <vector>
#include <iostream>
#include "defaultscheduler.h"
#include "defaulthandlerregistry.h"
#include "diagnostics.h"
#include "diagnosticsinterface.h"


namespace dynaa {

enum SimState {
    Idle, Running, Stopped
};

class SimulationEngine {

private:
    /**
     * The scheduler used in this simulation engine
     */
    DefaultScheduler scheduler;

    /**
     * The event handler registry used in this simulation engine
     */
    DefaultHandlerRegistry hRegistry;

    /**
     * Holds the master simulation time
     */
    SimTime simulationTime;

    /**
     * Holds the current simulation engine state
     */
    SimState currentState;


    /**
     * The diagnostics module used in this simulation engine
     */
    Diagnostics& diagnostics;

    /**
     * Construct a new simulation engine. This function is private because it is
     * a singleton class. If any other class wants an instantiation of the
     * class, the function getInstance() should be used.
     *
     * @see #getInstance()
     */
    SimulationEngine(): scheduler(DefaultScheduler()),
        hRegistry(DefaultHandlerRegistry()),
        simulationTime(0),
        currentState(Idle),
        diagnostics(Diagnostics::getInstance()) {}

    /**
     * Deletes and blocks the copy constructor and the assignment operator!
     *
     * See design discussion !  This is an important block !
     */
    SimulationEngine(const SimulationEngine &other) = delete;
    SimulationEngine& operator=(const SimulationEngine &other) = delete;

    /**
     * Initializes this simulation engine.
     *
     * Active at the beginning and on resets
     */
    void init();

    /**
    * Proceed one instant in the simulation. This means executing all the
    * events in the same instant including any events scheduled after the
    * stepInstant() function is called, for example by using the scheduleNow()
    * function. Thus, this function can potentially have an infinite loop.
    */
    void stepInstant();

    /**
    * Proceed one event in the simulation. Returns the time of that event.
    */
    void stepEvent(Event event);

    /**
    * Updates the simulation time to the nearest instant where an event is scheduled.
    * If there are no more events in the timeline, it keeps the current value.
    */
    void updateSimulationTime();


public:

    /**
     * Retrieves the current time of this
     * @return a double specifying the current simulation time. The unit of time
     *         is note defined explicitly
     */
    SimTime getCurrentTime() const;

    /**
     * The Simulation Engine is a singleton class, so internally an instance is
     * kept to the only instance that will be created. Using this function this
     * instance can be obtained.
     *
     * @return an instance of the singleton simulation engine
     */
    static SimulationEngine& getInstance();

    /**
     * Run the Simulation until no more events are scheduled
     *
     * @throws SimulationStoppedException
     * @see #run(double)
     */
    void run();

    // /**
    //  * Pause the simulation
    //  * The simulation will stop running after the current instant or event is
    //  * executed, depending on how the execution was called. If the simulation is
    //  * called using run(), run(double), or runInstant(double), it will be the
    //  * current instant. Conversely, if the simulation was called using runEvent(double)
    //  * it it will be the current event.
    //  * After that the simulation can resume to the next instant by making use of
    //  * functions like run(), run(double), runInstant(double), or runEvent(double)
    //  */
    // void pause();

    /**
    * Run the simulation until, but not including, the specified point in time.
    *
    * The instant indicated by endTime is not included in the simulation.  That
    * means events schedule at endTime will not be issued.
    *
    * @param endTime
    *            a double denoting the point in time at which the simulation
    *            will stop
    * @throws SimulationStoppedException
    * @see #run()
    */
    void run(const SimTime endTime);

    /**
    * Run the simulation for the next numberOfInstants instants.
    *
    * @param numberOfInstants
    *            an int denoting the number of instants
    * @throws SimulationStoppedException
    * @see #run()
    */
    void runInstant(unsigned numberOfInstants);

    /**
    * Run the simulation for the next numberOfEvents events.
    *
    * @param numberOfEvents
    *            an int denoting the number of events
    * @throws SimulationStoppedException
    * @see #run()
    */
    void runEvent(unsigned numberOfEvents);

    /**
    * Provides the access to the scheduler of this simulation engine.
    *
    * @return the SchedulerUserInterface used by the simulation engine
    */
    SchedulerUserInterface& getScheduler();

    /**
    * Provides the access to an interface to inspect  scheduler of this simulation engine.
    *
    * @return the SchedulerInspectorInterface used by the simulation engine
    */
    SchedulerInspectorInterface& getEventInspector();

    /**
     * Provides access to the handler registry of this simulation engine.
     *
     * This object is used to register and unregister handlers.
     *
     * @return the HandlerRegistrationInterface used by the simulation engine.
     */
    HandlerRegistrationInterface& getHandlerRegistry();

    /**
     * Provides access to an interface to query handlers of this simulation engine.
     *
     * @return the HandlerQueryInterface used by the simulation engine.
     */
    HandlerQueryInterface& getHandlerInspector();

    /**
     * Provides access to an interface to access diagnostics information
     *
     * @return the DiagnosticsInterface used by the simulation engine.
     */
    DiagnosticsInterface& getDiagnostics();

    /**
    * Stops the current simulation by setting the current state to STOPPED.
    * This will cause all threads in which a {@link #run()} or a
    * {@link #stepInstant()} is running to end.
    *
    * This causes the simulation engine to get in a state after which a
    * {@link #reset()} has to be called in order to get out of.
    */
    void stop();

    /**
     * Resets the simulator into it's original state
     */
    void reset();

    /**
     * Gets the state of this simulator
     */
    SimState getState() const;

    /**
     * Unschedules the given event from the simulation timeline.
     *
     * @param event
     *          the event to be unscheduled
     */
    void unschedule(const Event &event);

};

typedef SimulationEngine DynAASim;

}
#endif

/**
 * Design discussions :
 *
 * 1) Singleton Design 1: Singleton pattern.
 * By design, DynAA has a singleton simulation engine.
 * Such fact allows overall access to the engine (e.g. by entities to schedule events,
 * or e.g. by the user to control the simulation) but with the guarantee that everybody
 * is referring to the same engine.
 * Not using a singleton would require a lot of discipline of the user to not create
 * accidentally (e.g. copy, de-reference, return value, etc.) another engine.
 *
 * 2) Singleton Design 2: Deleted copy constructor and assignment operator.
 * If a copy constructor is  allowed (or the assignment operator), it would be possible to make a copy of the
 * simulation engine or copy assign it to another variable.  This violates the singleton.
 * The real danger is that such assignment operation can be a simple accident, and very difficult
 * to detect.  Consider the following situation:
 *
 * // User wants to use a local variable for holding the simulation engine
 * SimulationEngine simEngine = SimulationEngine::getInstance();
 * simEngine.run(20);
 * a = simEngine.getCurrentTime(); // a = 20
 * b = SimulationEngine::getInstance().getCurrentTime(); //  b = 0 !!!!!!! Cannot be true if we expect a singleton !!
 *
 * The reason for this problem is in the line :
 * SimulationEngine simEngine = SimulationEngine::getInstance();  // It goes wrong here !
 *
 * The getInstance method returns the correct value (a reference to the simulation engine),  but the assignment would
 * create a copy when assigning it to simEngine.  This error is very tricky -- missing only by one symbol.  The correct
 * answer would be:
 * SimulationEngine &simEngine = SimulationEngine::getInstance();
 * For this reason, we explicitly block the assignment and copy constructor.
 */
