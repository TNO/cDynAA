#include "catch.h"
#include "entitytest.h"
#include "simenginetest.h"
#include "../include/event.h"
#include "../include/eventtype.h"
#include "../include/diagnostics.h"
#include "../include/diagnosticseventstorage.h"
#include "../include/diagnosticseventstorageinterface.h"

#include <string>
#include <iostream>

using namespace dynaa;

void dummy(HandlerReport e) {
    static int number;
    ++number;
    return;
}

TEST_CASE( "Activate and deactivate diagnostics engine", "[diagnostics][stable]" ) {
    Diagnostics& diagnostics = Diagnostics::getInstance();
    SECTION( "It is initially deactivated" ) {
        REQUIRE(diagnostics.isDiagnosticsStorageActive() == false);
    }
    SECTION( "Can be activated" ) {
        REQUIRE(diagnostics.isDiagnosticsStorageActive() == false);
        diagnostics.activateDiagnosticsStorage();
        REQUIRE(diagnostics.isDiagnosticsStorageActive() == true);
    }
    SECTION( "Can be deactivated" ) {
        REQUIRE(diagnostics.isDiagnosticsStorageActive() == true); // Remains active from previous section
        diagnostics.deactivateDiagnosticsStorage();
        REQUIRE(diagnostics.isDiagnosticsStorageActive() == false);
    }
}
// Tests to the past
TEST_CASE( "Get past events from diagnostics engine", "[diagnostics][stable]" ) {
    /*
    *  This test is basically the same as in the events storage, but using the main module
    */

    // Create useful variables
    Diagnostics& diagnostics = Diagnostics::getInstance();
    diagnostics.activateDiagnosticsStorage();
    shared_ptr<EntityTest> testEntityA = make_shared<EntityTest>();
    shared_ptr<EntityTest> testEntityB = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = EventType("TYPE_B", "Description B");
    Event event1 = Event::eventFactory(testEntityA, eventTypeA);
    Event event2 = Event::eventFactory(testEntityA, eventTypeB);
    Event event3 = Event::eventFactory(testEntityB, eventTypeA);
    Event event4 = Event::eventFactory(testEntityB, eventTypeB);


    SECTION( "Return past events for the current run, same time" ) {
        // Populate storage
        diagnostics.addDiagnosticsRecord(event1);
        diagnostics.addDiagnosticsRecord(event2);
        diagnostics.addDiagnosticsRecord(event3);

        // Retrieve values
        DiagnosticsEventTimeline timeline = diagnostics.getPastEvents();
        REQUIRE(timeline.size() == 3); // Returns all the values
        REQUIRE(timeline[0].second.atomicEventSourceID() == event1.source()->getUID());
        REQUIRE(timeline[0].second.atomicEventType() == event1.type());
        REQUIRE(timeline[1].second.atomicEventSourceID() == event2.source()->getUID());
        REQUIRE(timeline[1].second.atomicEventType() == event2.type());
        REQUIRE(timeline[2].second.atomicEventSourceID() == event3.source()->getUID());
        REQUIRE(timeline[2].second.atomicEventType() == event3.type());
    }

    SECTION( "Return past events for empty run" ) {
        DiagnosticsEventTimeline timeline = diagnostics.getPastEvents();
        REQUIRE(timeline.size() == 0); // Returns empty vector
    }

    SECTION( "Return past events with multiple runs" ) {
        // Populate first run
        diagnostics.addDiagnosticsRecord(event1);
        diagnostics.addDiagnosticsRecord(event2);
        // New run
        diagnostics.startNewRun();
        // Populate second run
        diagnostics.addDiagnosticsRecord(event3);
        diagnostics.addDiagnosticsRecord(event2);
        diagnostics.addDiagnosticsRecord(event1);
        // New run
        diagnostics.startNewRun();
        // Populate third run
        diagnostics.addDiagnosticsRecord(event4);
        // Retrieve values for current run
        DiagnosticsEventTimeline timeline = diagnostics.getPastEventsLastRun();
        REQUIRE(timeline.size() == 1); // Returns only values in last run
        REQUIRE(timeline[0].second.atomicEventSourceID() == event4.source()->getUID());
        REQUIRE(timeline[0].second.atomicEventType() == event4.type());
        // Retrieve values for first run
        timeline = diagnostics.getPastEvents(0);
        REQUIRE(timeline.size() == 2); // Returns only values in first run
        REQUIRE(timeline[0].second.atomicEventSourceID() == event1.source()->getUID());
        REQUIRE(timeline[0].second.atomicEventType() == event1.type());
        // Retrieve values for second run
        timeline = diagnostics.getPastEvents(1);
        REQUIRE(timeline.size() == 3); // Returns only values in second run
        REQUIRE(timeline[0].second.atomicEventSourceID() == event3.source()->getUID());
        REQUIRE(timeline[0].second.atomicEventType() == event3.type());
        // Retrieve values for out of range run
        timeline = diagnostics.getPastEvents(9);
        REQUIRE(timeline.size() == 1); // Returns last (current) run
        REQUIRE(timeline[0].second.atomicEventSourceID() == event4.source()->getUID());
        REQUIRE(timeline[0].second.atomicEventType() == event4.type());
    }
    SECTION( "Deactivated diagnostics doesn't store" ) {
        diagnostics.deactivateDiagnosticsStorage();
        // Try to opulate storage
        diagnostics.addDiagnosticsRecord(event1);
        diagnostics.addDiagnosticsRecord(event2);
        diagnostics.addDiagnosticsRecord(event3);

        // Retrieve values
        DiagnosticsEventTimeline timeline = diagnostics.getPastEvents();
        REQUIRE(timeline.size() == 0);
    }
    SECTION( "Deactivated diagnostics can be reactivated" ) {
        diagnostics.deactivateDiagnosticsStorage();
        // Try to opulate storage
        diagnostics.addDiagnosticsRecord(event1);
        diagnostics.addDiagnosticsRecord(event2);
        diagnostics.activateDiagnosticsStorage();
        diagnostics.addDiagnosticsRecord(event3);

        // Retrieve values
        DiagnosticsEventTimeline timeline = diagnostics.getPastEvents();
        REQUIRE(timeline.size() == 1);
    }
    diagnostics.reset();
}

