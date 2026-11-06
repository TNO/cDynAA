#include "catch.h"
#include "../include/simengine.h"
#include "entitytest.h"
#include "simenginetest.h"

#include <string>
#include <iostream>
#include <functional>

using namespace std;

TEST_CASE( "Checks the functionality of the simulation engine at large simulation times", 
            "[simengine][stable]" ) {
    // Setup the simulation engine, some event handlers and some events.
    DynAASim &simulatorRef1 = DynAASim::getInstance();

    // Should not throw because simulation is not running
    REQUIRE_NOTHROW(simulatorRef1.reset());

    // Schedules first event at a huge number
    shared_ptr<EntityTest> entity = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType::create("TYPE_A", "Description A");
    Event event = entity->scheduleAt(1e12, eventTypeA);

    // Setup event handlers
    shared_ptr<EventHandler> eventHandler1 = EventHandler::create(a_callback_function);
    entity->wait(eventHandler1, entity);

    SECTION ( "Check we can run until large simulation times" ) {
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        simulatorRef1.run(1e10);
        REQUIRE( simulatorRef1.getCurrentTime() == 1e10 );
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
        simulatorRef1.run(1e25); 
        REQUIRE( simulatorRef1.getCurrentTime() == 1e25 );
        simulatorRef1.run();
        REQUIRE( simulatorRef1.getCurrentTime() == 1e25 );
    }

    SECTION ( "Operating schedule after at large simulation times" ) {
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        simulatorRef1.run(1e20);
        REQUIRE( simulatorRef1.getCurrentTime() == 1e20 );
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
        // Cannot capture, but next line should issue a message in cerr
        entity->scheduleAfter(1e2, eventTypeA);
        simulatorRef1.run(1e25); 
        REQUIRE( simulatorRef1.getCurrentTime() == 1e25 );
        simulatorRef1.run();
        REQUIRE( simulatorRef1.getCurrentTime() == 1e25 );
    }

} 

