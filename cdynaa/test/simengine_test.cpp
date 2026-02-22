#include "catch.h"
#include "../include/simengine.h"
#include "entitytest.h"
#include "simenginetest.h"

#include <string>
#include <iostream>
#include <functional>

using namespace std;
using namespace dynaa;


TEST_CASE( "Checks the functionality of the simulation engine", "[simengine][stable]" ) {
    // Setup the simulation engine, some event handlers and some events.
    DynAASim &simulatorRef1 = DynAASim::getInstance();
    DynAASim &simulatorRef2 = DynAASim::getInstance();

    REQUIRE_NOTHROW(simulatorRef1.reset()); // Should not throw because simulation is not running

    shared_ptr<EntityTest> entity = make_shared<EntityTest>();
    const EventType eventTypeA("TYPE_A", "Description A");
    EventExpression event = entity->scheduleAt(10, eventTypeA);
    entity->scheduleAt(20, eventTypeA);
    // auto diagInfo = simulatorRef2.diagnostics.getFutureStatistics();
    auto diagInfo = Diagnostics::getInstance().getFutureStatistics();
    REQUIRE( diagInfo["num_events_scheduled"] == 2);
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

    diagInfo = Diagnostics::getInstance().getFutureStatistics();
    REQUIRE( diagInfo["num_registered_handlers"] == 3);

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
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 0);
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
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 0);
        REQUIRE( simulatorRef1.getCurrentTime() == 0);
    }

    SECTION ( "Check current time and state stepping through instants") {
        // First we add some more events to be able to step through
        const dynaa::EventType &eventTypeB = EventType("TYPE_B", "Description B");
        const dynaa::EventType &eventTypeC = EventType("TYPE_C", "Description C");
        EventExpression event = entity->scheduleAt(10, eventTypeB); // 2 events at t=10
        entity->scheduleAt(20, eventTypeB);
        entity->scheduleAt(20, eventTypeC); // 3 events at t=20
        // Now we start stepping and checking
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_current_instants"] == 2);
        simulatorRef1.runInstant(1);
        REQUIRE( simulatorRef1.getCurrentTime() == 20 );
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_current_instants"] == 1);
        simulatorRef1.runInstant(0); // 0 instants should be ignored
        REQUIRE( simulatorRef1.getCurrentTime() == 20 );
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_current_instants"] == 1);
        simulatorRef1.runInstant(2); //Last event in t=10 and first in t=20
        REQUIRE( simulatorRef1.getCurrentTime() == 20);
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_current_instants"] == 0);
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
    }

    SECTION ( "Check current time and state stepping through events") {
        // First we add some more events to be able to step through
        const dynaa::EventType &eventTypeB = EventType("TYPE_B", "Description B");
        const dynaa::EventType &eventTypeC = EventType("TYPE_C", "Description C");
        EventExpression event = entity->scheduleAt(10, eventTypeB); // 2 events at t=10
        entity->scheduleAt(20, eventTypeB);
        entity->scheduleAt(20, eventTypeC); // 3 events at t=20
        // Now we start stepping and checking
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 5);
        simulatorRef1.runEvent(1);
        REQUIRE( simulatorRef1.getCurrentTime() == 10 );
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 4);
        simulatorRef1.runEvent(0); // 0 events should be ignored
        REQUIRE( simulatorRef1.getCurrentTime() == 10 );
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 4);
        simulatorRef1.runEvent(2); //Last event in t=10 and first in t=20
        REQUIRE( simulatorRef1.getCurrentTime() == 20 );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 2);
        simulatorRef1.runEvent(4); // Off-bounds, should not fail
        REQUIRE( simulatorRef1.getCurrentTime() == 20);
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 0);
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
    }

    SECTION ( "Check current time and state combining stepping through events and instants") {
        // First we add some more events to be able to step through
        const dynaa::EventType &eventTypeB = EventType("TYPE_B", "Description B");
        const dynaa::EventType &eventTypeC = EventType("TYPE_C", "Description C");
        EventExpression event = entity->scheduleAt(10, eventTypeB); // 2 events at t=10
        entity->scheduleAt(20, eventTypeB);
        entity->scheduleAt(20, eventTypeC); // 3 events at t=20
        // Now we start stepping and checking
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 5);
        simulatorRef1.runEvent(1);
        REQUIRE( simulatorRef1.getCurrentTime() == 10 );
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 4);
        simulatorRef1.runInstant(1);
        REQUIRE( simulatorRef1.getCurrentTime() == 20 );
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 3);
        simulatorRef1.runInstant(1);
        REQUIRE( simulatorRef1.getCurrentTime() == 20);
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 0);
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
    }

    SECTION ( "Check current time and state after steping 2 instants together") {
        // First we add some more events to be able to step through
        const dynaa::EventType &eventTypeB = EventType("TYPE_B", "Description B");
        const dynaa::EventType &eventTypeC = EventType("TYPE_C", "Description C");
        EventExpression event = entity->scheduleAt(10, eventTypeB); // 2 events at t=10
        entity->scheduleAt(20, eventTypeB);
        entity->scheduleAt(20, eventTypeC); // 3 events at t=20
        // Now we start stepping and checking
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 5);
        simulatorRef1.runInstant(2);
        REQUIRE( simulatorRef1.getCurrentTime() == 20);
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 0);
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
    }

    SECTION ( "Check that a reset properly reset the cached events for the current instant") {
        // First we add some more events to be able to step through
        const dynaa::EventType &eventTypeB = EventType("TYPE_B", "Description B");
        const dynaa::EventType &eventTypeC = EventType("TYPE_C", "Description C");
        EventExpression event = entity->scheduleAt(10, eventTypeB); // 2 events at t=10
        entity->scheduleAt(20, eventTypeB);
        entity->scheduleAt(20, eventTypeC); // 3 events at t=20
        // Now we start stepping and checking
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 5);
        simulatorRef1.runEvent(1);
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 4);
        simulatorRef1.reset();
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 0);
        entity->scheduleAt(20, eventTypeB);
        entity->scheduleAt(20, eventTypeC);
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 2);
        simulatorRef1.runInstant(1);
        REQUIRE( Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == 0);
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
    }


    // SECTION ( "No handler is called if source of event is deleted before the event is issued.") {
    //     REQUIRE( simulatorRef1.getCurrentTime() == 0 );
    //     simulatorRef1.run(5);
    //     REQUIRE( simulatorRef1.getCurrentTime() == 5 );
    //     REQUIRE( eventHandlerTest->callbackMethodCounter == 0);
    //     entity.reset(); // Destroys the entity, which is source of eventA in time 10.
    //     REQUIRE_NOTHROW(simulatorRef1.run(12));
    //     REQUIRE( simulatorRef1.getCurrentTime() == 12 );
    //     REQUIRE( eventHandlerTest->callbackMethodCounter ==
    //              0); // An is issued in 10, which increments this counter, but should not be handled.
    // }

    // SECTION ( "Pure simulation in the core does not accumulate owner for entities") {
    //     REQUIRE( simulatorRef1.getCurrentTime() == 0 );
    //     simulatorRef1.run(25);
    //     REQUIRE( entity.use_count() == 1);
    // }

    // SECTION ( "Checks that events are being issued and watched." ) {
    //     a_callbackFunctionCounter = 0;
    //     SIMEventHandlerTest::a_callbackStaticCounter = 0;
    //     simulatorRef2.run(12);
    //     REQUIRE( a_callbackFunctionCounter == 1);
    //     REQUIRE( SIMEventHandlerTest::a_callbackStaticCounter == 1);
    //     REQUIRE( eventHandlerTest->callbackMethodCounter == 1);
    //     simulatorRef2.run();
    //     REQUIRE( a_callbackFunctionCounter == 2);
    //     REQUIRE( SIMEventHandlerTest::a_callbackStaticCounter == 1);
    //     REQUIRE( eventHandlerTest->callbackMethodCounter == 2);
    //     REQUIRE( simulatorRef1.getCurrentTime() == 20);
    // }

    // SECTION ( "Check if schedulingAt a past time fails with exception.") {
    //     REQUIRE( simulatorRef1.getCurrentTime() == 0 );
    //     simulatorRef1.run(5);
    //     REQUIRE( simulatorRef1.getCurrentTime() == 5 );
    //     REQUIRE_THROWS_AS( entity->scheduleAt(2, eventTypeA), InvalidTimePointException &);
    // }

    SECTION( "Unscheduling events should work for events inserted now, at, or after" ) {
        EventExpression eventNow = entity->scheduleNow(eventTypeA);
        REQUIRE(entity->unschedule(eventNow.atomicEventType(), eventNow.atomicEventID()) == 1);
        EventExpression eventAt5 = entity->scheduleAt(5, eventTypeA);
        REQUIRE(entity->unschedule(eventAt5.atomicEventType(), eventAt5.atomicEventID()) == 1);
        EventExpression eventAfter10 = entity->scheduleAfter(10, eventTypeA);
        REQUIRE(entity->unschedule(eventAfter10.atomicEventType(), eventAfter10.atomicEventID()) == 1);
    }

    SECTION( "Unscheduling events should silently do nothing if events where removed previously" ) {
        // Allows unscheduling the same event many times (different actors for example).
        // No problem with this behavior, and should not break code.
        EventExpression eventNow = entity->scheduleNow(eventTypeA);
        REQUIRE(entity->unschedule(eventNow.atomicEventType(), eventNow.atomicEventID()) == 1);
        REQUIRE(entity->unschedule(eventNow.atomicEventType(), eventNow.atomicEventID()) == 0);
        REQUIRE(entity->unschedule(eventNow.atomicEventType(), eventNow.atomicEventID()) == 0);
    }

    SECTION ( "Checks that events in the past can be unscheduled as well (silently does nothing)" ) {
        // Nothing really is affected, but behaviour is possible and accepted.  Should not crash.
        a_callbackFunctionCounter = 0;
        SIMEventHandlerTest::a_callbackStaticCounter = 0;
        simulatorRef2.run(12);
        REQUIRE(a_callbackFunctionCounter == 1);
        REQUIRE(entity->unschedule(event.atomicEventType(), event.atomicEventID()) == 0); // This event is in the past.
    }

    SECTION ( "Checks that events are being unscheduled" ) {
        a_callbackFunctionCounter = 0;
        SIMEventHandlerTest::a_callbackStaticCounter = 0;
        long num_events = Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"];
        REQUIRE(entity->unschedule(event.atomicEventType(), event.atomicEventID()) == 1);
        REQUIRE(Diagnostics::getInstance().getFutureStatistics()["num_events_scheduled"] == (num_events - 1));
        simulatorRef2.run(12); // Does not trigger event anymore.
        REQUIRE(a_callbackFunctionCounter == 0);
    }

    // TODO unschedule should also be tested for ANY_TYPE and ANY_ID

    // SECTION ("Checks behavior of the simulation engine when simulation is stopped") {
    //     shared_ptr<EventHandler> eventHandler_stopOnHandling = EventHandler::create(
    //                 SIMEventHandlerTest::a_callback_that_stops_the_simulation,
    //                 "callback_stops_simulation");
    //     entity->waitOnce(eventHandler_stopOnHandling, event);
    //     simulatorRef1.run(100);
    //     // Handler should have stopped the simulation.
    //     REQUIRE ( simulatorRef1.getState() == SimState::Stopped);
    //     // Simulation stops and time is not advanced.
    //     REQUIRE ( simulatorRef1.getCurrentTime() == 10);
    //     // Simulation should not run anymore
    //     REQUIRE_THROWS( simulatorRef1.run());
    // }
}