TEST_CASE( "Get past statistics from diagnostics engine", "[diagnostics][stable]" ) {
    // Create useful variables
    Diagnostics& diagnostics = Diagnostics::getInstance();
    diagnostics.activateDiagnosticsStorage();
    shared_ptr<EntityTest> testEntityA = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType("TYPE_A", "Description A");
    Event event1 = Event::eventFactory(testEntityA, eventTypeA);
    EventID id1 = 1;
    HandlerToken hToken1 = createHandlerToken(1, 1);
    EventExpression evExprAA1 = EventExpression(testEntityA, eventTypeA, id1);
    std::vector<HandlerReport::EventReport> dummy_atom_state_vector;
    HandlerReport eRecord1 = HandlerReport(hToken1, evExprAA1, 1, false, dummy_atom_state_vector);

    SECTION( "Test with single run" ) {
        // A few constants to make the test more readable
        int events = 15, events_copy = 15;
        int handlers = 9, handlers_copy = 9;
        int trig_userexpressions = 7, trig_userexpressions_copy = 7;
        int reg_userespressions = 17, reg_userespressions_copy = 17;
        int trig_atom = 33, trig_atom_copy = 33;
        int reg_atom = 0;
        int complexity = 1;

        // Trigger the counters
        while (events--) {
            diagnostics.addDiagnosticsRecord(event1);
            diagnostics.incrementEventsCounter();
        }
        while (handlers--) {
            diagnostics.addDiagnosticsRecord(hToken1, eRecord1);
            diagnostics.incrementHandlersCounter();
            diagnostics.incrementCallbacksCounter();
        }
        while (trig_userexpressions--) diagnostics.storeTriggeredUserExprComplexity(trig_userexpressions+1);
        while (reg_userespressions--) {
            reg_atom += complexity;
            diagnostics.incrementRegisteredAtomExprCounter(complexity++);
        }
        while (trig_atom--) diagnostics.incrementTriggeredAtomExprCounter();

        // Get stats and check results
        std::map<string,double> stats = diagnostics.getPastStatistics();
        REQUIRE(stats["total_events_triggered"] == events_copy);
        REQUIRE(stats["current_run_events_triggered"] == events_copy);
        REQUIRE(stats["average_events_per_instant"] == events_copy); // How to do this better? (simulate different times)
        REQUIRE(stats["variance_events_per_instant"] == 0);
        REQUIRE(stats["total_handlers_triggered"] == handlers_copy);
        REQUIRE(stats["current_run_handlers_triggered"] == handlers_copy);
        REQUIRE(stats["average_handlers_per_event"] == (double) handlers_copy / events_copy);
        REQUIRE(stats["average_handlers_per_instant"] == handlers_copy); // How to do this better? (simulate different times)
        REQUIRE(stats["variance_handlers_per_instant"] == 0);
        REQUIRE(stats["user_expressions_triggered"] == trig_userexpressions_copy);
        REQUIRE(stats["user_expressions_registered"] == reg_userespressions_copy);
        REQUIRE(stats["atomic_expressions_triggered"] == trig_atom_copy);
        REQUIRE(stats["average_complexity_registered_expressions"] == (double) reg_atom / reg_userespressions_copy);
        REQUIRE(stats["variance_complexity_registered_expressions"] == 25.5);
        REQUIRE(stats["average_complexity_triggered_expressions"] == (double) trig_atom_copy / trig_userexpressions_copy);
        REQUIRE(stats["variance_complexity_triggered_expressions"] == 28.0/6.0);
        REQUIRE(stats["number_of_callback_functions"] == handlers_copy);
        REQUIRE(stats["number_of_callback_functions_current_run"] == handlers_copy);
    }

    SECTION( "Test with multiple runs" ) {
        // A few constants to make the test more readable
        int events = 15, events_copy = 15;
        int handlers = 9, handlers_copy = 9;
        int trig_userexpressions = 7, trig_userexpressions_copy = 7;
        int reg_userespressions = 17, reg_userespressions_copy = 17;
        int trig_atom = 33, trig_atom_copy = 33;
        int reg_atom = 25, reg_atom_copy = 25;
        int complexity = 1;

        // Trigger the counters
        while (events--) {
            diagnostics.addDiagnosticsRecord(event1);
            diagnostics.incrementEventsCounter();
        }
        while (handlers--) {
            diagnostics.addDiagnosticsRecord(hToken1, eRecord1);
            diagnostics.incrementHandlersCounter();
            diagnostics.incrementCallbacksCounter();
        }
        while (trig_userexpressions--) diagnostics.storeTriggeredUserExprComplexity(trig_userexpressions+1);
        while (trig_atom--) diagnostics.incrementTriggeredAtomExprCounter();
        while (reg_userespressions--) {
            reg_atom += complexity;
            diagnostics.incrementRegisteredAtomExprCounter(complexity++);
        }

        diagnostics.startNewRun();

        // Do the same for the second run
        int events2 = 5, events2_copy = 5;
        int handlers2 = 99, handlers2_copy = 99;
        int trig_userexpressions2 = 5, trig_userexpressions2_copy = 5;
        int reg_userespressions2 = 19, reg_userespressions2_copy = 19;
        int trig_atom2 = 30, trig_atom2_copy = 30;
        int reg_atom2 = 0;
        complexity=1;

        // Trigger the counters
        while (events2--) {
            diagnostics.addDiagnosticsRecord(event1);
            diagnostics.incrementEventsCounter();
        }
        while (handlers2--) {
            diagnostics.addDiagnosticsRecord(hToken1, eRecord1);
            diagnostics.incrementHandlersCounter();
            diagnostics.incrementCallbacksCounter();
        }
        while (trig_userexpressions2--) diagnostics.storeTriggeredUserExprComplexity(trig_userexpressions2+1);
        while (trig_atom2--) diagnostics.incrementTriggeredAtomExprCounter();
        while (reg_userespressions2--) {
            reg_atom2 += complexity;
            diagnostics.incrementRegisteredAtomExprCounter(complexity++);
        }

        // Get stats and check results
        std::map<string,double> stats = diagnostics.getPastStatistics();
        REQUIRE(stats["total_events_triggered"] == events_copy + events2_copy);
        REQUIRE(stats["current_run_events_triggered"] == events2_copy);
        REQUIRE(stats["average_events_per_instant"] == events_copy +
                events2_copy); // How to do this better? (simulate different times)
        REQUIRE(stats["variance_events_per_instant"] == 0);
        REQUIRE(stats["total_handlers_triggered"] == handlers_copy + handlers2_copy);
        REQUIRE(stats["current_run_handlers_triggered"] == handlers2_copy);
        REQUIRE(stats["average_handlers_per_event"] == (double) handlers2_copy / events2_copy);
        REQUIRE(stats["average_handlers_per_instant"] == handlers_copy +
                handlers2_copy); // How to do this better? (simulate different times)
        REQUIRE(stats["variance_handlers_per_instant"] == 0);
        REQUIRE(stats["user_expressions_triggered"] == trig_userexpressions2_copy);
        REQUIRE(stats["user_expressions_registered"] == reg_userespressions2_copy);
        REQUIRE(stats["atomic_expressions_triggered"] == trig_atom2_copy);
        REQUIRE(stats["average_complexity_registered_expressions"] == (double) reg_atom2 / reg_userespressions2_copy);
        REQUIRE(stats["variance_complexity_registered_expressions"] == 570.0/18.0);
        REQUIRE(stats["average_complexity_triggered_expressions"] == (double) trig_atom2_copy / trig_userexpressions2_copy);
        REQUIRE(stats["variance_complexity_triggered_expressions"] == 2.5);
        REQUIRE(stats["number_of_callback_functions"] == handlers_copy + handlers2_copy);
        REQUIRE(stats["number_of_callback_functions_current_run"] == handlers2_copy);
    }
    diagnostics.reset();
}

