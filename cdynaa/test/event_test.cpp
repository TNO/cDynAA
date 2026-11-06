#include "catch.h"
#include "../include/event.h"
#include "../include/eventtype.h"
#include "entitytest.h"

#include <string>
#include <iostream>

using namespace std;

TEST_CASE( "EventType creation and comparisons", "[event][stable]" ) {

    const string nameA = "TYPE_A";
    const string descrA = "Description A";
    const dynaa::EventType &eventTypeA = dynaa::EventType::create(nameA, descrA);

    SECTION( "Creation sets correct name and description" ) {
        REQUIRE( eventTypeA.name() == nameA);
        REQUIRE( eventTypeA.description() == descrA);
    }
    SECTION( "Unequal to others irrespective of common name or description" ) {
        REQUIRE( eventTypeA == eventTypeA);
        const EventType &eventTypeB = EventType::create(nameA, descrA);
        REQUIRE( eventTypeA != eventTypeB);
    }
    SECTION( "EventType can be copied (copy constructed) during assignment" ) {
        EventType eventTypeACopy = eventTypeA;
        REQUIRE (eventTypeACopy == eventTypeA);
        REQUIRE (&eventTypeACopy != &eventTypeA);
    }
    SECTION( "Convert to string for printing" ) {
        std::ostringstream ss;
        ss << eventTypeA;
        REQUIRE( ss.str().find(nameA) != std::string::npos );
    }

    SECTION( "Creating and copy creating of event types" ) {
        EventType firstType = EventType("example_type","Just an example type");
        EventType myEventType;  // Creates a different type !!!!
        REQUIRE(firstType != myEventType);
        EventType secondType = EventType(firstType);
        REQUIRE(firstType == secondType);
    }
}

TEST_CASE( "Event creation and usage", "[event][stable]" ) {

    shared_ptr<EntityTest> testEntity = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType::create("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = EventType::create("TYPE_B", "Description B");
    const dynaa::Event& event1 = testEntity->scheduleNow(eventTypeA);
    const dynaa::Event& event2 = testEntity->scheduleNow(eventTypeB);

    SECTION( "ID is unique to event and non-zero" ) {
        REQUIRE( event1.id() != 0 );
        REQUIRE( event2.id() != 0 );
        REQUIRE( event1.id() != event2.id() );
    }
    SECTION( "Source of event is set to entity that created it" ) {
        REQUIRE( event1.source()->getUID() == testEntity->getUID() );
    }
    SECTION( "Type of event matches type it was instantiated with" ) {
        REQUIRE( event1.type() == eventTypeA );
        REQUIRE( event1.type() != event2.type() );
    }
    SECTION( "Events are only equal if id, source and type all match" ) {
        REQUIRE( event1 == event1 );
        // event1 has same source as event2 but different type:
        REQUIRE( event1 != event2 );
        // event with same type as event1 but different source:
        const Event& event3 = testEntity->scheduleNow(eventTypeA);
        REQUIRE( event1 != event3 );
    }
    SECTION( "Event objects can be copy assigned.  Copies are equal." ) {
        const dynaa::Event event3 = event1; // Copy
        REQUIRE( &event3 != &event1 );
        REQUIRE( event3 == event1 );
        REQUIRE( event3 != event2 );
    }
    SECTION( "Event types can be copy assigned. Copies are equal." ) {
        EventType copyOfType = eventTypeA; // Copy
        const dynaa::Event event3 = event1; // Copy
        REQUIRE( copyOfType == eventTypeA );
        REQUIRE( &copyOfType != &eventTypeA );
        const EventType& type3 = event3.type();
        const EventType& type1 = event1.type();
        REQUIRE( type3 == type1);
        // Now we use const shared pointer to store EventType in Event, so address are equal
        REQUIRE( &(type3) == &(type1));
        //REQUIRE( &(type3) != &(type1));
    }
}

TEST_CASE( "Presentation", "[event][stable]" ) {

    shared_ptr<EntityTest> testEntity = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType::create("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = EventType::create("TYPE_B", "Description B");
    const dynaa::Event& event1 = testEntity->scheduleNow(eventTypeA);
    const dynaa::Event& event2 = testEntity->scheduleNow(eventTypeB);
    const dynaa::Event event3 = event1; // Copy

    SECTION( "Presentation of event and event type as string" ) {
        std::ostringstream ss_test;
        REQUIRE_NOTHROW( ss_test << eventTypeB << endl );
        REQUIRE_NOTHROW( ss_test << event1 << endl);
    }
    SECTION( "Convert to string for printing" ) {
        std::ostringstream ss_test, ss_result;
        REQUIRE_NOTHROW (ss_test << event1);
        ss_result << "ev_" << event1.id();
        REQUIRE( ss_test.str() == ss_result.str() );
    }
}

const dynaa::Event createEventWithNonPersistentType(shared_ptr<EntityTest> testEntity) {
    // This event type disappears when closing this function
    dynaa::EventType nonPersistentEventType = EventType::create("TYPE_NON_PERSISTENT", "");
    return testEntity->scheduleNow(nonPersistentEventType);
}

TEST_CASE( "Event memory management", "[event][stable]" ) {

    shared_ptr<EntityTest> testEntity = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType::create("TYPE_A", "Description A");

    SECTION( "Event types persistent in the core" ) {
        // Creates an event and destroys the reference to its type
        const dynaa::Event& event = createEventWithNonPersistentType(testEntity);
        // This event should still have a copy of the original (destroyed) type
        REQUIRE_NOTHROW( event.type() != eventTypeA );
        REQUIRE( event.type().name() == "TYPE_NON_PERSISTENT" );
        REQUIRE( event.type().description() == "");
    }

    SECTION( "Scheduling events should not retain ownership inside the core." ) {
        REQUIRE( testEntity.use_count() == 1 ); //Unique owner is this scope
        testEntity->scheduleNow(eventTypeA);
        REQUIRE( testEntity.use_count() == 1 );
        testEntity->scheduleAt(10, eventTypeA);
        REQUIRE( testEntity.use_count() == 1 );
        testEntity->scheduleAfter(20, eventTypeA);
        REQUIRE( testEntity.use_count() == 1 );
    }
}



