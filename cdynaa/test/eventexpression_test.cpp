#include "catch.h"
#include "../include/eventexpression.h"
#include "../include/eventtype.h"
#include "entitytest.h"
#include <cmath>
#include <string>
#include <iostream>
#include <memory>
#include <set>

using namespace std;
using namespace dynaa;

TEST_CASE( "EventExpression creation and comparisons", "[eventexpression][stable]" ) {
    const EventType &eventTypeA = EventType::create("TYPE_A", "Description A");
    auto sourceEntity = make_shared<EntityTest>();
    auto waitingEntity = make_shared<EntityTest>();
    auto nullExprHandler = make_shared<ExpressionHandler>();

    SECTION( "Creation of atomic EventExpression objects" ) {
        EventExpression ev_expr(sourceEntity, eventTypeA, 5);
        REQUIRE( ev_expr.type() == Atomic);
        REQUIRE( ev_expr.value() == false);
        REQUIRE( ev_expr.atomicEventType() == eventTypeA);
        REQUIRE( ev_expr.atomicEventID() == 5);
        REQUIRE( ev_expr.atomicEventSource().lock() == sourceEntity);
        REQUIRE( ev_expr.triggeredEvent() == nullptr);
        REQUIRE( isnan(ev_expr.triggeredTime()));
        EventExpression ev_expr2;
        REQUIRE( ev_expr2.type() == Atomic);
        REQUIRE( ev_expr2.value() == false);
        REQUIRE( ev_expr2.atomicEventType() == Event::ANY_TYPE);
        REQUIRE( ev_expr2.atomicEventID() == Event::ANY_ID);
        REQUIRE( ev_expr2.atomicEventSource().lock() == Event::ANY_SOURCE);
        REQUIRE( ev_expr.triggeredEvent() == nullptr);
        REQUIRE( isnan(ev_expr.triggeredTime()));
    }

    SECTION( "[shared_ptr] Creation of atomic EventExpression objects" ) {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        REQUIRE( ev_expr->type() == Atomic);
        REQUIRE( ev_expr->value() == false);
        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>();
        REQUIRE( ev_expr2->type() == Atomic);
        REQUIRE( ev_expr2->value() == false);
        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>(*ev_expr);
        REQUIRE( ev_expr3->type() == Atomic);
        REQUIRE( ev_expr3->value() == false);
        REQUIRE( *ev_expr3 == *ev_expr );
    }

    SECTION( "Creation by operators") {
        EventExpression ev_expr(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr2(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr3(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression and_combined = ev_expr && ev_expr2;
        REQUIRE( and_combined.type() == AND);
        REQUIRE( and_combined.value() == false);
        REQUIRE( and_combined.firstTerm() == ev_expr);
        REQUIRE( and_combined.secondTerm() == ev_expr2);
        REQUIRE( and_combined.atomicEventType() == Event::ANY_TYPE);
        REQUIRE( and_combined.atomicEventID() == Event::ANY_ID);
        REQUIRE( and_combined.atomicEventSource().lock() == Event::ANY_SOURCE);
        REQUIRE( and_combined.triggeredEvent() == nullptr);
        REQUIRE( isnan(and_combined.triggeredTime()));
        EventExpression and_combined2 = ev_expr && ev_expr2 && ev_expr3;
        REQUIRE(and_combined2.type() == AND);
        REQUIRE(and_combined2.value() == false);
        EventExpression or_combined = ev_expr || ev_expr2 || ev_expr3;
        REQUIRE(or_combined.type() == OR);
        REQUIRE(or_combined.value() == false);
        REQUIRE(or_combined.firstTerm().type() == OR);
        REQUIRE(or_combined.firstTerm().firstTerm() == ev_expr);
        REQUIRE(or_combined.firstTerm().secondTerm() == ev_expr2);
        REQUIRE(or_combined.secondTerm() == ev_expr3);
        EventExpression and_or_combined = ev_expr || ev_expr2 && ev_expr3;
        REQUIRE(and_or_combined.type() == OR);
        REQUIRE(and_or_combined.value() == false);
        REQUIRE(and_or_combined.firstTerm() == ev_expr);
        REQUIRE(and_or_combined.secondTerm().type() == AND);
        REQUIRE(and_or_combined.secondTerm().firstTerm() == ev_expr2);
        REQUIRE(and_or_combined.secondTerm().secondTerm() == ev_expr3);
        EventExpression and_or_combined2 = (ev_expr || ev_expr2) && ev_expr3;
        REQUIRE(and_or_combined2.type() == AND);
        REQUIRE(and_or_combined2.value() == false);
        REQUIRE(and_or_combined2.secondTerm() == ev_expr3);
        REQUIRE(and_or_combined2.firstTerm().type() == OR);
    }

    SECTION( "[shared_ptr] Creation by operators") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> and_combined = ev_expr->logicalAnd(ev_expr2);
        REQUIRE( and_combined->type() == AND);
        REQUIRE( and_combined->value() == false);
        REQUIRE( *and_combined->firstTermAsPtr() == *ev_expr); // Comparison of pointers
        REQUIRE( *and_combined->secondTermAsPtr() == *ev_expr2); // Comparison of pointers
        shared_ptr<EventExpression> and_combined2 = ev_expr->logicalAnd(ev_expr2)->logicalAnd(ev_expr3);
        REQUIRE(and_combined2->type() == AND);
        REQUIRE(and_combined2->value() == false);
        REQUIRE(*and_combined2->firstTermAsPtr() == and_combined2->firstTerm());
        shared_ptr<EventExpression> or_combined = (ev_expr->logicalOr(ev_expr2))->logicalOr(ev_expr3);
        REQUIRE(or_combined->type() == OR);
        REQUIRE(or_combined->value() == false);
        REQUIRE(or_combined->firstTermAsPtr()->type() == OR);
        REQUIRE(*or_combined->firstTermAsPtr()->firstTermAsPtr() == *ev_expr);
        REQUIRE(*or_combined->firstTermAsPtr()->secondTermAsPtr() == *ev_expr2);
        REQUIRE(*or_combined->secondTermAsPtr() == *ev_expr3);
        REQUIRE(*or_combined->firstTermAsPtr() == or_combined->firstTerm());
        REQUIRE(*or_combined->secondTermAsPtr() == or_combined->secondTerm());
        // NOTE can also create shared pointer managed event expressions with boolean operators:
        shared_ptr<EventExpression> and_or_combined = make_shared<EventExpression>(*ev_expr || *ev_expr2 && *ev_expr3);
        REQUIRE(and_or_combined->type() == OR);
        REQUIRE(and_or_combined->value() == false);
        REQUIRE(*and_or_combined->firstTermAsPtr() == *ev_expr);
        REQUIRE(and_or_combined->secondTermAsPtr()->type() == AND);
        REQUIRE(*and_or_combined->secondTermAsPtr()->firstTermAsPtr() == *ev_expr2);
        REQUIRE(*and_or_combined->secondTermAsPtr()->secondTermAsPtr() == *ev_expr3);
        shared_ptr<EventExpression> and_or_combined2 = make_shared<EventExpression>(
            (*ev_expr || *ev_expr2) && *ev_expr3);
        REQUIRE(and_or_combined2->type() == AND);
        REQUIRE(and_or_combined2->value() == false);
        REQUIRE(*and_or_combined2->secondTermAsPtr() == *ev_expr3);
        REQUIRE(and_or_combined2->firstTermAsPtr()->type() == OR);
    }
}

TEST_CASE( "[shared_ptr] EventExpression activation during simulaton runs", "[eventexpression][stable]") {
    const dynaa::EventType &eventTypeA = dynaa::EventType::create("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = dynaa::EventType::create("TYPE_B", "Description B");
    auto sourceEntity = make_shared<EntityTest>();
    auto sourceEntity2 = make_shared<EntityTest>();
    auto waitingEntity = make_shared<EntityTest>();
    auto nullExprHandler = make_shared<ExpressionHandler>();
    DynAASim &simEngine = DynAASim::getInstance();
    simEngine.reset();

    SECTION( "[shared_ptr] Registration and activation of single EventExpression objects") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>(Event::ANY_SOURCE, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>(sourceEntity2, eventTypeA, Event::ANY_ID);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, ev_expr) );
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, ev_expr2) );
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, ev_expr3) );
        Event event = sourceEntity->scheduleAt(1, eventTypeA);
        REQUIRE(ev_expr->value() == false);
        REQUIRE(ev_expr2->value() == false);
        REQUIRE(ev_expr3->value() == false);
        simEngine.run(2); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(ev_expr->value() == true);
        REQUIRE(ev_expr2->value() == true);
        REQUIRE(ev_expr3->value() == false); // remains false as the expected source is not valid.
        REQUIRE(ev_expr->triggeredEvent()->id() == event.id());
        REQUIRE(ev_expr->triggeredTime() == Approx(1));
    }

    SECTION( "[shared_ptr] Registration and activation of single EventExpression objects -- further event match cases") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, ev_expr) );
        sourceEntity2->scheduleAt(1,
                                  eventTypeA); // Issues correct event type but other source, should not trigger the expression
        sourceEntity->scheduleAt(1,
                                 eventTypeB); // Issues from correct source, but other event type. Should not trigger the expression
        REQUIRE(ev_expr->value() == false);
        simEngine.run(2); // Issue events by advancing time... expression should not be activated
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(ev_expr->value() == false);
    }

    SECTION( "[shared_ptr] Registration and activation of AND combined - 2 terms expressions") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>(sourceEntity2, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> and_combined = ev_expr->logicalAnd(ev_expr2);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, and_combined) );
        Event event1 = sourceEntity2->scheduleAt(1, eventTypeA); // When issued activates part of the expression.
        Event event2 = sourceEntity->scheduleAt(3, eventTypeA); // When issued activates the second part of the expression.
        REQUIRE(and_combined->value() == false);
        REQUIRE(and_combined->firstTerm().value() == false);
        REQUIRE(and_combined->firstTermAsPtr()->value() == false);
        REQUIRE(and_combined->secondTerm().value() == false);
        REQUIRE(and_combined->secondTermAsPtr()->value() == false);
        REQUIRE(and_combined->firstTerm().triggeredEvent() == nullptr);
        REQUIRE(and_combined->secondTerm().triggeredEvent() == nullptr);
        REQUIRE(isnan(and_combined->triggeredTime()));
        simEngine.run(2); // Advances time, only one event is issued triggering only one term of the expression.
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(and_combined->value() == false);
        REQUIRE(and_combined->firstTerm().value() == false);
        REQUIRE(and_combined->secondTerm().value() == true);
        REQUIRE(and_combined->firstTermAsPtr()->value() == false);
        REQUIRE(and_combined->secondTermAsPtr()->value() == true);
        REQUIRE(and_combined->firstTerm().triggeredEvent() == nullptr);
        REQUIRE(and_combined->secondTerm().triggeredEvent()->id() == event1.id());
        REQUIRE(isnan(and_combined->triggeredTime()));
        simEngine.run(4); // Advances time, second event is issued triggering the second term of the expression.
        REQUIRE(simEngine.getCurrentTime() == Approx(4));
        REQUIRE(and_combined->value() == true);
        REQUIRE(and_combined->firstTerm().value() == true);
        REQUIRE(and_combined->secondTerm().value() == true);
        REQUIRE(and_combined->firstTermAsPtr()->value() == true);
        REQUIRE(and_combined->secondTermAsPtr()->value() == true);
        REQUIRE(and_combined->firstTerm().triggeredEvent()->id() == event2.id());
        REQUIRE(and_combined->secondTerm().triggeredEvent()->id() == event1.id());
        REQUIRE(and_combined->triggeredTime() == Approx(3));
        // Below expressions false because they weren't waiting:
        REQUIRE(ev_expr->value() == false);
        REQUIRE(ev_expr2->value() == false);
    }

    SECTION( "[shared_ptr] Registration and activation of OR combined EventExpression objects - 2 terms expressions") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>(sourceEntity2, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> or_combined = ev_expr->logicalOr(ev_expr2);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, or_combined) );
        Event event1 =sourceEntity2->scheduleAt(1, eventTypeA); // When issued activates part of the expression.
        Event event2 =sourceEntity->scheduleAt(3, eventTypeA); // When issued activates the second part of the expression.
        REQUIRE(or_combined->value() == false);
        REQUIRE(or_combined->firstTerm().value() == false);
        REQUIRE(or_combined->secondTerm().value() == false);
        REQUIRE(or_combined->firstTerm().triggeredEvent() == nullptr);
        REQUIRE(or_combined->secondTerm().triggeredEvent() == nullptr);
        REQUIRE(isnan(or_combined->triggeredTime()));
        simEngine.run(2); // Advances time, only one event is issued triggering only one term of the expression.
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(or_combined->value() == true); // As it is an OR expression, it should be already active.
        REQUIRE(or_combined->firstTerm().value() == false);
        REQUIRE(or_combined->secondTerm().value() == true);
        REQUIRE(or_combined->firstTerm().triggeredEvent() == nullptr);
        REQUIRE(or_combined->secondTerm().triggeredEvent()->id() == event1.id());
        REQUIRE(or_combined->triggeredTime() == Approx(1));
        // Below expressions false because they weren't waiting:
        REQUIRE(ev_expr->value() == false);
        REQUIRE(ev_expr2->value() == false);
    }

    SECTION( "Registration and activation of AND/OR combined EventExpression objects - 3 terms expressions") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>(sourceEntity2, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>(Event::ANY_SOURCE, eventTypeB, Event::ANY_ID);
        shared_ptr<EventExpression> and_or_combined = (ev_expr->logicalAnd(ev_expr2))->logicalOr(ev_expr3);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, and_or_combined) );
        sourceEntity2->scheduleAt(1, eventTypeA); // When issued activates part of the expression.
        sourceEntity->scheduleAt(3, eventTypeB); // When issued activates the second part of the expression.
        REQUIRE(and_or_combined->value() == false);
        simEngine.run(2); // Advances time, only one event is issued triggering only one term of the expression.
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(and_or_combined->value() == false); // Not yet active
        REQUIRE(isnan(and_or_combined->triggeredTime()));
        simEngine.run(4);
        REQUIRE(simEngine.getCurrentTime() == Approx(4));
        REQUIRE(and_or_combined->value() == true); // Active by the ev_expr3 in the OR branch.
        REQUIRE(and_or_combined->firstTerm().firstTerm().value() == false); // Term that should not yet be active
        REQUIRE(and_or_combined->triggeredTime() == Approx(3));
    }
}