TEST_CASE( "Checks the functionality of the simulation engine", "[simengine][stable]" ) {
    // Setup the simulation engine, some event handlers and some events.
    DynAASim &simulatorRef1 = DynAASim::getInstance();
    DynAASim &simulatorRef2 = DynAASim::getInstance();

    REQUIRE_NOTHROW(simulatorRef1.reset()); // Should not throw because simulation is not running

    shared_ptr<EntityTest> entity = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType::create("TYPE_A", "Description A");
    Event event = entity->scheduleAt(10, eventTypeA);
    entity->scheduleAt(20, eventTypeA);
    auto diagInfo = simulatorRef2.getDiagnosticInfo();
    REQUIRE( diagInfo["num_current_events"] == 2);
    REQUIRE( diagInfo["num_current_instants"] == 2);

    // Setup event handlers
    shared_ptr<EventHandler> eventHandler1 = EventHandler::create(a_callback_function);
    shared_ptr<EventHandler> eventHandler2 = EventHandler::create(SIMEventHandlerTest::a_callback_static_method,
            "callback_static_method");
    shared_ptr<SIMEventHandlerTest> eventHandlerTest = make_shared<SIMEventHandlerTest>();
    shared_ptr<EventHandler> eventHandler3 = EventHandler::create(std::bind(&SIMEventHandlerTest::callback_method,
            eventHandlerTest.get(), std::placeholders::_1), {eventHandlerTest});

    entity->wait(eventHandler1, entity);
    entity->waitOnce(eventHandler2, event);
    entity->wait(eventHandler3, eventTypeA);

    diagInfo = simulatorRef2.getDiagnosticInfo();
    REQUIRE( diagInfo["num_reg_handlers"] == 3);

    SECTION ( "Check singleton" ) {
        REQUIRE( &simulatorRef1 == &simulatorRef2 );
        REQUIRE( &simulatorRef1 == &DynAASim::getInstance() );
    }

    SECTION ( "Check current time and state") {
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        simulatorRef1.run(5);
        REQUIRE( simulatorRef1.getCurrentTime() == 5 );
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
        simulatorRef1.run(2); // Should not be possible to run backwards in time.
        REQUIRE( simulatorRef1.getCurrentTime() == 5 );
        simulatorRef1.run(12);
        REQUIRE( simulatorRef1.getCurrentTime() == 12 );
        simulatorRef1.run();
        REQUIRE( simulatorRef1.getCurrentTime() == 20);
        //REQUIRE( simulatorRef2.getNumberOfEvents() == 0);
        REQUIRE( simulatorRef2.getDiagnosticInfo()["num_current_events"] == 0);
        simulatorRef1.stop();
        REQUIRE( simulatorRef1.getCurrentTime() == 20);
        REQUIRE( simulatorRef1.getState() == SimState::Stopped);
        REQUIRE_THROWS( simulatorRef1.run() );
        simulatorRef1.reset();
        REQUIRE( simulatorRef1.getState() == SimState::Idle);
        REQUIRE( simulatorRef1.getCurrentTime() == 0);
        simulatorRef1.run();
        // No more events in the queue.
        //REQUIRE( simulatorRef2.getNumberOfEvents() == 0);
        REQUIRE( simulatorRef2.getDiagnosticInfo()["num_current_events"] == 0);
        REQUIRE( simulatorRef1.getCurrentTime() == 0);
    }

    SECTION ( "No handler is called if source of event is deleted before the event is issued.") {
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        simulatorRef1.run(5);
        REQUIRE( simulatorRef1.getCurrentTime() == 5 );
        REQUIRE( eventHandlerTest->callbackMethodCounter == 0);
        entity.reset(); // Destroys the entity, which is source of eventA in time 10.
        REQUIRE_NOTHROW(simulatorRef1.run(12));
        REQUIRE( simulatorRef1.getCurrentTime() == 12 );
        REQUIRE( eventHandlerTest->callbackMethodCounter ==
                 0); // An is issued in 10, which increments this counter, but should not be handled.
    }

    SECTION ( "Pure simulation in the core does not accumulate owner for entities") {
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        simulatorRef1.run(25);
        REQUIRE( entity.use_count() == 1);
    }

    SECTION ( "Checks that events are being issued and watched." ) {
        a_callbackFunctionCounter = 0;
        SIMEventHandlerTest::a_callbackStaticCounter = 0;
        simulatorRef2.run(12);
        REQUIRE( a_callbackFunctionCounter == 1);
        REQUIRE( SIMEventHandlerTest::a_callbackStaticCounter == 1);
        REQUIRE( eventHandlerTest->callbackMethodCounter == 1);
        simulatorRef2.run();
        REQUIRE( a_callbackFunctionCounter == 2);
        REQUIRE( SIMEventHandlerTest::a_callbackStaticCounter == 1);
        REQUIRE( eventHandlerTest->callbackMethodCounter == 2);
        REQUIRE( simulatorRef1.getCurrentTime() == 20);
    }

    SECTION ( "Check if schedulingAt a past time fails with exception.") {
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        simulatorRef1.run(5);
        REQUIRE( simulatorRef1.getCurrentTime() == 5 );
        REQUIRE_THROWS_AS( entity->scheduleAt(2, eventTypeA), InvalidTimePointException &);
    }

    SECTION( "Unscheduling events should work for events inserted now, at, or after" ) {
        Event eventNow = entity->scheduleNow(eventTypeA);
        REQUIRE_NOTHROW(SimulationEngine::getInstance().unschedule(eventNow));
        Event eventAt5 = entity->scheduleAt(5, eventTypeA);
        REQUIRE_NOTHROW(SimulationEngine::getInstance().unschedule(eventAt5));
        Event eventAfter10 = entity->scheduleAfter(10, eventTypeA);
        REQUIRE_NOTHROW(SimulationEngine::getInstance().unschedule(eventAfter10));
    }

    SECTION( "Unscheduling events should silently do nothing if events where removed previously" ) {
        // Allows unscheduling the same event many times (different actors for example).
        // No problem with this behavior, and should not break code.
        Event eventNow = entity->scheduleNow(eventTypeA);
        REQUIRE_NOTHROW(SimulationEngine::getInstance().unschedule(eventNow));
        REQUIRE_NOTHROW(SimulationEngine::getInstance().unschedule(eventNow));
        REQUIRE_NOTHROW(SimulationEngine::getInstance().unschedule(eventNow));
    }

    SECTION ( "Checks that events in the past can be unscheduled as well (silently does nothing)" ) {
        // Nothing really is affected, but behaviour is possible and accepted.  Should not crash.
        a_callbackFunctionCounter = 0;
        SIMEventHandlerTest::a_callbackStaticCounter = 0;
        simulatorRef2.run(12);
        REQUIRE( a_callbackFunctionCounter == 1);
        REQUIRE_NOTHROW(SimulationEngine::getInstance().unschedule(event)); // This event is in the past.
    }

    SECTION ( "Checks that events are being unscheduled" ) {
        a_callbackFunctionCounter = 0;
        SIMEventHandlerTest::a_callbackStaticCounter = 0;
        //REQUIRE( simulatorRef2.getDiagnosticInfo()["num_current_events"] == 0);
        long num_events = simulatorRef2.getDiagnosticInfo()["num_current_events"];
        REQUIRE_NOTHROW(SimulationEngine::getInstance().unschedule(event));
        //REQUIRE( simulatorRef2.getNumberOfEvents() == (num_events - 1));
        REQUIRE( simulatorRef2.getDiagnosticInfo()["num_current_events"] == (num_events - 1));
        simulatorRef2.run(12); // Does not trigger event anymore.
        REQUIRE( a_callbackFunctionCounter == 0);
    }

    SECTION ("Checks behavior of the simulation engine when simulation is stopped") {
        shared_ptr<EventHandler> eventHandler_stopOnHandling = EventHandler::create(SIMEventHandlerTest::a_callback_that_stops_the_simulation,
            "callback_stops_simulation");
        entity->waitOnce(eventHandler_stopOnHandling, event);
        simulatorRef1.run(100);
        // Handler should have stopped the simulation.
        REQUIRE ( simulatorRef1.getState() == SimState::Stopped);
        // Simulation stops and time is not advanced. 
        REQUIRE ( simulatorRef1.getCurrentTime() == 10);
        // Simulation should not run anymore 
        REQUIRE_THROWS( simulatorRef1.run());
    }
}
