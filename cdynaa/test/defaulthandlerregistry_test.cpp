#include "../include/defaulthandlerregistry.h"
#include "../include/event.h"
#include "catch.h"
#include "entitytest.h"
#include "eventhandlertest.h"

#include <functional>
#include <iostream>
#include <string>

using namespace std;

TEST_CASE("DefaultHandlerRegistry register event handlers and manipulate them", "[hregistry][stable]") {
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
                "callback_method", { eventHandlerTest });
    REQUIRE(eventHandlerTest.use_count() == 1);
    REQUIRE(eventHandler1->getIdentifier() == "_");
    REQUIRE(eventHandler2->getIdentifier() == "callback_function");
    REQUIRE(eventHandler2copy->getIdentifier() == "callback_function");
    // TODO is below still relevant for shared pointers?
    //REQUIRE( &eventHandler2copy != &eventHandler2);
    REQUIRE(eventHandler3->getIdentifier() == "callback_static_method");
    REQUIRE(eventHandler4->getIdentifier() == "callback_method");

    // Setup some events from one entity
    // Actually, this is a hack extrictly for the tests.
    // Exposing the schedule methods of an entity is really not desired.
    shared_ptr<EntityTest> entity = make_shared<EntityTest>();
    const dynaa::EventType eventTypeA("TYPE_A", "Description A");
    const dynaa::EventType eventTypeB("TYPE_B", "Description B");
    const dynaa::EventExpression& event1 = entity->scheduleNow(eventTypeA);
    const dynaa::EventExpression& event2 = entity->scheduleNow(eventTypeA);
    const dynaa::EventExpression& event3 = entity->scheduleNow(eventTypeB);
    HandlerReport report_event1(HandlerToken(),
                                EventExpression::any_event,
                                5,
                                true,
                                vector<HandlerReport::EventReport>()); // Expression in here is a dummy -- create it from report.

    // after the schedule operations above, this routine is still the only owner of entity.
    REQUIRE(entity.use_count() == 1);

    //TODO: Send these tests to a eventHandler test file
    EventHandlerTest::callbackStaticCounter = 0; // Resets this from previous tests
    callbackFunctionCounter = 0;
    REQUIRE_NOTHROW(eventHandler1->handle(report_event1)); // Increments once callbackFunctionCounter
    REQUIRE_NOTHROW(eventHandler2->handle(report_event1)); // Increments once callbackFunctionCounter
    REQUIRE_NOTHROW(eventHandler3->handle(report_event1)); // Increments once EventHandlerTest::callbackStaticCounter
    REQUIRE_NOTHROW(eventHandler4->handle(report_event1)); // Increments once the internal var of eventHandlerTest
    REQUIRE(callbackFunctionCounter == 2);
    REQUIRE(EventHandlerTest::callbackStaticCounter == 1);
    REQUIRE(eventHandlerTest->callbackMethodCounter == 1);

    // register some events
    dynaa::DefaultHandlerRegistry registry;
    REQUIRE_NOTHROW(registry.registerHandler(eventHandler1, event1, false));
    REQUIRE(registry.getNumberOfHandlers() == 1);
    REQUIRE(registry.getNumberOfHandlers() == 1);
    registry.registerHandler(eventHandler2, event1, false);
    REQUIRE(registry.getNumberOfHandlers() == 2);
    REQUIRE(registry.getNumberOfHandlers() == 2);
    registry.registerHandler(eventHandler2, event2, false);
    REQUIRE(registry.getNumberOfHandlers() == 3);
    registry.registerHandler(eventHandler2, EventExpression(Event::ANY_SOURCE,
                             event2.atomicEventType(),
                             event2.atomicEventID()),
                             false);
    REQUIRE(registry.getNumberOfHandlers() == 4);
    REQUIRE(entity.use_count() == 1);
    // Cannot make this test anymore because we cannot access events directly.
    // REQUIRE(registry.getEventHandlersOf(event1).size() == 2);
    // REQUIRE(registry.getEventHandlersOf(event2).size() == 2);
    REQUIRE(entity.use_count() == 1);

    // TODO check content correct