TEST_CASE( "Get past handlers and handler records from diagnostics engine", "[diagnostics][stable]" ) {
    // Create useful variables
    Diagnostics& diagnostics = Diagnostics::getInstance();
    diagnostics.activateDiagnosticsStorage();
    shared_ptr<EntityTest> testEntityA = make_shared<EntityTest>();
    shared_ptr<EntityTest> testEntityB = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = EventType("TYPE_B", "Description B");
    EventID id1 = 1;
    EventID id2 = 2;
    EventID id3 = 3;
    EventID id4 = 4;
    HandlerToken hToken1 = createHandlerToken(1, 1);
    HandlerToken hToken2 = createHandlerToken(2, 1);
    HandlerToken hToken3 = createHandlerToken(3, 1);
    HandlerToken hToken4 = createHandlerToken(4, 1);
    SimTime t1 = 1.2;
    SimTime t2 = 3.4;
    SimTime t3 = 5.6;
    SimTime t4 = 7.8;
    EventExpression evExprAA1 = EventExpression(testEntityA, eventTypeA, id1);
    EventExpression evExprAA2 = EventExpression(testEntityA, eventTypeA, id2);
    EventExpression evExprAA3 = EventExpression(testEntityA, eventTypeA, id3);
    EventExpression evExprAA4 = EventExpression(testEntityA, eventTypeA, id4);
    std::vector<HandlerReport::EventReport> dummy_atom_state_vector;
    HandlerReport eRecord1 = HandlerReport(hToken1, evExprAA1, 1, false, dummy_atom_state_vector);
    HandlerReport eRecord2 = HandlerReport(hToken1, evExprAA2, 2, false, dummy_atom_state_vector);
    HandlerReport eRecord3 = HandlerReport(hToken1, evExprAA3, 3, false, dummy_atom_state_vector);
    HandlerReport eRecord4 = HandlerReport(hToken1, evExprAA4, 4, false, dummy_atom_state_vector);

    SECTION( "Time with multiple handlers" ) {
        diagnostics.addDiagnosticsRecord(hToken1, eRecord1);
        diagnostics.addDiagnosticsRecord(hToken2, eRecord2);
        diagnostics.addDiagnosticsRecord(hToken3, eRecord3);

        std::vector<HandlerToken> tokens = diagnostics.getPastHandlers(0);

        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0] == hToken1);
        REQUIRE(tokens[1] == hToken2);
        REQUIRE(tokens[2] == hToken3);
    }

    SECTION( "Time without handlers" ) {
        diagnostics.addDiagnosticsRecord(hToken1, eRecord1);
        diagnostics.addDiagnosticsRecord(hToken2, eRecord2);
        diagnostics.addDiagnosticsRecord(hToken3, eRecord3);

        std::vector<HandlerToken> tokens = diagnostics.getPastHandlers(1);

        REQUIRE(tokens.size() == 0);
    }

    SECTION( "Multiple handlers, needs to find the right one" ) {
        diagnostics.addDiagnosticsRecord(hToken1, eRecord1);
        diagnostics.addDiagnosticsRecord(hToken2, eRecord2);
        diagnostics.addDiagnosticsRecord(hToken3, eRecord3);
        diagnostics.addDiagnosticsRecord(hToken2, eRecord4);

        DiagnosticsHandlerTimeline records = diagnostics.getPastHandlerReports(hToken2);
        REQUIRE_FALSE(hToken1==hToken2);
        REQUIRE(records.size() == 2);
        REQUIRE(records[0].second->getEventExpression().atomicEventID() == id2);
        REQUIRE(records[1].second->getEventExpression().atomicEventID() == id4);
    }

    SECTION( "Handler token is not matched" ) {
        diagnostics.addDiagnosticsRecord(hToken1, eRecord1);
        diagnostics.addDiagnosticsRecord(hToken4, eRecord2);
        diagnostics.addDiagnosticsRecord(hToken3, eRecord3);

        DiagnosticsHandlerTimeline records = diagnostics.getPastHandlerReports(hToken2);

        REQUIRE(records.size() == 0);
    }
    diagnostics.reset();
}