TEST_CASE( "[shared_ptr] EventExpression automatic triggering of callback functions", "[eventexpression][stable]") {
    const dynaa::EventType &eventTypeA = dynaa::EventType::create("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = dynaa::EventType::create("TYPE_B", "Description B");
    auto sourceEntity = make_shared<EntityTest>();
    auto sourceEntity2 = make_shared<EntityTest>();
    auto sourceEntity3 = make_shared<EntityTest>();
    auto waitingEntity = make_shared<EntityTest>();
    DynAASim &simEngine = DynAASim::getInstance();
    simEngine.reset();

    SECTION( "Registration and activation of single EventExpression objects") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>(sourceEntity2, eventTypeA, Event::ANY_ID);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity->simpleExprHandler, ev_expr));
        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity2->simpleExprHandler, ev_expr3));
        sourceEntity->scheduleAt(1,eventTypeA);
        REQUIRE(!(sourceEntity->m_check));
        REQUIRE(!(sourceEntity2->m_check));
        sourceEntity->m_counter = 0;
        sourceEntity2->m_counter = 0;
        simEngine.run(2); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(sourceEntity->m_check);
        REQUIRE(sourceEntity->m_counter == 1);
        REQUIRE(!(sourceEntity2->m_check));
    }

    SECTION( "Continuous waiting of single EventExpression objects") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>(sourceEntity2, eventTypeA, Event::ANY_ID);
        REQUIRE(simEngine.getCurrentTime() == 0);
        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity->simpleExprHandler, ev_expr));
        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity2->simpleExprHandler, ev_expr3));
        int N = 3;
        for (int i = 0; i < N; i++) {
            sourceEntity->scheduleAt(i + 1,eventTypeA);
        }
        REQUIRE(!(sourceEntity->m_check));
        REQUIRE(!(sourceEntity2->m_check));
        sourceEntity->m_counter = 0;
        sourceEntity2->m_counter = 0;
        simEngine.run(4); // Activates the expression via event
        REQUIRE(sourceEntity->m_check);
        REQUIRE(sourceEntity->m_counter == N);
        REQUIRE(!(sourceEntity2->m_check));
    }

    SECTION( "Multiple ExpressionHandlers for the same EventExpression") {
        shared_ptr<EventExpression> ev_expr1 = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        // This also tests having multiple handlers
        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity->simpleExprHandler, ev_expr1));
        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity2->simpleExprHandler, ev_expr1));
        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity3->simpleExprHandler, ev_expr1));
        int N = 3.;
        double timestep = 10.;
        for (int i = 1; i <= N; i++){
            sourceEntity->scheduleAt(i * timestep, eventTypeA);
        }
        REQUIRE(sourceEntity->m_counter == 0);
        REQUIRE(sourceEntity2->m_counter == 0);
        REQUIRE(sourceEntity3->m_counter == 0);
        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr1).size() == 3);
        simEngine.run(timestep + 1);
        REQUIRE(sourceEntity->m_counter == 1);
        REQUIRE(sourceEntity2->m_counter == 1);
        REQUIRE(sourceEntity3->m_counter == 1);
        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr1).size() == 2);
        // Override a continuous wait with wait once:
        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity2->simpleExprHandler, ev_expr1));
        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr1).size() == 2);
        simEngine.run(2 * timestep + 1);
        REQUIRE(sourceEntity->m_counter == 2);
        REQUIRE(sourceEntity2->m_counter == 2);
        REQUIRE(sourceEntity3->m_counter == 1);
        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr1).size() == 1);
        simEngine.run(N * timestep + 1);
        REQUIRE(sourceEntity->m_counter == N);
        REQUIRE(sourceEntity2->m_counter == 2);
        REQUIRE(sourceEntity3->m_counter == 1);
        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr1).size() == 1);
    }

    SECTION( "Dismissing of EventExpression objects") {
        shared_ptr<EventExpression> ev_expr1 = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>(sourceEntity2, eventTypeB, Event::ANY_ID);
        // This also tests having multiple handlers
        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity->simpleExprHandler, ev_expr1));
        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity2->simpleExprHandler, ev_expr1));
        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity->simpleExprHandler, ev_expr2));
        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity2->simpleExprHandler, ev_expr2));
        std::vector<std::unique_ptr<Event>> triggered_events = {};
        double timestep = 10.;
        triggered_events.push_back(std::unique_ptr<Event>(new Event(sourceEntity->scheduleAt(timestep, eventTypeA))));
        triggered_events.push_back(std::unique_ptr<Event>(new Event(sourceEntity2->scheduleAt(timestep, eventTypeB))));
        REQUIRE(sourceEntity->m_triggeredEvents.size() == 0);
        REQUIRE(sourceEntity2->m_triggeredEvents.size() == 0);
        simEngine.run(timestep + 1); // Activates the expression via event
        // Check triggered events captured as expected (before dismissals)
        REQUIRE(sourceEntity->m_triggeredEvents.size() == 2);
        REQUIRE(sourceEntity2->m_triggeredEvents.size() == 2);
        REQUIRE(std::equal(triggered_events.begin(), triggered_events.end(),
                           sourceEntity->m_triggeredEvents.begin(),
                           [](const std::unique_ptr<Event>& a, const std::unique_ptr<Event>& b){ return *a == *b; }));
        REQUIRE(std::equal(triggered_events.begin(), triggered_events.end(),
                           sourceEntity2->m_triggeredEvents.begin(),
                           [](const std::unique_ptr<Event>& a, const std::unique_ptr<Event>& b){ return *a == *b; }));
        // Now dismiss handler1 for expr1, and handler2 for all expressions
        waitingEntity->dismiss(sourceEntity->simpleExprHandler, ev_expr1);
        waitingEntity->dismiss(sourceEntity2->simpleExprHandler);
        sourceEntity->m_triggeredEvents.clear();
        sourceEntity2->m_triggeredEvents.clear();
        timestep += 10.;
        sourceEntity->scheduleAt(timestep, eventTypeA);
        Event event2 = sourceEntity2->scheduleAt(timestep, eventTypeB);
        simEngine.run(timestep + 1); // Activates the expression via event
        // Check only handler1 triggered on expr2
        REQUIRE(sourceEntity->m_triggeredEvents.size() == 1);
        REQUIRE(sourceEntity2->m_triggeredEvents.size() == 0);
        REQUIRE(*sourceEntity->m_triggeredEvents[0] == event2);
    }

    SECTION( "Nested waiting for EventExpression objects") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity->nestedExprHandler, ev_expr));
        sourceEntity->scheduleAfter(1,eventTypeA);
        sourceEntity->m_counter = 0;
        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr).size() == 1);
        simEngine.run(2); // Activates the expression via event, counter should go to 1
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(sourceEntity->m_counter == 1);
        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr).size() == 1);
        sourceEntity->scheduleAfter(1,eventTypeA);
        sourceEntity->m_check = false;
        simEngine.run(4);
        REQUIRE(simEngine.getCurrentTime() == Approx(4));
        REQUIRE(sourceEntity->m_counter == 2);
        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr).size() == 1);
        sourceEntity->scheduleAfter(1,eventTypeA);
        sourceEntity->m_check = false;
        simEngine.run(6);
        REQUIRE(simEngine.getCurrentTime() == Approx(6));
        REQUIRE(sourceEntity->m_counter == 3);
        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr).size() == 1);
    }

    SECTION( "Registration and activation of combined EventExpression objects") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>(sourceEntity2, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>(sourceEntity2, eventTypeB, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr4 = make_shared<EventExpression>(sourceEntity, eventTypeB, Event::ANY_ID);
        shared_ptr<EventExpression> combined = make_shared<EventExpression>(*ev_expr && *ev_expr2 || *ev_expr3 && *ev_expr4);
        shared_ptr<EventExpression> combined2 = make_shared<EventExpression>(*ev_expr || *ev_expr3);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity->simpleExprHandler, combined));
        long numHandlers1 = simEngine.getDiagnosticInfo().find("num_reg_handlers")->second;
        REQUIRE(numHandlers1 > 0); // should be at least one handler (implementation agnostic)
        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity2->simpleExprHandler, combined2));
        long numHandlersTot = simEngine.getDiagnosticInfo().find("num_reg_handlers")->second;
        REQUIRE(numHandlersTot > numHandlers1); // should be at least two handlers (implementation agnostic)
        REQUIRE(!(sourceEntity->m_check));
        REQUIRE(!(sourceEntity2->m_check));
        sourceEntity->scheduleAt(1,eventTypeA); // will activate ev_expr only
        sourceEntity2->scheduleAt(3,eventTypeB); // will ativate ev_expr3 only
        sourceEntity->scheduleAt(5,eventTypeB); // will ativate ev_expr4 only and the combined expression
        sourceEntity->m_counter = 0;
        simEngine.run(2); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(!(sourceEntity->m_check)); // Not yet.
        REQUIRE(isnan(combined->triggeredTime()));
        REQUIRE(sourceEntity2->m_check); // Yes, should be active
        REQUIRE(combined2->triggeredTime() == Approx(1));
        REQUIRE(sourceEntity2->m_counter == 1);
        // Should be back to number of handlers for first wait call:
        REQUIRE(simEngine.getDiagnosticInfo().find("num_reg_handlers")->second == numHandlers1);
        simEngine.run(4); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(4));
        REQUIRE(!(sourceEntity->m_check)); // Not yet.
        REQUIRE(isnan(combined->triggeredTime()));
        REQUIRE(sourceEntity2->m_counter == 1);  // second expression in OR shouldn't call callback again
        REQUIRE(combined2->triggeredTime() == Approx(1));
        simEngine.run(6); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(6));
        REQUIRE(sourceEntity->m_check); // Yes, should be active
        REQUIRE(combined->triggeredTime() == Approx(5));
        REQUIRE(sourceEntity->m_counter == 1);
        // Check no more handlers registered:
        REQUIRE(simEngine.getDiagnosticInfo().find("num_reg_handlers")->second == 0);
    }

    SECTION( "Continuous waiting of combined EventExpression objects") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>(sourceEntity2, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>(sourceEntity2, eventTypeB, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr4 = make_shared<EventExpression>(sourceEntity, eventTypeB, Event::ANY_ID);
        shared_ptr<EventExpression> combined = make_shared<EventExpression>(*ev_expr && *ev_expr2 || *ev_expr3 && *ev_expr4);
        shared_ptr<EventExpression> combined2 = make_shared<EventExpression>(*ev_expr || *ev_expr3);
        REQUIRE(simEngine.getCurrentTime() == 0);
        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity->simpleExprHandler, combined));
        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity2->simpleExprHandler, combined2));
        REQUIRE(!(sourceEntity->m_check));
        REQUIRE(!(sourceEntity2->m_check));
        sourceEntity->scheduleAt(1,eventTypeA); // will activate ev_expr only
        sourceEntity2->scheduleAt(3,eventTypeB); // will ativate ev_expr3 only
        sourceEntity->scheduleAt(5,eventTypeB); // will ativate ev_expr4 only and the combined expression
        sourceEntity2->scheduleAt(6,eventTypeA); // will ativate ev_expr2 only
        sourceEntity->scheduleAt(9,eventTypeA); // will activate ev_expr only and the combined expression
        sourceEntity->m_counter = 0;
        simEngine.run(2); // Activates the expression via event
        REQUIRE(!(sourceEntity->m_check)); // Not yet.
        REQUIRE(sourceEntity2->m_check); // Yes, should be active (never reset)
        REQUIRE(isnan(combined2->triggeredTime()));  // reprimed, so again NaN
        //REQUIRE(sourceEntity2->m_triggeredTimes.back() == Approx(1));
        REQUIRE(sourceEntity2->m_counter == 1);
        simEngine.run(4); // Activates the expression via event
        REQUIRE(!(sourceEntity->m_check)); // Not yet.
        REQUIRE(sourceEntity2->m_counter == 2);  // activated again
        REQUIRE(isnan(combined2->triggeredTime()));  // reprimed, so again NaN
        //REQUIRE(sourceEntity2->m_triggeredTimes.back() == Approx(3));
        simEngine.run(6); // Activates the expression via event
        REQUIRE(sourceEntity->m_check); // Yes, should be active
        REQUIRE(sourceEntity->m_counter == 1);
        REQUIRE(sourceEntity2->m_counter == 2);  // as before
        REQUIRE(isnan(combined2->triggeredTime()));  // as before
        //REQUIRE(sourceEntity2->m_triggeredTimes.back() == Approx(3));  // as before
        simEngine.run(7);
        REQUIRE(sourceEntity->m_counter == 1); // as before
        REQUIRE(sourceEntity2->m_counter == 2);  // as before
        simEngine.run(10);
        REQUIRE(sourceEntity->m_counter == 2); // + 1
        REQUIRE(sourceEntity2->m_counter == 3);  // + 1
        REQUIRE(isnan(combined2->triggeredTime()));
        //REQUIRE(sourceEntity2->m_triggeredTimes.back() == Approx(9));
    }

    SECTION( "Nested waits in the callbacks of combined EventExpression objects") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>(sourceEntity2, eventTypeB, Event::ANY_ID);
        shared_ptr<EventExpression> combined2 = make_shared<EventExpression>(*ev_expr || *ev_expr3);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity2->nestedExprHandler, combined2));
        sourceEntity2->scheduleAt(1,eventTypeB); // will activate ev_expr3 only
        sourceEntity2->scheduleAt(3,eventTypeB); // will ativate ev_expr3 only
        sourceEntity2->scheduleAt(5,eventTypeB); // will ativate ev_expr3 only
        sourceEntity->scheduleAt(7,eventTypeA); // will ativate ev_expr only
        sourceEntity2->scheduleAt(9,eventTypeB); // will activate ev_expr3 only
        sourceEntity2->m_counter = 0;
        simEngine.run(2); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(sourceEntity2->m_counter == 1);
        simEngine.run(4); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(4));
        REQUIRE(sourceEntity2->m_counter ==
                2);  // second expression in OR and because it was re-enabled in the callback it should count 2
        simEngine.run(6); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(6));
        REQUIRE(sourceEntity2->m_counter == 3);
        simEngine.run(8); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(8));
        REQUIRE(sourceEntity2->m_counter == 4);
        simEngine.run(10); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(10));
        REQUIRE(sourceEntity2->m_counter == 5);
    }

    SECTION( "Nested waits in the callbacks of AND-OR combined EventExpression objects") {
        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>(sourceEntity, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>(sourceEntity2, eventTypeA, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>(sourceEntity2, eventTypeB, Event::ANY_ID);
        shared_ptr<EventExpression> ev_expr4 = make_shared<EventExpression>(sourceEntity, eventTypeB, Event::ANY_ID);
        shared_ptr<EventExpression> combined = make_shared<EventExpression>(*ev_expr && *ev_expr2 || *ev_expr3 && *ev_expr4);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity->nestedExprHandler, combined));
        sourceEntity->scheduleAt(1,eventTypeA); // will activate ev_expr only
        sourceEntity2->scheduleAt(1,eventTypeA); // will ativate ev_expr2 only
        sourceEntity->scheduleAt(3,eventTypeB); // will ativate ev_expr4 only and the combined expression
        sourceEntity2->scheduleAt(5,eventTypeB); // will ativate ev_expr3 only
        sourceEntity->scheduleAt(7,eventTypeA); // will activate ev_expr only
        sourceEntity2->scheduleAt(7,eventTypeA); // will ativate ev_expr2 only
        sourceEntity->m_counter = 0;
        simEngine.run(2); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(sourceEntity->m_counter == 1);
        simEngine.run(4); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(4));
        REQUIRE(sourceEntity->m_counter == 1); // Not yet.
        simEngine.run(6); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(6));
        REQUIRE(sourceEntity->m_counter == 2); // Yes, should be active
        simEngine.run(8); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(8));
        REQUIRE(sourceEntity->m_counter == 3);  // Combined and combined 2: both affected.
    }
}

