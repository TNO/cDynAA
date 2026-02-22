#include "catch.h"
#include "../include/eventtype.h"

#include <string>
#include <iostream>

using namespace std;
using namespace dynaa;

TEST_CASE( "EventType creation and comparisons", "[eventtype][stable]" ) {

    const string nameA = "TYPE_A";
    const string descrA = "Description A";
    const EventType defaultEventType;
    const EventType eventTypeA(nameA, descrA);

    SECTION( "Creation sets correct name and description" ) {
        REQUIRE( eventTypeA.name() == nameA);
        REQUIRE( eventTypeA.description() == descrA);
    }
    SECTION( "Unequal to others irrespective of common name or description" ) {
        REQUIRE( eventTypeA == eventTypeA);
        REQUIRE( eventTypeA != defaultEventType);
        const EventType eventTypeB(nameA, descrA);
        REQUIRE( eventTypeA != eventTypeB);
    }
    SECTION( "EventType can be copied (copy constructed) during assignment" ) {
        EventType eventTypeACopy = eventTypeA;
        REQUIRE (eventTypeACopy == eventTypeA);
        REQUIRE (eventTypeA.name() == eventTypeACopy.name());
        REQUIRE (eventTypeA.description() == eventTypeACopy.description());
        REQUIRE (&eventTypeACopy != &eventTypeA);
    }
    SECTION( "Convert to string for printing" ) {
        std::ostringstream ss;
        ss << eventTypeA;
        REQUIRE( ss.str().find(nameA) != std::string::npos );
    }

    SECTION( "Creating and copy creating of event types" ) {
        const EventType firstType("example_type","Just an example type");
        const EventType myEventType;  // Creates a different type !!!!
        REQUIRE(firstType != myEventType);
        const EventType secondType(firstType);
        REQUIRE(firstType == secondType);
    }
}

TEST_CASE( "EventType value semantics", "[eventtype][stable]" ) {

    const EventType eventTypeA("TYPE_A", "Description A");

    SECTION("Default construction") {
        EventType eventType;
    }

    SECTION("Copy construction") {
        EventType eventType_copy(eventTypeA);
        REQUIRE(eventType_copy == eventTypeA);
        REQUIRE(&eventType_copy != &eventTypeA);
    }

    SECTION("Assignment copy construction") {
        EventType eventType_copy = eventTypeA;
        REQUIRE(eventType_copy == eventTypeA);
        REQUIRE(&eventType_copy != &eventTypeA);
    }

    SECTION("Copy assignment") {
        EventType eventType;
        REQUIRE(&eventType != &eventTypeA);
        REQUIRE(eventType != eventTypeA);
        eventType = eventTypeA;
        REQUIRE(&eventType != &eventTypeA);
        REQUIRE(eventType == eventTypeA);
    }

    SECTION("Move constructor and move assignment") {
        EventType eventType("TYPE_A", "Description A");
        EventType eventTypeA(eventType);
        EventType eventType_moved(std::move(eventTypeA));
        REQUIRE(eventType_moved == eventTypeA);
        REQUIRE(&eventType_moved != &eventTypeA);
    }
}