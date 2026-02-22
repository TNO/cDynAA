#include "catch.h"
#include "entitytest.h"
#include "../include/event.h"
#include "../include/eventtype.h"
#include "../include/diagnosticseventstorage.h"
#include "../include/diagnosticseventstorageinterface.h"

#include <string>
#include <iostream>

using namespace dynaa;

// DEFAULT STORAGE

TEST_CASE( "Get full event records", "[diagnosticsstorage][stable]" ) {
    // Create useful variables
    DiagnosticsEventStorageInterface* storage = new DiagnosticsEventStorage();
    shared_ptr<EntityTest> testEntityA = make_shared<EntityTest>();
    shared_ptr<EntityTest> testEntityB = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = EventType("TYPE_B", "Description B");
    Event event1 = Event::eventFactory(testEntityA, eventTypeA);
    Event event2 = Event::eventFactory(testEntityA, eventTypeB);
    Event event3 = Event::eventFactory(testEntityB, eventTypeA);
    Event event4 = Event::eventFactory(testEntityB, eventTypeB);
    SimTime t1 = 1.2;
    SimTime t2 = 3.4;
    SimTime t3 = 5.6;

    SECTION( "Return past events for the current run" ) {
        // Populate storage
        storage->addDiagnosticsRecord(t1, event1);
        storage->addDiagnosticsRecord(t2, event2);
        storage->addDiagnosticsRecord(t3, event3);

        // Retrieve values
        DiagnosticsEventRecordsList records = storage->getRecords();
        REQUIRE(records.size() == 3); // Returns all the values
        REQUIRE(records[0].timestamp == t1);  // Values are as expected and in order
        REQUIRE(records[0].event.source() == event1.source());
        REQUIRE(records[0].event.type() == event1.type());
        REQUIRE(records[0].label == 0);
        REQUIRE(records[1].timestamp == t2);
        REQUIRE(records[1].event.source() == event2.source());
        REQUIRE(records[1].event.type() == event2.type());
        REQUIRE(records[1].label == 0);
        REQUIRE(records[2].timestamp == t3);
        REQUIRE(records[2].event.source() == event3.source());
        REQUIRE(records[2].event.type() == event3.type());
        REQUIRE(records[2].label == 0);
    }

    SECTION( "Return past events for empty run" ) {
        DiagnosticsEventRecordsList records = storage->getRecords();
        REQUIRE(records.size() == 0); // Returns empty vector
    }

    SECTION( "Return past events with multiple runs" ) {
        // Populate first run
        storage->addDiagnosticsRecord(t1, event1);
        storage->addDiagnosticsRecord(t1, event2);
        // New run
        storage->setLabel(storage->getLabel() + 1);
        // Populate second run
        storage->addDiagnosticsRecord(t2, event3);
        storage->addDiagnosticsRecord(t2, event2);
        storage->addDiagnosticsRecord(t3, event1);
        // New run
        storage->setLabel(storage->getLabel() + 1);
        // Populate third run
        storage->addDiagnosticsRecord(t1, event4);
        // Retrieve values for current run
        DiagnosticsEventRecordsList records = storage->getRecords();
        REQUIRE(records.size() == 6); // Returns values in all runs
        REQUIRE(records[0].timestamp == t1);  // Values are as expected and in order
        REQUIRE(records[0].event.source() == event1.source());
        REQUIRE(records[0].event.type() == event1.type());
        REQUIRE(records[0].label == 0);
        REQUIRE(records[1].timestamp == t1);
        REQUIRE(records[1].event.source() == event2.source());
        REQUIRE(records[1].event.type() == event2.type());
        REQUIRE(records[1].label == 0);
        REQUIRE(records[2].timestamp == t2);
        REQUIRE(records[2].event.source() == event3.source());
        REQUIRE(records[2].event.type() == event3.type());
        REQUIRE(records[2].label == 1);
        REQUIRE(records[3].timestamp == t2);
        REQUIRE(records[3].event.source() == event2.source());
        REQUIRE(records[3].event.type() == event2.type());
        REQUIRE(records[3].label == 1);
        REQUIRE(records[4].timestamp == t3);
        REQUIRE(records[4].event.source() == event1.source());
        REQUIRE(records[4].event.type() == event1.type());
        REQUIRE(records[4].label == 1);
        REQUIRE(records[5].timestamp == t1);
        REQUIRE(records[5].event.source() == event4.source());
        REQUIRE(records[5].event.type() == event4.type());
        REQUIRE(records[5].label == 2);
    }
}