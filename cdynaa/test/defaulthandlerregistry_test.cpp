#include "catch.h"
#include "entitytest.h"
#include "eventhandlertest.h"
#include "../include/defaulthandlerregistry.h"
#include "../include/event.h"

#include <string>
#include <iostream>
#include <functional>

using namespace std;

TEST_CASE( "DefaultHandlerRegistry register event handlers and manipulate them", "[hregistry][stable]" ) {
    // Setup event handlers
    shared_ptr<EventHandler> eventHandler1 = EventHandler::create(callback_function);
    shared_ptr<EventHandler> eventHandler2 = EventHandler::create(callback_function, "callback_function");
    shared_ptr<EventHandler> eventHandler2copy = eventHandler2; // Copy of eventHandlers
    shared_ptr<EventHandler> eventHandler3 = EventHandler::create(EventHandlerTest::callback_static_method,
            "callback_static_method");
    shared_ptr<EventHandlerTest> eventHandlerTest = make_shared<EventHandlerTest>();
    shared_ptr<EventHandler> eventHandler4 = EventHandler::create(
                std::bind(&EventHandlerTest::callback_method, eventHandlerTest.get(),
                          std::placeholders::_1),
                "callback_method", {eventHandlerTest} );
    REQUIRE( eventHandlerTest.use_count() == 1);
    REQUIRE( eventHandler1->getIdentifier() == "_" );
    REQUIRE( eventHandler2->getIdentifier() == "callback_function" );
    REQUIRE( eventHandler2copy->getIdentifier() == "callback_function");
    // TODO is below still relevant for shared pointers?
    //REQUIRE( &eventHandler2copy != &eventHandler2);
    REQUIRE( eventHandler3->getIdentifier() == "callback_static_method" );
    REQUIRE( eventHandler4->getIdentifier() == "callback_method");

    // Setup some events from one entity
    // Actually, this is a hack extrictly for the tests.
    // Exposing the schedule methods of an entity is really not desired.
    shared_ptr<EntityTest> entity = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType::create("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = EventType::create("TYPE_B", "Description B");
    const dynaa::Event& event1 = entity->scheduleNow(eventTypeA);
    const dynaa::Event& event2 = entity->scheduleNow(eventTypeA);
    const dynaa::Event& event3 = entity->scheduleNow(eventTypeB);
    // after the schedule operations above, this routine is still the only owner of entity.
    REQUIRE( entity.use_count() == 1);

    //TODO: Send these tests to a eventHandler test file
    EventHandlerTest::callbackStaticCounter = 0; // Resets this from previous tests
    callbackFunctionCounter = 0;
    REQUIRE_NOTHROW( eventHandler1->handle(event1) ); // Increments once callbackFunctionCounter
    REQUIRE_NOTHROW( eventHandler2->handle(event2) ); // Increments once callbackFunctionCounter
    REQUIRE_NOTHROW( eventHandler3->handle(event1) ); // Increments once EventHandlerTest::callbackStaticCounter
    REQUIRE_NOTHROW( eventHandler4->handle(event2) ); // Increments once the internal var of eventHandlerTest
    REQUIRE( callbackFunctionCounter == 2);
    REQUIRE( EventHandlerTest::callbackStaticCounter == 1);
    REQUIRE( eventHandlerTest->callbackMethodCounter == 1);


    // register some events
    dynaa::DefaultHandlerRegistry registry;
    REQUIRE_NOTHROW( registry.registerHandler(eventHandler1,
                     event1.source(),
                     event1.type(),
                     event1.id(),
                     false) );
    REQUIRE( registry.getNumberOfHandlers() == 1 );
    registry.registerHandler(eventHandler2,
                             event1.source(),
                             event1.type(),
                             event1.id(),
                             false);
    REQUIRE( registry.getNumberOfHandlers() == 2 );
    registry.registerHandler(eventHandler2,
                             event2.source(),
                             event2.type(),
                             event2.id(),
                             false);
    REQUIRE( registry.getNumberOfHandlers() == 3 );
    registry.registerHandler(eventHandler2,
                             Event::ANY_SOURCE,
                             event2.type(),
                             event2.id(),
                             false);
    REQUIRE( registry.getNumberOfHandlers() == 4 );
    REQUIRE( entity.use_count() == 1);
    REQUIRE( registry.getEventHandlersOf(event1).size() == 2 );
    REQUIRE( registry.getEventHandlersOf(event2).size() == 1 );
    REQUIRE( entity.use_count() == 1);

    // TODO check content correct

    SECTION ( "Deregister some event handlers" ) {
        REQUIRE(eventHandlerTest.use_count() == 1);
        REQUIRE( registry.getEventHandlersOf(event1).size() == 2 );
        REQUIRE( registry.getEventHandlersOf(event2).size() == 1 );
        REQUIRE_NOTHROW( registry.unregisterHandler(eventHandler1,
                         event1.source(),
                         event1.type(),
                         event1.id()) );
        REQUIRE( registry.getEventHandlersOf(event1).size() == 1 );
        REQUIRE_NOTHROW( registry.unregisterHandler(eventHandler2,
                         event2.source(),
                         event2.type(),
                         event2.id()) );
        REQUIRE( registry.getEventHandlersOf(event1).size() == 1 );
        // Only the handler associated with the specific event2 is deleted.
        // Same handler is also associated with a wildcard and survives the unregister of specific mask
        REQUIRE( registry.getEventHandlersOf(event2).size() == 1 );
        // Now it goes away
        REQUIRE_NOTHROW( registry.unregisterHandler(eventHandler2,
                         Event::ANY_SOURCE,
                         event2.type(),
                         Event::ANY_ID) );
        REQUIRE( registry.getEventHandlersOf(event2).size() == 0 );
        REQUIRE( entity.use_count() == 1);
        REQUIRE( registry.getNumberOfHandlers() == 0 );
    }

    SECTION ( "Deregister some event handlers with wildcards" ) {
        REQUIRE( registry.getEventHandlersOf(event1).size() == 2 );
        REQUIRE( registry.getEventHandlersOf(event2).size() == 1 );
        REQUIRE_NOTHROW( registry.unregisterHandler(eventHandler2,
                         dynaa::Event::ANY_SOURCE,
                         dynaa::Event::ANY_TYPE,
                         dynaa::Event::ANY_ID) );
        REQUIRE( registry.getEventHandlersOf(event1).size() == 1 );
        REQUIRE( registry.getEventHandlersOf(event2).size() == 0 );

        REQUIRE_NOTHROW( registry.unregisterHandler(eventHandler1,
                         dynaa::Event::ANY_SOURCE,
                         dynaa::Event::ANY_TYPE,
                         event2.id()) );
        REQUIRE( registry.getEventHandlersOf(event1).size() == 1 );
        REQUIRE( registry.getEventHandlersOf(event2).size() == 0 );

        REQUIRE_NOTHROW( registry.unregisterHandler(eventHandler1,
                         dynaa::Event::ANY_SOURCE,
                         dynaa::Event::ANY_TYPE,
                         event1.id()) );
        REQUIRE( registry.getEventHandlersOf(event1).size() == 0 );
        REQUIRE( registry.getEventHandlersOf(event2).size() == 0 );
        REQUIRE( entity.use_count() == 1);
        REQUIRE( registry.getNumberOfHandlers() == 0 );
    }
}