TEST_CASE( "Checks the functionality of the simulation engine at huge simulation times", "[simengine][stable]" ) {
    // Setup the simulation engine, some event handlers and some events.
    DynAASim &simulatorRef1 = DynAASim::getInstance();
    DynAASim &simulatorRef2 = DynAASim::getInstance();

    REQUIRE_NOTHROW(simulatorRef1.reset()); // Should not throw because simulation is not running

    shared_ptr<EntityTest> entity = make_shared<EntityTest>();
    const EventType eventTypeA("TYPE_A", "Description A");
    EventExpression event = entity->scheduleAt(1e12, eventTypeA);
    entity->scheduleAt(1e14, eventTypeA);
    // auto diagInfo = simulatorRef2.diagnostics.getFutureStatistics();
    auto diagInfo = Diagnostics::getInstance().getFutureStatistics();
    REQUIRE( diagInfo["num_events_scheduled"] == 2);
    REQUIRE( diagInfo["num_current_instants"] == 2);

    // Setup event handlers
    shared_ptr<EventHandler> eventHandler1 = EventHandler::create(a_callback_function);

    entity->wait(eventHandler1, entity);

    SECTION ( "Check we can run until large simulation times" ) {
        REQUIRE( simulatorRef1.getCurrentTime() == 0 );
        simulatorRef1.run(1e10);
        REQUIRE( simulatorRef1.getCurrentTime() == 1e10 );
        REQUIRE( simulatorRef1.getState() == SimState::Idle );
        simulatorRef1.run(2e25);
        REQUIRE( simulatorRef1.getCurrentTime() == 2e25 );
        simulatorRef1.run();
        REQUIRE( simulatorRef1.getCurrentTime() == 2e25 );
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
