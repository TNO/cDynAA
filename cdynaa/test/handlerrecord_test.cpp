#include "catch.h"
#include "../include/eventhandler.h"
#include "../include/eventtype.h"
#include "../include/handlertoken.h"
#include "../include/handlerrecord.h"
#include "../include/eventexpression.h"
#include "entitytest.h"



#include <iostream>
#include <string>

using namespace std;

// TODO:Send this template to a stdutils.h file
template <class T>
inline bool contains(const std::vector<T> &vec, const T &value) {
    return std::find(vec.begin(), vec.end(), value) != vec.end();
};


/** Test cases for HandlerRecord and HandlerToken
 */

TEST_CASE("HandlerRecord creation", "[handlerrecord][stable]") {

    const EventType eventTypeA("TYPE_A", "Description A");
    auto sourceEntity = make_shared<Entity>();
    EventExpression ev_expr("namedExpression", sourceEntity, eventTypeA, Event::ANY_ID);
    shared_ptr<EventHandler> eventHandler = make_shared<EventHandler>(EntityTest::callback);

    SECTION("Creation of handler records") {
        HandlerRecord record(ev_expr, eventHandler);
        REQUIRE_NOTHROW(make_shared<HandlerRecord>(ev_expr, eventHandler));
    }
}

TEST_CASE("Basic operations with handler records", "[handlerrecord][stable]") {

    const EventType eventTypeA("TYPE_A", "Description A");
    auto sourceEntity = make_shared<Entity>();
    EventExpression ev_expr("namedExpression", sourceEntity, eventTypeA, Event::ANY_ID);
    EventMask ev_expr_mask = EventMask::generateMask(ev_expr.atomicEventSourceID(),
                             ev_expr.atomicEventType(), ev_expr.atomicEventID());
    EventExpression ev_expr2(sourceEntity, Event::ANY_TYPE, 5);
    EventMask ev_expr_mask2 = EventMask::generateMask(ev_expr2.atomicEventSourceID(),
                              ev_expr2.atomicEventType(), ev_expr2.atomicEventID());
    auto or_combined_ev_expr = ev_expr || ev_expr2;
    auto and_combined_ev_expr = ev_expr && ev_expr2;

    shared_ptr<EventHandler> eventHandler = make_shared<EventHandler>(EntityTest::callback);
    HandlerRecord record1(ev_expr, eventHandler);
    HandlerRecord record2(or_combined_ev_expr, eventHandler);
    HandlerRecord record3(and_combined_ev_expr, eventHandler);

    SECTION("Retrieval of direct information") {
        REQUIRE(record1.getUID() != record2.getUID());

        REQUIRE(ev_expr == record1.getEventExpression());
        REQUIRE(!record1.getEventMasks().empty());
        REQUIRE(eventHandler == record1.getEventHandler());

        REQUIRE(or_combined_ev_expr == record2.getEventExpression());
        REQUIRE(!record2.getEventMasks().empty());
        REQUIRE(eventHandler == record2.getEventHandler());

        REQUIRE(and_combined_ev_expr == record3.getEventExpression());
        REQUIRE(!record3.getEventMasks().empty());
        REQUIRE(eventHandler == record3.getEventHandler());
    }

    SECTION("Retrieval of mask information on the record") {
        const std::vector<EventMask> &masks1 = record1.getEventMasks();
        REQUIRE(masks1.size() == 1);
        REQUIRE(contains(masks1, ev_expr_mask));

        const std::vector<EventMask> &masks2 = record2.getEventMasks();
        REQUIRE(masks2.size() == 2);
        REQUIRE(contains(masks2, ev_expr_mask));
        REQUIRE(contains(masks2, ev_expr_mask2));

        const std::vector<EventMask> &masks3 = record3.getEventMasks();
        REQUIRE(masks3.size() == 2);
        REQUIRE(contains(masks3, ev_expr_mask));
        REQUIRE(contains(masks3, ev_expr_mask2));
    }
}