// TODO below tests could be removed as don't test much new..
/*******************
   From here on, we basically duplicate part of the tests above, but without using
   shared_ptr as the way to manage the initial lifetime cycle of event expressions.

   Both strategies are available in cDynAA, so both of them should pass throught the same set of tests.
*******************/
TEST_CASE( "EventExpression activation during simulaton runs", "[eventexpression][stable]") {
    const dynaa::EventType &eventTypeA = dynaa::EventType::create("TYPE_A", "Description A");
    const dynaa::EventType &eventTypeB = dynaa::EventType::create("TYPE_B", "Description B");
    auto sourceEntity = make_shared<EntityTest>();
    auto sourceEntity2 = make_shared<EntityTest>();
    auto waitingEntity = make_shared<EntityTest>();
    auto nullExprHandler = make_shared<ExpressionHandler>();
    DynAASim &simEngine = DynAASim::getInstance();
    simEngine.reset();

    SECTION( "Registration and activation of single EventExpression objects") {
        EventExpression ev_expr(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr2(Event::ANY_SOURCE, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr3(sourceEntity2, eventTypeA, Event::ANY_ID);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        shared_ptr<EventExpression> sp_ev_expr = make_shared<EventExpression>(ev_expr);
        shared_ptr<EventExpression> sp_ev_expr2 = make_shared<EventExpression>(ev_expr2);
        shared_ptr<EventExpression> sp_ev_expr3 = make_shared<EventExpression>(ev_expr3);
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, sp_ev_expr) );
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, sp_ev_expr2) );
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, sp_ev_expr3) );
        sourceEntity->scheduleAt(1,eventTypeA);
        REQUIRE(ev_expr.value() == false);
        REQUIRE(ev_expr2.value() == false);
        REQUIRE(ev_expr3.value() == false);
        simEngine.run(2); // Activates the expression via event
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(sp_ev_expr->value() == true);
        REQUIRE(sp_ev_expr2->value() == true);
        REQUIRE(sp_ev_expr3->value() == false); // remains false as the expected source is not valid.
        // These remain untriggered copies:
        REQUIRE(ev_expr.value() == false);
        REQUIRE(ev_expr2.value() == false);
        REQUIRE(ev_expr3.value() == false);
    }

    SECTION( "Registration and activation of single EventExpression objects -- further event match cases") {
        EventExpression ev_expr(sourceEntity, eventTypeA, Event::ANY_ID);
        REQUIRE(simEngine.getCurrentTime() == 0);
        shared_ptr<EventExpression> sp_ev_expr = make_shared<EventExpression>(ev_expr);
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, sp_ev_expr) );
        //REQUIRE_NOTHROW( ev_expr.wait() );
        sourceEntity2->scheduleAt(1,
                                  eventTypeA); // Issues correct event type but other source, should not trigger the expression
        sourceEntity->scheduleAt(1,
                                 eventTypeB); // Issues from correct source, but other event type. Should not trigger the expression
        REQUIRE(ev_expr.value() == false);
        simEngine.run(2); // Issue events by advancing time... expression should not be activated
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(ev_expr.value() == false);
    }

    SECTION( "Registration and activation of AND combined EventExpression objects - 2 terms expressions") {
        EventExpression ev_expr(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr2(sourceEntity2, eventTypeA, Event::ANY_ID);
        EventExpression and_combined = ev_expr && ev_expr2;
        REQUIRE(simEngine.getCurrentTime() == 0);
        shared_ptr<EventExpression> sp_and_combined = make_shared<EventExpression>(and_combined);
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, sp_and_combined) );
        sourceEntity2->scheduleAt(1, eventTypeA); // When issued activates part of the expression.
        sourceEntity->scheduleAt(3, eventTypeA); // When issued activates the second part of the expression.
        REQUIRE(sp_and_combined->value() == false);
        REQUIRE(sp_and_combined->firstTerm().value() == false);
        REQUIRE(sp_and_combined->secondTerm().value() == false);
        simEngine.run(2); // Advances time, only one event is issued triggering only one term of the expression.
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(sp_and_combined->value() == false);
        REQUIRE(sp_and_combined->firstTerm().value() == false);
        REQUIRE(sp_and_combined->secondTerm().value() == true);
        simEngine.run(4); // Advances time, second event is issued triggering the second term of the expression.
        REQUIRE(simEngine.getCurrentTime() == Approx(4));
        REQUIRE(sp_and_combined->value() == true);
        REQUIRE(sp_and_combined->firstTerm().value() == true);
        REQUIRE(sp_and_combined->secondTerm().value() == true);
        // Below expressions false because they weren't waiting:
        REQUIRE(ev_expr.value() == false);
        REQUIRE(ev_expr2.value() == false);
    }

    SECTION( "Registration and activation of OR combined EventExpression objects - 2 terms expressions") {
        EventExpression ev_expr(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr2(sourceEntity2, eventTypeA, Event::ANY_ID);
        EventExpression or_combined = ev_expr || ev_expr2;
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        shared_ptr<EventExpression> sp_or_combined = make_shared<EventExpression>(or_combined);
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, sp_or_combined) );
        //REQUIRE_NOTHROW( or_combined.wait() );
        sourceEntity2->scheduleAt(1, eventTypeA); // When issued activates part of the expression.
        sourceEntity->scheduleAt(3, eventTypeA); // When issued activates the second part of the expression.
        REQUIRE(sp_or_combined->value() == false);
        REQUIRE(sp_or_combined->firstTerm().value() == false);
        REQUIRE(sp_or_combined->secondTerm().value() == false);
        simEngine.run(2); // Advances time, only one event is issued triggering only one term of the expression.
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(sp_or_combined->value() == true); // As it is an OR expression, it should be already active.
        REQUIRE(sp_or_combined->firstTerm().value() == false);
        REQUIRE(sp_or_combined->secondTerm().value() == true);
        // Below expressions false because they weren't waiting:
        REQUIRE(ev_expr.value() == false);
        REQUIRE(ev_expr2.value() == false);
    }

    SECTION( "Registration and activation of AND/OR combined EventExpression objects - 3 terms expressions") {
        EventExpression ev_expr(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr2(sourceEntity2, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr3(Event::ANY_SOURCE, eventTypeB, Event::ANY_ID);
        EventExpression and_or_combined = ev_expr && ev_expr2 || ev_expr3;
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        shared_ptr<EventExpression> sp_and_or_combined = make_shared<EventExpression>(and_or_combined);
        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, sp_and_or_combined) );
        sourceEntity2->scheduleAt(1, eventTypeA); // When issued activates part of the expression.
        sourceEntity->scheduleAt(3, eventTypeB); // When issued activates the second part of the expression.
        REQUIRE(sp_and_or_combined->value() == false);
        simEngine.run(2); // Advances time, only one event is issued triggering only one term of the expression.
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
        REQUIRE(sp_and_or_combined->value() == false); // Not yet active
        simEngine.run(4);
        REQUIRE(simEngine.getCurrentTime() == Approx(4));
        REQUIRE(sp_and_or_combined->value() == true); // Active by the ev_expr3 in the OR branch.
        REQUIRE(sp_and_or_combined->firstTerm().firstTerm().value() == false); // Term that should not yet be active
    }
}