// Tests to the future
TEST_CASE( "Get future events", "[diagnostics][stable]") {
    // Useful variables
    Diagnostics& diagnostics = Diagnostics::getInstance();
    diagnostics.activateDiagnosticsStorage();
    DynAASim &simulatorRef1 = DynAASim::getInstance();
    simulatorRef1.reset();
    shared_ptr<EntityTest> testEntityA = make_shared<EntityTest>();
    shared_ptr<EntityTest> testEntityB = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = EventType("TYPE_B", "Description B");

    SECTION( "Simple get future events" ) {
        // Schedule some events
        testEntityA->scheduleNow(eventTypeB);
        testEntityB->scheduleNow(eventTypeB);
        testEntityA->scheduleAt(10, eventTypeA);
        testEntityA->scheduleAt(20, eventTypeA);

        DiagnosticsEventTimeline timeline = diagnostics.getFutureEvents();

        REQUIRE(timeline.size() == 4);
        REQUIRE(timeline[0].first == 0);
        REQUIRE(timeline[0].second.atomicEventType() == eventTypeB);
        //REQUIRE(timeline[0].second.source()->getUID() == testEntityB->getUID());
        REQUIRE(timeline[1].first == 0);
        REQUIRE(timeline[1].second.atomicEventType() == eventTypeB);
        //REQUIRE(timeline[1].second.source()->getUID() == testEntityA->getUID());
        REQUIRE(timeline[2].first == 10);
        REQUIRE(timeline[2].second.atomicEventType() == eventTypeA);
        REQUIRE(timeline[2].second.atomicEventSourceID() == testEntityA->getUID());
        REQUIRE(timeline[3].first == 20);
        REQUIRE(timeline[3].second.atomicEventType() == eventTypeA);
        REQUIRE(timeline[3].second.atomicEventSourceID() == testEntityA->getUID());
    }

    SECTION( "Get time of last event and next event" ) {
        // Schedule some events
        testEntityA->scheduleAt(10, eventTypeA);
        testEntityA->scheduleAt(20, eventTypeA);
        SimTime last = diagnostics.getTimeOfMostRecentEvent();
        SimTime next = diagnostics.getTimeOfNextEvent();

        REQUIRE(last == 0); // Last event (current time)
        REQUIRE(next == 10); // Next event (in future)
    }

    SECTION( "Get time of next event when no more events are scheduled" ) {
        // Schedule some events
        SimTime last = diagnostics.getTimeOfMostRecentEvent();
        SimTime next = diagnostics.getTimeOfNextEvent();

        REQUIRE(last == 0); // Last event (current time)
        REQUIRE(next == -1); // Next event (in future)
    }

    SECTION( "Get events in the next instant" ) {
        // Schedule some events
        testEntityA->scheduleAt(10, eventTypeA);
        testEntityB->scheduleAt(20, eventTypeB);
        testEntityA->scheduleAt(10, eventTypeA);
        DiagnosticsEventTimeline timeline = diagnostics.getEventsAtNextInstant();

        REQUIRE(timeline.size() == 2);
        REQUIRE(timeline[0].first == 10);
        REQUIRE(timeline[0].second.atomicEventType() == eventTypeA);
        REQUIRE(timeline[0].second.atomicEventSourceID() == testEntityA->getUID());
        REQUIRE(timeline[1].first == 10);
        REQUIRE(timeline[1].second.atomicEventType() == eventTypeA);
        REQUIRE(timeline[1].second.atomicEventSourceID() == testEntityA->getUID());
    }

    SECTION( "Get events in the next instant, but there isn't any" ) {
        DiagnosticsEventTimeline timeline = diagnostics.getEventsAtNextInstant();

        REQUIRE(timeline.size() == 0);
    }

    SECTION( "Get events the next n events" ) {
        // Schedule some events
        testEntityA->scheduleAt(10, eventTypeA);
        testEntityA->scheduleNow(eventTypeA);
        testEntityB->scheduleAt(10, eventTypeB);
        testEntityA->scheduleAt(20, eventTypeA);
        testEntityA->scheduleAt(15, eventTypeB);
        DiagnosticsEventTimeline timeline = diagnostics.getNextEvents(4);

        REQUIRE(timeline.size() == 4);
        REQUIRE(timeline[0].first == 0);
        REQUIRE(timeline[0].second.atomicEventType() == eventTypeA);
        REQUIRE(timeline[0].second.atomicEventSourceID() == testEntityA->getUID());
        REQUIRE(timeline[1].first == 10);
        // REQUIRE(timeline[1].second.atomicEventType() == eventTypeB); // see issue #132
        // REQUIRE(timeline[1].second.source()->getUID() == testEntityB->getUID());
        REQUIRE(timeline[2].first == 10);
        // REQUIRE(timeline[2].second.atomicEventType() == eventTypeA); // see issue #132
        // REQUIRE(timeline[2].second.source()->getUID() == testEntityA->getUID());
        REQUIRE(timeline[3].first == 15);
        REQUIRE(timeline[3].second.atomicEventType() == eventTypeB);
        REQUIRE(timeline[3].second.atomicEventSourceID() == testEntityA->getUID());
    }

    SECTION( "Get events the next n events, but there are less than n scheduled" ) {
        // Schedule some events
        testEntityA->scheduleAt(10, eventTypeA);
        testEntityA->scheduleNow(eventTypeA);
        testEntityB->scheduleAt(10, eventTypeB);
        testEntityA->scheduleAt(20, eventTypeA);
        testEntityA->scheduleAt(15, eventTypeB);
        DiagnosticsEventTimeline timeline = diagnostics.getNextEvents(8);

        REQUIRE(timeline.size() == 5);
    }

    SECTION( "Get events the next n events, but there are 0 scheduled" ) {
        // Schedule some events
        DiagnosticsEventTimeline timeline = diagnostics.getNextEvents(8);

        REQUIRE(timeline.size() == 0);
    }
}