//     SECTION("Deregister some event handlers") {
//         REQUIRE(eventHandlerTest.use_count() == 1);
//         REQUIRE_NOTHROW(registry.unregisterHandler(eventHandler1,
//                         event1.source(),
//                         event1.type(),
//                         event1.id()));
//         REQUIRE(registry.getEventHandlersOf(event1).size() == 1);
//         REQUIRE_NOTHROW(registry.unregisterHandler(eventHandler2,
//                         event2.source(),
//                         event2.type(),
//                         event2.id()));
//         REQUIRE(registry.getEventHandlersOf(event1).size() == 1);
//         // Only the handler associated with the specific event2 is deleted.
//         // Same handler is also associated with a wildcard and survives the unregister of specific mask
//         REQUIRE(registry.getEventHandlersOf(event2).size() == 1);
//         // Now it goes away
//         REQUIRE_NOTHROW(registry.unregisterHandler(eventHandler2));
//         REQUIRE(registry.getEventHandlersOf(event2).size() == 0);
//         REQUIRE(entity.use_count() == 1);
//         REQUIRE(registry.getNumberOfHandlers() == 0);
//     }

//     SECTION("Deregister some event handlers with wildcards") {
//         REQUIRE_NOTHROW(registry.unregisterHandler(eventHandler2));
//         REQUIRE(registry.getEventHandlersOf(event1).size() == 1);
//         REQUIRE(registry.getEventHandlersOf(event2).size() == 0);

//         REQUIRE_NOTHROW(registry.unregisterHandler(eventHandler1,
//                         dynaa::Event::ANY_SOURCE,
//                         dynaa::Event::ANY_TYPE,
//                         event2.id()));
//         REQUIRE(registry.getEventHandlersOf(event1).size() == 1);
//         REQUIRE(registry.getEventHandlersOf(event2).size() == 0);

//         REQUIRE_NOTHROW(registry.unregisterHandler(eventHandler1,
//                         dynaa::Event::ANY_SOURCE,
//                         dynaa::Event::ANY_TYPE,
//                         event1.id()));
//         REQUIRE(registry.getEventHandlersOf(event1).size() == 1);
//         REQUIRE(registry.getEventHandlersOf(event2).size() == 0);
//         REQUIRE(entity.use_count() == 1);
//         REQUIRE(registry.getNumberOfHandlers() == 1);
//         REQUIRE_NOTHROW(registry.unregisterHandler(eventHandler1,
//                         event1.source(),
//                         event1.type(),
//                         event1.id()));
//         REQUIRE(registry.getEventHandlersOf(event1).size() == 0);
//         REQUIRE(registry.getEventHandlersOf(event2).size() == 0);
//         REQUIRE(entity.use_count() == 1);
//         REQUIRE(registry.getNumberOfHandlers() == 0);
//     }
}

TEST_CASE("Redesign: DefaultHandlerRegistry excepts on registering invalid event handlers", "[hregistry][stable]") {
    // Invalid event Handler
    dynaa::DefaultHandlerRegistry registry;
    shared_ptr<EventHandler> invalidHandler;
    HandlerToken token_1;

    REQUIRE_THROWS(token_1 = registry.registerHandler(invalidHandler,
                             EventExpression(Event::ANY_SOURCE, Event::ANY_TYPE, Event::ANY_ID), false));
}

