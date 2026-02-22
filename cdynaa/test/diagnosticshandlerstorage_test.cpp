#include "catch.h"
#include "entitytest.h"
#include "../include/handlerreport.h"
#include "../include/handlertoken.h"
#include "../include/diagnosticshandlerstorage.h"
#include "../include/diagnosticshandlerstorageinterface.h"

#include <string>
#include <iostream>

using namespace dynaa;

// DEFAULT STORAGE
TEST_CASE( "Get full handler records", "[diagnosticshandlerstorage][stable]" ) {
    // Create useful variables
    DiagnosticsHandlerStorage* storage = new DiagnosticsHandlerStorage();
    HandlerReportLabel label;
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

    SECTION( "Return past handlers for the current run" ) {
        // Populate storage
        storage->addDiagnosticsRecord(t1, eRecord1, hToken1);
        storage->addDiagnosticsRecord(t2, eRecord2, hToken2);
        storage->addDiagnosticsRecord(t3, eRecord3, hToken3);

        // Retrieve values
        DiagnosticsHandlerReportsList records = storage->getRecords();
        REQUIRE(records.size() == 3); // Returns all the values

        REQUIRE(records[0].timestamp == t1);  // Values are as expected and in order
        REQUIRE(records[0].hReport.getEventExpression().atomicEventID() == id1);
        REQUIRE(records[0].hToken == hToken1);
        REQUIRE(records[0].label == 0);

        REQUIRE(records[1].timestamp == t2);  // Values are as expected and in order
        REQUIRE(records[1].hReport.getEventExpression().atomicEventID() == id2);
        REQUIRE(records[1].hToken == hToken2);
        REQUIRE(records[1].label == 0);

        REQUIRE(records[2].timestamp == t3);  // Values are as expected and in order
        REQUIRE(records[2].hReport.getEventExpression().atomicEventID() == id3);
        REQUIRE(records[2].hToken == hToken3);
        REQUIRE(records[2].label == 0);
    }

    SECTION( "Return past handlers for empty run" ) {
        DiagnosticsHandlerReportsList records = storage->getRecords();
        REQUIRE(records.size() == 0); // Returns empty vector
    }

    SECTION( "Return past handlers with multiple runs" ) {
        // Populate first run
        storage->addDiagnosticsRecord(t1, eRecord1, hToken1);
        storage->addDiagnosticsRecord(t2, eRecord2, hToken2);
        // New run
        label = storage->getLabel();
        storage->setLabel(++label);
        // Populate second run
        storage->addDiagnosticsRecord(t1, eRecord1, hToken1);
        storage->addDiagnosticsRecord(t2, eRecord2, hToken2);
        storage->addDiagnosticsRecord(t3, eRecord3, hToken3);
        // New run
        label = storage->getLabel();
        storage->setLabel(++label);
        // Populate third run
        storage->addDiagnosticsRecord(t4, eRecord4, hToken4);
        // Retrieve values for current run
        DiagnosticsHandlerReportsList records = storage->getRecords();
        REQUIRE(records.size() == 6); // Returns values in all runs

        REQUIRE(records[0].timestamp == t1);  // Values are as expected and in order
        REQUIRE(records[0].hReport.getEventExpression().atomicEventID() == id1);
        REQUIRE(records[0].hToken == hToken1);
        REQUIRE(records[0].label == 0);

        REQUIRE(records[1].timestamp == t2);  // Values are as expected and in order
        REQUIRE(records[1].hReport.getEventExpression().atomicEventID() == id2);
        REQUIRE(records[1].hToken == hToken2);
        REQUIRE(records[1].label == 0);

        REQUIRE(records[2].timestamp == t1);  // Values are as expected and in order
        REQUIRE(records[2].hReport.getEventExpression().atomicEventID() == id1);
        REQUIRE(records[2].hToken == hToken1);
        REQUIRE(records[2].label == 1);

        REQUIRE(records[3].timestamp == t2);  // Values are as expected and in order
        REQUIRE(records[3].hReport.getEventExpression().atomicEventID() == id2);
        REQUIRE(records[3].hToken == hToken2);
        REQUIRE(records[3].label == 1);

        REQUIRE(records[4].timestamp == t3);  // Values are as expected and in order
        REQUIRE(records[4].hReport.getEventExpression().atomicEventID() == id3);
        REQUIRE(records[4].hToken == hToken3);
        REQUIRE(records[4].label == 1);

        REQUIRE(records[5].timestamp == t4);  // Values are as expected and in order
        REQUIRE(records[5].hReport.getEventExpression().atomicEventID() == id4);
        REQUIRE(records[5].hToken == hToken4);
        REQUIRE(records[5].label == 2);
    }
}