TEST_CASE( "Get future statistics", "[diagnostics][stable]") {
    // Useful variables
    Diagnostics& diagnostics = Diagnostics::getInstance();
    diagnostics.activateDiagnosticsStorage();
    DynAASim &simulatorRef1 = DynAASim::getInstance();
    simulatorRef1.reset();
    shared_ptr<EntityTest> testEntityA = make_shared<EntityTest>();
    shared_ptr<EntityTest> testEntityB = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = EventType("TYPE_B", "Description B");

    SECTION( "Empty case 1") {
        // Get stats and check results
        std::map<string,double> stats = diagnostics.getFutureStatistics();
        REQUIRE(stats["num_current_instants"] == 0);
        REQUIRE(stats["average_delta_time_between_instants"] == 0);
        REQUIRE(stats["variance_delta_time_between_instants"] == 0);
        REQUIRE(stats["num_events_scheduled"] == 0);
        REQUIRE(stats["average_num_events_scheduled"] == 0);
        REQUIRE(stats["variance_num_events_scheduled"] == 0);
        REQUIRE(stats["num_registered_handlers"] == 0);
    }

    SECTION( "Example case 1") {
        // Schedule some events
        testEntityA->scheduleNow(eventTypeA);
        testEntityA->scheduleNow(eventTypeB);
        testEntityB->scheduleNow(eventTypeB);
        EventExpression event = testEntityA->scheduleAt(10, eventTypeA);
        testEntityB->scheduleAt(10, eventTypeB);
        testEntityA->scheduleAt(15, eventTypeB);
        testEntityA->scheduleAt(21, eventTypeA);
        testEntityA->scheduleAt(21, eventTypeA);

        // Setup event handlers
        shared_ptr<EventHandler> eventHandler1 = EventHandler::create(dummy);

        testEntityA->wait(eventHandler1, testEntityB);
        testEntityA->waitOnce(eventHandler1, event);
        testEntityB->wait(eventHandler1, eventTypeA);

        // Get stats and check results
        std::map<string,double> stats = diagnostics.getFutureStatistics();
        REQUIRE(stats["num_current_instants"] == 4);
        REQUIRE(stats["average_delta_time_between_instants"] == 7.0);
        REQUIRE(stats["variance_delta_time_between_instants"] == 7.0);
        REQUIRE(stats["num_events_scheduled"] == 8);
        REQUIRE(stats["average_num_events_scheduled"] == 8.0/4.0);
        REQUIRE(stats["variance_num_events_scheduled"] == 2.0/3.0);
        REQUIRE(stats["num_registered_handlers"] == 3);
    }
}