TEST_CASE("Redesign: DefaultHandlerRegistry register event handlers and manipulate them", "[hregistry][stable]") {
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
                "callback_method", { eventHandlerTest });

    // Setup some events from one entity
    // Actually, this is a hack extrictly for the tests.
    // Exposing the schedule methods of an entity is really not desired.
    shared_ptr<EntityTest> entity = make_shared<EntityTest>();
    const dynaa::EventType eventTypeA("TYPE_A", "Description A");
    const dynaa::EventType eventTypeB("TYPE_B", "Description B");
    const dynaa::EventExpression& event1 = entity->scheduleNow(eventTypeA);
    const dynaa::EventExpression& event2 = entity->scheduleNow(eventTypeA);
    const dynaa::EventExpression& event3 = entity->scheduleNow(eventTypeB);
    // after the schedule operations above, this routine is still the only owner of entity.
    REQUIRE(entity.use_count() == 1);

    // //TODO: Send these tests to a eventHandler test file
    // EventHandlerTest::callbackStaticCounter = 0; // Resets this from previous tests
    // callbackFunctionCounter = 0;
    // REQUIRE_NOTHROW( eventHandler1->handle(event1) ); // Increments once callbackFunctionCounter
    // REQUIRE_NOTHROW( eventHandler2->handle(event2) ); // Increments once callbackFunctionCounter
    // REQUIRE_NOTHROW( eventHandler3->handle(event1) ); // Increments once EventHandlerTest::callbackStaticCounter
    // REQUIRE_NOTHROW( eventHandler4->handle(event2) ); // Increments once the internal var of eventHandlerTest
    // REQUIRE( callbackFunctionCounter == 2);
    // REQUIRE( EventHandlerTest::callbackStaticCounter == 1);
    // REQUIRE( eventHandlerTest->callbackMethodCounter == 1);
    // Creates a registry
    dynaa::DefaultHandlerRegistry registry;

    SECTION("Register some event handlers") {
        // Register 1
        HandlerToken token_1;
        REQUIRE_NOTHROW(token_1 = registry.registerHandler(eventHandler1, event1, false));
        REQUIRE(registry.getNumberOfHandlers() == 1);
        // Register 2
        HandlerToken token_2;
        REQUIRE_NOTHROW(token_2 = registry.registerHandler(eventHandler2, event1, false));
        REQUIRE(registry.getNumberOfHandlers() == 2);
        REQUIRE(token_1 != token_2);
        // Register 3
        HandlerToken token_3;
        REQUIRE_NOTHROW(token_3 = registry.registerHandler(eventHandler2, event2, false));
        REQUIRE(registry.getNumberOfHandlers() == 3);
        REQUIRE(token_1 != token_3);
        REQUIRE(token_2 != token_3);
        // Register 4
        REQUIRE_NOTHROW(registry.registerHandler(eventHandler2,
                        EventExpression(Event::ANY_SOURCE, event2.atomicEventType(), event2.atomicEventID()), false));
        REQUIRE(registry.getNumberOfHandlers() == 4);
        // Register 5
        HandlerToken token_5;
        REQUIRE_NOTHROW(token_5 = registry.registerHandler(eventHandler2, EventExpression(event2) || event1, false));
        REQUIRE(token_5 != HandlerToken::INVALID_TOKEN);
        REQUIRE(registry.getNumberOfHandlers() == 5);
        REQUIRE(token_1 != token_5);
        REQUIRE(token_2 != token_5);
        REQUIRE(token_3 != token_5);
        // Register 6
        REQUIRE_NOTHROW(registry.registerHandler(eventHandler1, event1, false));
        REQUIRE(registry.getNumberOfHandlers() == 6);
        // Register 7
        REQUIRE_NOTHROW(registry.registerHandler(eventHandler1, EventExpression(Event::ANY_SOURCE, Event::ANY_TYPE,
                        Event::ANY_ID), false));
        REQUIRE(registry.getNumberOfHandlers() == 7);

        REQUIRE(entity.use_count() == 1);
        //FIXME: Cannot use getEventHandlers anymore.  Think about other way to test it.
        //REQUIRE(registry.getEventHandlersOf(event1).size() == 5); // Registers 1,2,5,6,7
        //REQUIRE(registry.getEventHandlersOf(event2).size() == 4); // Registers 3,4,5,7
    }

    //FIXME: getEventHandlersOf do not work with expressions.  Think about other way to make the test
    // SECTION ( "Deregister some event handlers" ) {
    //     // Register some handlers, get their tokens
    //     HandlerToken token_1 = registry.registerHandler(eventHandler1, event1, false);
    //     HandlerToken token_2 = registry.registerHandler(eventHandler2, event1, false);
    //     HandlerToken token_3 = registry.registerHandler(eventHandler2, event2, false);
    //     REQUIRE(registry.getNumberOfHandlers() == 3);
    //     REQUIRE( registry.getEventHandlersOf(event1).size() == 2 );
    //     REQUIRE( registry.getEventHandlersOf(event2).size() == 1 );
    //     REQUIRE_NOTHROW( registry.unregisterHandler(token_1) );
    //     REQUIRE(registry.getNumberOfHandlers() == 2);
    //     REQUIRE( registry.getEventHandlersOf(event1).size() == 1 );
    //     REQUIRE( registry.getEventHandlersOf(event2).size() == 1 );
    //     REQUIRE_NOTHROW( registry.unregisterHandler(token_2));
    //     REQUIRE(registry.getNumberOfHandlers() == 1);
    //     REQUIRE( registry.getEventHandlersOf(event1).size() == 0 );
    //     REQUIRE( registry.getEventHandlersOf(event2).size() == 1 );
    //     auto tag = *(registry.getEventHandlersOf(event2).begin());
    //     REQUIRE( tag == token_3);
    //     REQUIRE_NOTHROW( registry.unregisterHandler(token_3));
    //     REQUIRE(registry.getNumberOfHandlers() == 0);
    //     REQUIRE( registry.getEventHandlersOf(event2).size() == 0 );
    // }

    // FIXME: Cannot make use of getEventHandlers without events.  Solve it.
    // SECTION ("Clean all event handlers") {
    //     HandlerToken token_1 = registry.registerHandler(eventHandler1, event1, false);
    //     HandlerToken token_2 = registry.registerHandler(eventHandler2, event1, false);
    //     HandlerToken token_3 = registry.registerHandler(eventHandler2, event2, false);
    //     registry.clean();
    //     REQUIRE(registry.getNumberOfHandlers() == 3);
    //     REQUIRE(registry.getEventHandlersOf(event1).size() == 2 );
    //     REQUIRE(registry.getEventHandlersOf(event2).size() == 1 );
    //     REQUIRE_NOTHROW( registry.unregisterHandler(token_1) );
    //     REQUIRE_NOTHROW( registry.unregisterHandler(token_2));
    //     REQUIRE_NOTHROW( registry.unregisterHandler(token_3));
    //     REQUIRE_NOTHROW(registry.clean());
    //     // REQUIRE(registry.getNumberOfHandlers_() == 0);
    // }

    // SECTION ( "Deregister some event handlers with wildcards" ) {
    //     REQUIRE( registry.getEventHandlersOf(event1).size() == 2 );
    //     REQUIRE( registry.getEventHandlersOf(event2).size() == 1 );
    //     REQUIRE_NOTHROW( registry.unregisterHandler(eventHandler2,
    //                      dynaa::Event::ANY_SOURCE,
    //                      dynaa::Event::ANY_TYPE,
    //                      dynaa::Event::ANY_ID) );
    //     REQUIRE( registry.getEventHandlersOf(event1).size() == 1 );
    //     REQUIRE( registry.getEventHandlersOf(event2).size() == 0 );

    //     REQUIRE_NOTHROW( registry.unregisterHandler(eventHandler1,
    //                      dynaa::Event::ANY_SOURCE,
    //                      dynaa::Event::ANY_TYPE,
    //                      event2.id()) );
    //     REQUIRE( registry.getEventHandlersOf(event1).size() == 1 );
    //     REQUIRE( registry.getEventHandlersOf(event2).size() == 0 );

    //     REQUIRE_NOTHROW( registry.unregisterHandler(eventHandler1,
    //                      dynaa::Event::ANY_SOURCE,
    //                      dynaa::Event::ANY_TYPE,
    //                      event1.id()) );
    //     REQUIRE( registry.getEventHandlersOf(event1).size() == 0 );
    //     REQUIRE( registry.getEventHandlersOf(event2).size() == 0 );
    //     REQUIRE( entity.use_count() == 1);
    //     REQUIRE( registry.getNumberOfHandlers() == 0 );
    // }
}