TEST_CASE("Updating a record with events", "[handlerrecord][stable]") {
    const EventType eventTypeA("TYPE_A", "Description A");
    const EventType eventTypeB("TYPE_B", "Description B");
    auto sourceEntity = make_shared<EntityTest>();
    const dynaa::EventExpression& event1 = sourceEntity->scheduleNow(eventTypeA);
    const dynaa::EventExpression& event2 = sourceEntity->scheduleNow(eventTypeB);
    shared_ptr<EventHandler> eventHandler = make_shared<EventHandler>(EntityTest::callback);
    SimTime testTime = 967583;

    // Create event expressions in some formats
    EventExpression a_specific_event(event2);
    EventExpression all_typeA(Event::ANY_SOURCE, eventTypeA, Event::ANY_ID);
    EventExpression any_event(Event::ANY_SOURCE, Event::ANY_TYPE, Event::ANY_ID);

    auto or_combined_ev_expr = a_specific_event || all_typeA;
    auto and_combined_ev_expr = a_specific_event && all_typeA;
    auto and_or_combined_ev_expr = or_combined_ev_expr && and_combined_ev_expr;
    auto or_and_combined_ev_expr = and_combined_ev_expr || (a_specific_event && any_event);

    // Records for test expressions
    HandlerRecord record_specific_event(a_specific_event, eventHandler);
    HandlerRecord record_or(or_combined_ev_expr, eventHandler);
    HandlerRecord record_and(and_combined_ev_expr, eventHandler);
    HandlerRecord record_and_or(and_or_combined_ev_expr, eventHandler);
    HandlerRecord record_or_and(or_and_combined_ev_expr, eventHandler);

    // const dynaa::EventType eventTypeA("TYPE_A", "Description A");
    // const dynaa::EventType eventTypeB("TYPE_B", "Description B");
    // const dynaa::Event& event1 = testEntity.scheduleNow(eventTypeA);
    // const dynaa::Event& event2 = testEntity.scheduleNow(eventTypeB);

    // FIXME:  We do not have access to events directly anymore.  Find another way to make this test
    // SECTION("Updating records with atomic expressions") {    // SECTION("Updating records with simple AND (a && b) expressions") {
    //     REQUIRE(!record_and.triggered());
    //     // event2 triggers only one side of the expression (specific_event)
    //     record_and.update(event2, testTime);
    //     REQUIRE(!record_and.triggered());
    //     // event1 triggers the second side of the expression
    //     record_and.update(event1, testTime);
    //     REQUIRE(record_and.triggered());

    //     // TODO:Inspection of values in final reportage
    // }

    // SECTION("Updating records with simple OR (a || b) expressions") {
    //     REQUIRE(!record_or.triggered());
    //     record_or.update(event2, testTime);
    //     REQUIRE(record_or.triggered());
    //     record_or.reset();
    //     REQUIRE(!record_or.triggered());
    //     record_or.update(event1, testTime);
    //     REQUIRE(record_or.triggered());

    //     // TODO:Inspection of values in final reportage
    // }

    // SECTION("Updating records with complex expressions") {
    //     REQUIRE(!record_and_or.triggered());
    //     // updating with valid event registers the triggering
    //     // Event of type (a || b) && (a && b) -- a triggered -- expression does not fire
    //     record_and_or.update(event2, testTime);
    //     REQUIRE(!record_and_or.triggered());
    //     record_and_or.update(event1, testTime);
    //     REQUIRE(record_and_or.triggered());

    //     REQUIRE(!record_or_and.triggered());
    //     // event1 happens triggering two masks (ev1 && allA) || (ev1 && any) -- record is not triggered yet
    //     // note that reporting event1 should trigger ev1 and allA
    //     record_or_and.update(event1, testTime);
    //     REQUIRE(!record_or_and.triggered());
    //     // event2 happens triggering one mask but affecting both sides (ev1 && allA) || (ev1 && any)
    //     record_or_and.update(event2, testTime);
    //     REQUIRE(record_or_and.triggered());


    //     // TODO:Inspection of values in final reportage
    // }
    //     REQUIRE(!record_specific_event.triggered());
    //     // updating with event not covered in expression changes nothing
    //     record_specific_event.update(event1, testTime);
    //     REQUIRE(!record_specific_event.triggered());
    //     // updating with valid event registers the triggering
    //     record_specific_event.update(event2, testTime);
    //     REQUIRE(record_specific_event.triggered());

    //     // TODO:Inspection of values in final reportage
    // }

    // SECTION("Updating records with simple AND (a && b) expressions") {
    //     REQUIRE(!record_and.triggered());
    //     // event2 triggers only one side of the expression (specific_event)
    //     record_and.update(event2, testTime);
    //     REQUIRE(!record_and.triggered());
    //     // event1 triggers the second side of the expression
    //     record_and.update(event1, testTime);
    //     REQUIRE(record_and.triggered());

    //     // TODO:Inspection of values in final reportage
    // }

    // SECTION("Updating records with simple OR (a || b) expressions") {
    //     REQUIRE(!record_or.triggered());
    //     record_or.update(event2, testTime);
    //     REQUIRE(record_or.triggered());
    //     record_or.reset();
    //     REQUIRE(!record_or.triggered());
    //     record_or.update(event1, testTime);
    //     REQUIRE(record_or.triggered());

    //     // TODO:Inspection of values in final reportage
    // }

    // SECTION("Updating records with complex expressions") {
    //     REQUIRE(!record_and_or.triggered());
    //     // updating with valid event registers the triggering
    //     // Event of type (a || b) && (a && b) -- a triggered -- expression does not fire
    //     record_and_or.update(event2, testTime);
    //     REQUIRE(!record_and_or.triggered());
    //     record_and_or.update(event1, testTime);
    //     REQUIRE(record_and_or.triggered());

    //     REQUIRE(!record_or_and.triggered());
    //     // event1 happens triggering two masks (ev1 && allA) || (ev1 && any) -- record is not triggered yet
    //     // note that reporting event1 should trigger ev1 and allA
    //     record_or_and.update(event1, testTime);
    //     REQUIRE(!record_or_and.triggered());
    //     // event2 happens triggering one mask but affecting both sides (ev1 && allA) || (ev1 && any)
    //     record_or_and.update(event2, testTime);
    //     REQUIRE(record_or_and.triggered());


    //     // TODO:Inspection of values in final reportage
    // }

}

TEST_CASE("HandlerToken creation and default operations","[handlertoken][stable]") {

    const EventType eventTypeA("TYPE_A", "Description A");
    auto sourceEntity = make_shared<Entity>();
    EventExpression ev_expr("namedExpression", sourceEntity, eventTypeA, Event::ANY_ID);
    shared_ptr<EventHandler> eventHandler = make_shared<EventHandler>(EntityTest::callback);

    SECTION("Creation of HandlerToken") {
        HandlerToken invalid_token; // Default created tokens are all alike and equals to an invalid token.
        HandlerToken also_invalid_token;
        REQUIRE(invalid_token == also_invalid_token);
        REQUIRE(!(invalid_token != also_invalid_token));

        HandlerRecord record(ev_expr, eventHandler);
        HandlerToken record_token = record.getUID();
        REQUIRE(record_token == record.getUID());
        REQUIRE(record_token != invalid_token);

        // HandlerRecords are (copy) assignable
        HandlerToken dummy;
        REQUIRE(dummy == invalid_token);
        dummy = record_token;
        REQUIRE(dummy == record.getUID());
    }
}


