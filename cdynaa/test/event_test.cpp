#include "catch.h"
#include "../include/event.h"
#include "../include/eventtype.h"
#include "entitytest.h"

#include <string>
#include <iostream>

using namespace std;

TEST_CASE( "Event copy and assignment semantics", "[event][stable]" ) {

    // shared_ptr<EntityTest> testEntity = make_shared<EntityTest>();
    // const dynaa::EventType eventTypeA("TYPE_A", "Description A");
    // const dynaa::EventType eventTypeB("TYPE_B", "Description B");
    // const dynaa::Event event1 = Event(testEntity, eventTypeA);
    // const dynaa::Event event2 = testEntity->scheduleNow(eventTypeB);

    SECTION("Default construction") {
        Event event_1;
        REQUIRE_THROWS(event_1.source());
        REQUIRE(event_1.id() != Event::ANY_ID);
        REQUIRE(event_1.type() == Event::INVALID_EVENT_TYPE);

        Event event_2;
        REQUIRE(event_1 != event_2);
    }

    SECTION("Copy construction and assignment copy construction") {
        Event event_1;
        REQUIRE_THROWS(event_1.source());
        REQUIRE(event_1.id() != Event::ANY_ID);
        REQUIRE(event_1.type() == Event::INVALID_EVENT_TYPE);

        Event event_2(event_1);
        REQUIRE(event_1 == event_2);

        Event event_3 = event_1;
        REQUIRE(event_1 == event_3);

    }

    SECTION("Copy assignment") {
        Event event_1;
        Event event_2;
        REQUIRE(event_1 != event_2);

        REQUIRE_NOTHROW(event_2 = event_1);
        REQUIRE(event_1 == event_2);
    }

    SECTION("Move constructor and move assignment") {
        Event event_1;
        EventID id = event_1.id();
        Event event_moved(std::move(event_1));
        REQUIRE(event_moved.id() == id);
        Event event_2;
        event_2 = std::move(event_moved);
        REQUIRE(event_2.id() == id);
    }
}

//TODO: All the tests in here are NOT for Event anymore, they are all for event expressions.
//      Rewrite tests for events.

TEST_CASE( "Event creation and usage", "[event][stable]" ) {

    shared_ptr<EntityTest> testEntity = make_shared<EntityTest>();
    const dynaa::EventType eventTypeA("TYPE_A", "Description A");
    const dynaa::EventType eventTypeB("TYPE_B", "Description B");
    const dynaa::EventExpression& event1 = testEntity->scheduleNow(eventTypeA);
    const dynaa::EventExpression& event2 = testEntity->scheduleNow(eventTypeB);

    SECTION( "ID is unique to event and non-zero" ) {
        REQUIRE( event1.atomicEventID() != 0 );
        REQUIRE( event2.atomicEventID() != 0 );
        REQUIRE( event1.atomicEventID() != event2.atomicEventID() );
    }
    SECTION( "Source of event is set to entity that created it" ) {
        REQUIRE( event1.atomicEventSourceID() == testEntity->getUID() );
    }
    SECTION( "Type of event matches type it was instantiated with" ) {
        REQUIRE( event1.atomicEventType() == eventTypeA );
        REQUIRE( event1.atomicEventType() != event2.atomicEventType() );
    }
    SECTION( "Events are only equal if id, source and type all match" ) {
        REQUIRE( event1 == event1 );
        // event1 has same source as event2 but different type:
        REQUIRE( event1 != event2 );
        // event with same type as event1 but different source:
        const EventExpression& event3 = testEntity->scheduleNow(eventTypeA);
        REQUIRE( event1 != event3 );
    }
    SECTION( "Event objects can be copy assigned.  Copies are equal." ) {
        const dynaa::EventExpression event3 = event1; // Copy
        REQUIRE( &event3 != &event1 );
        REQUIRE( event3 == event1 );
        REQUIRE( event3 != event2 );
    }

}

TEST_CASE( "Presentation", "[event][stable]" ) {

    shared_ptr<EntityTest> testEntity = make_shared<EntityTest>();
    const dynaa::EventType eventTypeA("TYPE_A", "Description A");
    const dynaa::EventType eventTypeB("TYPE_B", "Description B");
    const dynaa::EventExpression& event1 = testEntity->scheduleNow(eventTypeA);
    const dynaa::EventExpression& event2 = testEntity->scheduleNow(eventTypeB);
    const dynaa::EventExpression event3 = event1; // Copy

    // FIXME: Makes no sense because we are not testing event expression prints.  We have to review all the event_test
    // to really work with events.
    // SECTION( "Presentation of event and event type as string" ) {
    //     std::ostringstream ss_test;
    //     REQUIRE_NOTHROW( ss_test << eventTypeB << endl );
    //     REQUIRE_NOTHROW( ss_test << event1 << endl);
    // }
    // SECTION( "Convert to string for printing" ) {
    //     std::ostringstream ss_test, ss_result;
    //     REQUIRE_NOTHROW (ss_test << event1);
    //     ss_result << "ev_" << event1.atomicEventID();
    //     REQUIRE( ss_test.str() == ss_result.str() );
    // }
}

const dynaa::EventExpression createEventWithNonPersistentType(shared_ptr<EntityTest> testEntity) {
    // This event type disappears when closing this function
    dynaa::EventType nonPersistentEventType("TYPE_NON_PERSISTENT", "");
    return testEntity->scheduleNow(nonPersistentEventType);
}

TEST_CASE( "Event memory management", "[event][stable]" ) {

    shared_ptr<EntityTest> testEntity = make_shared<EntityTest>();
    const dynaa::EventType eventTypeA("TYPE_A", "Description A");

    SECTION( "Event types persistent in the core" ) {
        // Creates an event and destroys the reference to its type
        const dynaa::EventExpression& event = createEventWithNonPersistentType(testEntity);
        // This event should still have a copy of the original (destroyed) type
        REQUIRE_NOTHROW( event.atomicEventType() != eventTypeA );
        REQUIRE( event.atomicEventType().name() == "TYPE_NON_PERSISTENT" );
        REQUIRE( event.atomicEventType().description() == "");
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