TEST_CASE("Redesign: DefaultHandlerRegistry returns handlers (tokens) according priority levels",
          "[hregistry][stable]") {
    // Setup event handlers
    shared_ptr<EventHandler> eventHandler1 = EventHandler::create(callback_function, EventHandler::HIGHEST_PRIORITY_LEVEL);
    shared_ptr<EventHandler> eventHandler2 = EventHandler::create(callback_function, EventHandler::LOWEST_PRIORITY_LEVEL);
    shared_ptr<EventHandler> eventHandler3 = EventHandler::create(callback_function, EventHandler::DEFAULT_PRIORITY_LEVEL);

    // Setup some events from one entity
    // Actually, this is a hack extrictly for the tests.
    // Exposing the schedule methods of an entity is really not desired.
    shared_ptr<EntityTest> entity = make_shared<EntityTest>();
    const dynaa::EventType eventTypeA("TYPE_A", "Description A");
    const dynaa::EventExpression& event1 = entity->scheduleNow(eventTypeA);

    dynaa::DefaultHandlerRegistry registry;

    SECTION ("Retrieval respects priority order") {
        HandlerToken token_1 = registry.registerHandler(eventHandler1, EventExpression::any_event, false);
        HandlerToken token_2 = registry.registerHandler(eventHandler2, EventExpression::any_event, false);
        HandlerToken token_3 = registry.registerHandler(eventHandler3, EventExpression::any_event, false);
        REQUIRE(registry.getNumberOfHandlers() == 3);
        REQUIRE(token_1 != token_2);
        REQUIRE(token_1 != token_3);
        REQUIRE(token_2 != token_3);
        REQUIRE(eventHandler1->getPriorityLevel() < eventHandler3->getPriorityLevel());
        REQUIRE(eventHandler3->getPriorityLevel() < eventHandler2->getPriorityLevel());

        // FIXME:  Cannot make this test anymore as event1 is not an event anymore.
        // std::set<HandlerToken> handlers = registry.getEventHandlersOf(event1);
        // REQUIRE(handlers.size() == 3 );
        // auto handlers_it = handlers.begin();
        // REQUIRE(*handlers_it == token_1);
        // handlers_it++;
        // REQUIRE(*handlers_it == token_3);
        // handlers_it++;
        // REQUIRE(*handlers_it == token_2);
    }

}