#include "catch.h"
#include "../include/eventexpression.h"
#include "../include/eventtype.h"
#include "../include/cythonutils.h"
#include "entitytest.h"
#include <cmath>
#include <string>
#include <iostream>
#include <memory>
#include <set>

using namespace std;
using namespace dynaa;

TEST_CASE( "EventExpressions copy and assignment semantics", "[event][stable]" ) {

    SECTION("Default construction") {
        EventExpression event_expression;
        REQUIRE(event_expression.atomicEventID() == Event::ANY_ID);
        REQUIRE(event_expression.atomicEventType() == Event::ANY_TYPE);
        REQUIRE(event_expression.atomicEventSource().lock() == Event::ANY_SOURCE);
        REQUIRE(event_expression.atomicEventSourceID() == Event::ANY_SOURCE_ID);

        EventExpression event_expression2;
        REQUIRE(event_expression == event_expression2);
    }

    SECTION("Copy construction and assignment copy construction") {
        EventType TEST_TYPE;
        EventExpression event_expression(Event::ANY_SOURCE, TEST_TYPE, 1);
        EventExpression copy_expression(event_expression);
        REQUIRE(event_expression == copy_expression);
        REQUIRE(copy_expression.atomicEventID() == 1);
        REQUIRE(copy_expression.atomicEventType() == TEST_TYPE);
        REQUIRE(copy_expression.atomicEventSource().lock() == Event::ANY_SOURCE);
        REQUIRE(&event_expression != &copy_expression);

        EventExpression copy_expression_2 = copy_expression;
        REQUIRE(event_expression == copy_expression_2);
        REQUIRE(&event_expression != &copy_expression_2);
        REQUIRE(&copy_expression != &copy_expression_2);
    }

    SECTION("Copy assignment") {
        EventType TEST_TYPE;
        EventExpression event_expression1(Event::ANY_SOURCE, TEST_TYPE, 1);
        EventExpression event_expression2;
        REQUIRE(event_expression1 != event_expression2);
        REQUIRE_NOTHROW(event_expression2 = event_expression1);
        REQUIRE(event_expression1 == event_expression2);
    }

    SECTION("Move constructor and move assignment") {
        EventType TEST_TYPE;
        EventExpression event_expression(Event::ANY_SOURCE, TEST_TYPE, 1);
        EventID id = event_expression.atomicEventID();
        EventExpression expression_moved;
        expression_moved = std::move(event_expression);
        REQUIRE(expression_moved.atomicEventID() == id);

        EventExpression event_expression2(Event::ANY_SOURCE, TEST_TYPE, 1);
        EventExpression expression_moved2(std::move(event_expression2));
        REQUIRE(expression_moved.atomicEventID() == 1);
    }
}

TEST_CASE("EventExpression creation and comparisons", "[eventexpression][stable]") {
    DynAASim &simEngine = DynAASim::getInstance();
    simEngine.reset();
    const EventType eventTypeA("TYPE_A", "Description A");
    auto sourceEntity = make_shared<Entity>();
    auto entityTest = make_shared<EntityTest>();
    EventExpression testEvent = entityTest->scheduleAt(1, eventTypeA);

    SECTION("Creation of atomic EventExpression objects") {
        EventExpression ev_expr(sourceEntity, eventTypeA, 5);
        REQUIRE(ev_expr.type() == Atomic);
        REQUIRE(ev_expr.atomicEventType() == eventTypeA);
        REQUIRE(ev_expr.atomicEventID() == 5);
        REQUIRE(ev_expr.atomicEventSource().lock() == sourceEntity);
        REQUIRE(ev_expr.atomicEventSourceID() == sourceEntity->getUID());
        REQUIRE(ev_expr.firstTerm() == ev_expr);
        REQUIRE(ev_expr.secondTerm() == ev_expr);
        REQUIRE(ev_expr.descriptor().empty());

        EventExpression ev_expr2;
        REQUIRE(ev_expr2.type() == Atomic);
        REQUIRE(ev_expr2.atomicEventType() == Event::ANY_TYPE);
        REQUIRE(ev_expr2.atomicEventID() == Event::ANY_ID);
        REQUIRE(ev_expr2.atomicEventSource().lock() == Event::ANY_SOURCE);
        REQUIRE(ev_expr2.atomicEventSourceID() == Event::ANY_SOURCE->getUID());
        REQUIRE(ev_expr2.firstTerm() == ev_expr2);
        REQUIRE(ev_expr2.secondTerm() == ev_expr2);
        REQUIRE(ev_expr2.descriptor().empty());

        EventExpression ev_expr3("namedExpression", sourceEntity, eventTypeA, 5);
        REQUIRE(ev_expr3.type() == Atomic);
        REQUIRE(ev_expr3.atomicEventType() == eventTypeA);
        REQUIRE(ev_expr3.atomicEventID() == 5);
        REQUIRE(ev_expr3.atomicEventSource().lock() == sourceEntity);
        REQUIRE(ev_expr3.atomicEventSourceID() == sourceEntity->getUID());
        REQUIRE(ev_expr3.firstTerm() == ev_expr);
        REQUIRE(ev_expr3.secondTerm() == ev_expr);
        REQUIRE(ev_expr3.descriptor() == "namedExpression");

        EventExpression ev_expr4(testEvent);
        REQUIRE(ev_expr4.type() == Atomic);
        REQUIRE(ev_expr4 == testEvent);
    }

    SECTION("Creation by operators") {
        EventExpression ev_expr("namedExpression", sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr2(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr3(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression and_combined = ev_expr && ev_expr2;
        REQUIRE(and_combined.type() == AND);
        REQUIRE(and_combined.firstTerm() == ev_expr);
        REQUIRE(and_combined.firstTerm().descriptor() == "namedExpression");
        REQUIRE(and_combined.secondTerm() == ev_expr2);
        REQUIRE(and_combined.atomicEventType() == Event::ANY_TYPE);
        REQUIRE(and_combined.atomicEventID() == Event::ANY_ID);
        REQUIRE(and_combined.atomicEventSource().lock() == Event::ANY_SOURCE);
        REQUIRE(and_combined.atomicEventSourceID() == Event::ANY_SOURCE->getUID());
        REQUIRE(and_combined.descriptor().empty());

        EventExpression and_combined2 = ev_expr && ev_expr2 && ev_expr3;
        REQUIRE(and_combined2.type() == AND);
        REQUIRE(and_combined2.firstTerm().type() == AND);
        REQUIRE(and_combined2.firstTerm().firstTerm() == ev_expr);
        REQUIRE(and_combined2.firstTerm().secondTerm() == ev_expr2);
        REQUIRE(and_combined2.secondTerm() == ev_expr3);
        REQUIRE(and_combined2.atomicEventType() == Event::ANY_TYPE);
        REQUIRE(and_combined2.atomicEventID() == Event::ANY_ID);
        REQUIRE(and_combined2.atomicEventSource().lock() == Event::ANY_SOURCE);
        REQUIRE(and_combined2.atomicEventSourceID() == Event::ANY_SOURCE->getUID());
        REQUIRE(and_combined2.descriptor().empty());

        EventExpression or_combined = ev_expr || ev_expr2 || ev_expr3;
        REQUIRE(or_combined.type() == OR);
        REQUIRE(or_combined.firstTerm().type() == OR);
        REQUIRE(or_combined.firstTerm().firstTerm() == ev_expr);
        REQUIRE(or_combined.firstTerm().secondTerm() == ev_expr2);
        REQUIRE(or_combined.secondTerm() == ev_expr3);
        REQUIRE(or_combined.atomicEventSourceID() == Event::ANY_SOURCE->getUID());
        REQUIRE(or_combined.descriptor().empty());

        EventExpression and_or_combined = ev_expr || ev_expr2 && ev_expr3;
        REQUIRE(and_or_combined.type() == OR);
        REQUIRE(and_or_combined.firstTerm() == ev_expr);
        REQUIRE(and_or_combined.secondTerm().type() == AND);
        REQUIRE(and_or_combined.secondTerm().firstTerm() == ev_expr2);
        REQUIRE(and_or_combined.secondTerm().secondTerm() == ev_expr3);
        REQUIRE(and_or_combined.atomicEventSourceID() == Event::ANY_SOURCE->getUID());
        REQUIRE(and_or_combined.descriptor().empty());

        EventExpression and_or_combined2 = (ev_expr || ev_expr2) && ev_expr3;
        REQUIRE(and_or_combined2.type() == AND);
        REQUIRE(and_or_combined2.secondTerm() == ev_expr3);
        REQUIRE(and_or_combined2.firstTerm().type() == OR);
        REQUIRE(and_or_combined2.firstTerm().firstTerm() == ev_expr);
        REQUIRE(and_or_combined2.firstTerm().secondTerm() == ev_expr2);

        REQUIRE(and_or_combined != and_or_combined2);
        REQUIRE(and_or_combined == and_or_combined);
    }

    SECTION("Creation by operators with events as parameters") {
        EventExpression ev_expr("namedExpression", sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ee_and_event_combined = ev_expr && testEvent;
        REQUIRE(ee_and_event_combined.type() == AND);
        REQUIRE(ee_and_event_combined.firstTerm() == ev_expr);
        REQUIRE(ee_and_event_combined.firstTerm().descriptor() == "namedExpression");
        REQUIRE(ee_and_event_combined.secondTerm() == testEvent);
        REQUIRE(ee_and_event_combined.secondTerm().descriptor().empty());

        EventExpression event_and_ee_combined = testEvent && ev_expr;
        REQUIRE(event_and_ee_combined.type() == AND);
        REQUIRE(event_and_ee_combined.firstTerm() == testEvent);
        REQUIRE(event_and_ee_combined.firstTerm().descriptor().empty());
        REQUIRE(event_and_ee_combined.secondTerm().descriptor() == "namedExpression");
        REQUIRE(event_and_ee_combined.secondTerm() == ev_expr);

        EventExpression event_and_event_combined = testEvent && testEvent;
        REQUIRE(event_and_event_combined.type() == AND);
        REQUIRE(event_and_event_combined.firstTerm() == testEvent);
        REQUIRE(event_and_event_combined.secondTerm() == testEvent);

        EventExpression ee_or_event_combined = ev_expr || testEvent;
        REQUIRE(ee_or_event_combined.type() == OR);
        REQUIRE(ee_or_event_combined.firstTerm() == ev_expr);
        REQUIRE(ee_or_event_combined.firstTerm().descriptor() == "namedExpression");
        REQUIRE(ee_or_event_combined.secondTerm() == testEvent);
        REQUIRE(ee_or_event_combined.secondTerm().descriptor().empty());

        EventExpression event_or_ee_combined = testEvent || ev_expr;
        REQUIRE(event_or_ee_combined.type() == OR);
        REQUIRE(event_or_ee_combined.firstTerm() == testEvent);
        REQUIRE(event_or_ee_combined.firstTerm().descriptor().empty());
        REQUIRE(event_or_ee_combined.secondTerm().descriptor() == "namedExpression");
        REQUIRE(event_or_ee_combined.secondTerm() == ev_expr);

        EventExpression event_or_event_combined = testEvent || testEvent;
        REQUIRE(event_or_event_combined.type() == OR);
        REQUIRE(event_or_event_combined.firstTerm() == EventExpression(testEvent));
        REQUIRE(event_or_event_combined.secondTerm() == EventExpression(testEvent));

    }

    //DEPRECATE: This will deprecate or change to tests of cythonutils.
    SECTION("[shared_ptr] Creation by //operators") {
        EventExpression ev_expr(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr2(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression ev_expr3(sourceEntity, eventTypeA, Event::ANY_ID);
        EventExpression and_combined = logicalAnd(ev_expr, ev_expr2);
        REQUIRE(and_combined.type() == AND);
        REQUIRE(and_combined.firstTerm() == ev_expr);
        REQUIRE(and_combined.secondTerm() == ev_expr2);

        EventExpression and_combined2 = logicalAnd(logicalAnd(ev_expr, ev_expr2), ev_expr3);
        REQUIRE(and_combined2.type() == AND);
        REQUIRE(and_combined2.firstTerm() == and_combined2.firstTerm());

        EventExpression or_combined = logicalOr(logicalOr(ev_expr, ev_expr2), ev_expr3);
        REQUIRE(or_combined.type() == OR);
        REQUIRE(or_combined.firstTerm().type() == OR);
        REQUIRE(or_combined.firstTerm().firstTerm() == ev_expr);
        REQUIRE(or_combined.firstTerm() == or_combined.firstTerm());
        REQUIRE(or_combined.secondTerm() == or_combined.secondTerm());
    }

    SECTION("Retrieving atomic expressions (terms) from expressions") {
        EventExpression ev_expr("namedExpression", sourceEntity, eventTypeA, 1);
        EventExpression ev_expr2(sourceEntity, eventTypeA, 2);
        EventExpression ev_expr3(sourceEntity, eventTypeA, 3);

        auto atoms = ev_expr.atoms();
        REQUIRE(atoms.size() == 1);
        REQUIRE(find(atoms.begin(),atoms.end(),ev_expr) != atoms.end());

        EventExpression and_combined = ev_expr && ev_expr2;
        auto and_atoms = and_combined.atoms();
        REQUIRE(and_atoms.size() == 2);
        REQUIRE(find(and_atoms.begin(),and_atoms.end(),ev_expr) != and_atoms.end());
        REQUIRE(find(and_atoms.begin(),and_atoms.end(),ev_expr2) != and_atoms.end());
        REQUIRE(ev_expr != ev_expr2);

        EventExpression and_combined2 = ev_expr && ev_expr2 || ev_expr3;
        auto and_atoms2 = and_combined2.atoms();
        REQUIRE(and_atoms2.size() == 3);
        REQUIRE(find(and_atoms2.begin(),and_atoms2.end(),ev_expr) != and_atoms.end());
        REQUIRE(find(and_atoms2.begin(),and_atoms2.end(),ev_expr2) != and_atoms.end());
        REQUIRE(find(and_atoms2.begin(),and_atoms2.end(),ev_expr3) != and_atoms.end());
        REQUIRE(ev_expr != ev_expr2);
        REQUIRE(ev_expr != ev_expr3);
        REQUIRE(ev_expr3 != ev_expr2);
    }
}

TEST_CASE( "[shared_ptr] EventExpression activation during simulaton runs", "[eventexpression][stable]") {
    const dynaa::EventType eventTypeA("TYPE_A", "Description A");
    const dynaa::EventType eventTypeB("TYPE_B", "Description B");
    auto sourceEntity = make_shared<EntityTest>();
    auto sourceEntity2 = make_shared<EntityTest>();
    auto waitingEntity = make_shared<EntityTest>();
    auto nullEventHandler = make_shared<EventHandler>();
    shared_ptr<HandlerReport> result_report;

    auto callback_lambda = [&result_report](dynaa::HandlerReport report) {
        result_report = make_shared<HandlerReport>(report); //Copy report to local register
    };

    auto lambdaEventHandler = make_shared<EventHandler>(callback_lambda);
    DynAASim &simEngine = DynAASim::getInstance();
    simEngine.reset();

    SECTION( "Registration and activation waiting on atomic event expressions - active by source and type") {
        EventExpression ev_expr(sourceEntity, eventTypeA, Event::ANY_ID);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        HandlerToken token = waitingEntity->waitOnce(lambdaEventHandler, ev_expr);
        EventExpression trigger = sourceEntity->scheduleAt(1, eventTypeA);
        REQUIRE_FALSE(result_report);    // There is no result report available yet
        simEngine.run(2);                // Activates the expression via event
        REQUIRE(result_report);          // Result reported was produced
        REQUIRE(result_report->getEventExpression() == ev_expr);
        REQUIRE(result_report->getHandlerPriority() == EventHandler::DEFAULT_PRIORITY_LEVEL);
        REQUIRE(result_report->getHandlerToken() == token);
        REQUIRE(result_report->once());
        auto state_report = result_report->getEventStateReport();
        REQUIRE(state_report.size() == 1);
        REQUIRE(state_report[0].atom_ee == ev_expr);
        REQUIRE(state_report[0].value);
        REQUIRE(state_report[0].trigger_time == Approx(1));
        REQUIRE(state_report[0].last_trigger == trigger);
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
    }

    SECTION( "Registration and activation waiting on atomic event expressions - active by type only") {
        EventExpression ev_expr2(Event::ANY_SOURCE, eventTypeA, Event::ANY_ID);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        HandlerToken token = waitingEntity->waitOnce(lambdaEventHandler, ev_expr2);
        EventExpression trigger = sourceEntity->scheduleAt(1, eventTypeA);
        REQUIRE_FALSE(result_report);    // There is no result report available yet
        simEngine.run(2);                // Activates the expression via event
        REQUIRE(result_report);          // Result reported was produced
        REQUIRE(result_report->getEventExpression() == ev_expr2);
        REQUIRE(result_report->getHandlerPriority() == EventHandler::DEFAULT_PRIORITY_LEVEL);
        REQUIRE(result_report->getHandlerToken() == token);
        REQUIRE(result_report->once());
        auto state_report = result_report->getEventStateReport();
        REQUIRE(state_report.size() == 1);
        REQUIRE(state_report[0].atom_ee == ev_expr2);
        REQUIRE(state_report[0].value);
        REQUIRE(state_report[0].trigger_time == Approx(1));
        REQUIRE(state_report[0].last_trigger == trigger);
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
    }

    SECTION( "[shared_ptr] Registration and activation waiting on event expressions") {
        EventExpression ev_expr3(sourceEntity2, eventTypeA, Event::ANY_ID);
        REQUIRE(simEngine.getCurrentTime() == Approx(0));
        waitingEntity->waitOnce(lambdaEventHandler, ev_expr3);
        sourceEntity->scheduleAt(1, eventTypeA); // Should not activate the callback because of wrong source
        REQUIRE_FALSE(result_report);    // There is no result report available yet
        simEngine.run(2);                // Activates the expression via event
        REQUIRE_FALSE(result_report);          // Result reported was produced
        REQUIRE(simEngine.getCurrentTime() == Approx(2));
    }
//
//    SECTION( "[shared_ptr] Registration and activation of single EventExpression //objects -- further event match cases") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        REQUIRE(simEngine.getCurrentTime() == Approx(0));
//        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, ev_expr) );
//        sourceEntity2->scheduleAt(1,
//                                  eventTypeA); // Issues correct event type but //other source, should not trigger the expression
//        sourceEntity->scheduleAt(1,
//                                 eventTypeB); // Issues from correct source, but //other event type. Should not trigger the //expression
//        REQUIRE(ev_expr->value() == false);
//        simEngine.run(2); // Issue events by advancing time... expression should //not be activated
//        REQUIRE(simEngine.getCurrentTime() == Approx(2));
//        REQUIRE(ev_expr->value() == false);
//    }
//
//    SECTION( "[shared_ptr] Registration and activation of AND combined - 2 terms //expressions") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>//(sourceEntity2, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> and_combined = ev_expr->logicalAnd(ev_expr2);
//        REQUIRE(simEngine.getCurrentTime() == Approx(0));
//        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, and_combined) )//;
//        Event event1 = sourceEntity2->scheduleAt(1, eventTypeA); // When issued //activates part of the expression.
//        Event event2 = sourceEntity->scheduleAt(3, eventTypeA); // When issued //activates the second part of the expression.
//        REQUIRE(and_combined->value() == false);
//        REQUIRE(and_combined->firstTerm().value() == false);
//        REQUIRE(and_combined->firstTermAsPtr()->value() == false);
//        REQUIRE(and_combined->secondTerm().value() == false);
//        REQUIRE(and_combined->secondTermAsPtr()->value() == false);
//        REQUIRE(and_combined->firstTerm().triggeredEvent() == nullptr);
//        REQUIRE(and_combined->secondTerm().triggeredEvent() == nullptr);
//        REQUIRE(isnan(and_combined->triggeredTime()));
//        simEngine.run(2); // Advances time, only one event is issued triggering //only one term of the expression.
//        REQUIRE(simEngine.getCurrentTime() == Approx(2));
//        REQUIRE(and_combined->value() == false);
//        REQUIRE(and_combined->firstTerm().value() == false);
//        REQUIRE(and_combined->secondTerm().value() == true);
//        REQUIRE(and_combined->firstTermAsPtr()->value() == false);
//        REQUIRE(and_combined->secondTermAsPtr()->value() == true);
//        REQUIRE(and_combined->firstTerm().triggeredEvent() == nullptr);
//        REQUIRE(and_combined->secondTerm().triggeredEvent()->id() == event1.id())//;
//        REQUIRE(isnan(and_combined->triggeredTime()));
//        simEngine.run(4); // Advances time, second event is issued triggering //the second term of the expression.
//        REQUIRE(simEngine.getCurrentTime() == Approx(4));
//        REQUIRE(and_combined->value() == true);
//        REQUIRE(and_combined->firstTerm().value() == true);
//        REQUIRE(and_combined->secondTerm().value() == true);
//        REQUIRE(and_combined->firstTermAsPtr()->value() == true);
//        REQUIRE(and_combined->secondTermAsPtr()->value() == true);
//        REQUIRE(and_combined->firstTerm().triggeredEvent()->id() == event2.id());
//        REQUIRE(and_combined->secondTerm().triggeredEvent()->id() == event1.id())//;
//        REQUIRE(and_combined->triggeredTime() == Approx(3));
//        // Below expressions false because they weren't waiting:
//        REQUIRE(ev_expr->value() == false);
//        REQUIRE(ev_expr2->value() == false);
//    }
//
//    SECTION( "[shared_ptr] Registration and activation of OR combined //EventExpression objects - 2 terms expressions") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>//(sourceEntity2, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> or_combined = ev_expr->logicalOr(ev_expr2);
//        REQUIRE(simEngine.getCurrentTime() == Approx(0));
//        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, or_combined) );
//        Event event1 =sourceEntity2->scheduleAt(1, eventTypeA); // When issued //activates part of the expression.
//        Event event2 =sourceEntity->scheduleAt(3, eventTypeA); // When issued //activates the second part of the expression.
//        REQUIRE(or_combined->value() == false);
//        REQUIRE(or_combined->firstTerm().value() == false);
//        REQUIRE(or_combined->secondTerm().value() == false);
//        REQUIRE(or_combined->firstTerm().triggeredEvent() == nullptr);
//        REQUIRE(or_combined->secondTerm().triggeredEvent() == nullptr);
//        REQUIRE(isnan(or_combined->triggeredTime()));
//        simEngine.run(2); // Advances time, only one event is issued triggering //only one term of the expression.
//        REQUIRE(simEngine.getCurrentTime() == Approx(2));
//        REQUIRE(or_combined->value() == true); // As it is an OR expression, it //should be already active.
//        REQUIRE(or_combined->firstTerm().value() == false);
//        REQUIRE(or_combined->secondTerm().value() == true);
//        REQUIRE(or_combined->firstTerm().triggeredEvent() == nullptr);
//        REQUIRE(or_combined->secondTerm().triggeredEvent()->id() == event1.id());
//        REQUIRE(or_combined->triggeredTime() == Approx(1));
//        // Below expressions false because they weren't waiting:
//        REQUIRE(ev_expr->value() == false);
//        REQUIRE(ev_expr2->value() == false);
//    }
//
//    SECTION( "ReRegistration and activation of AND/OR combined EventExpression //objects - 3 terms expressions") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>//(sourceEntity2, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>//(Event::ANY_SOURCE, eventTypeB, Event::ANY_ID);
//        shared_ptr<EventExpression> and_or_combined = (ev_expr->logicalAnd//(ev_expr2))->logicalOr(ev_expr3);
//        REQUIRE(simEngine.getCurrentTime() == Approx(0));
//        REQUIRE_NOTHROW( waitingEntity->waitOnce(nullExprHandler, //and_or_combined) );
//        sourceEntity2->scheduleAt(1, eventTypeA); // When issued activates part //of the expression.
//        sourceEntity->scheduleAt(3, eventTypeB); // When issued activates the //second part of the expression.
//        REQUIRE(and_or_combined->value() == false);
//        simEngine.run(2); // Advances time, only one event is issued triggering //only one term of the expression.
//        REQUIRE(simEngine.getCurrentTime() == Approx(2));
//        REQUIRE(and_or_combined->value() == false); // Not yet active
//        REQUIRE(isnan(and_or_combined->triggeredTime()));
//        simEngine.run(4);
//        REQUIRE(simEngine.getCurrentTime() == Approx(4));
//        REQUIRE(and_or_combined->value() == true); // Active by the ev_expr3 in //the OR branch.
//        REQUIRE(and_or_combined->firstTerm().firstTerm().value() == false); // //Term that should not yet be active
//        REQUIRE(and_or_combined->triggeredTime() == Approx(3));
//    }
//}
//
//TEST_CASE( "Re[shared_ptr] EventExpression automatic triggering of callback //functions", "[eventexpression][stable]") {
//    const dynaa::EventType eventTypeA("TYPE_A", //"Description A");
//    const dynaa::EventType eventTypeB("TYPE_B", //"Description B");
//    auto sourceEntity = make_shared<EntityTest>();
//    auto sourceEntity2 = make_shared<EntityTest>();
//    auto sourceEntity3 = make_shared<EntityTest>();
//    auto waitingEntity = make_shared<EntityTest>();
//    DynAASim &simEngine = DynAASim::getInstance();
//    simEngine.reset();
//
//    SECTION( "ReRegistration and activation of single EventExpression objects") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>//(sourceEntity2, eventTypeA, Event::ANY_ID);
//        REQUIRE(simEngine.getCurrentTime() == Approx(0));
//        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity->simpleExprHandler, //ev_expr));
//        REQUIRE_NOTHROW(waitingEntity->waitOnce//(sourceEntity2->simpleExprHandler, ev_expr3));
//        sourceEntity->scheduleAt(1,eventTypeA);
//        REQUIRE(!(sourceEntity->m_check));
//        REQUIRE(!(sourceEntity2->m_check));
//        sourceEntity->m_counter = 0;
//        sourceEntity2->m_counter = 0;
//        simEngine.run(2); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(2));
//        REQUIRE(sourceEntity->m_check);
//        REQUIRE(sourceEntity->m_counter == 1);
//        REQUIRE(!(sourceEntity2->m_check));
//    }
//
//    SECTION( "Continuous waiting of single EventExpression objects") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>//(sourceEntity2, eventTypeA, Event::ANY_ID);
//        REQUIRE(simEngine.getCurrentTime() == 0);
//        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity->simpleExprHandler, //ev_expr));
//        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity2->simpleExprHandler, //ev_expr3));
//        int N = 3;
//        for (int i = 0; i < N; i++) {
//            sourceEntity->scheduleAt(i + 1,eventTypeA);
//        }
//        REQUIRE(!(sourceEntity->m_check));
//        REQUIRE(!(sourceEntity2->m_check));
//        sourceEntity->m_counter = 0;
//        sourceEntity2->m_counter = 0;
//        simEngine.run(4); // Activates the expression via event
//        REQUIRE(sourceEntity->m_check);
//        REQUIRE(sourceEntity->m_counter == N);
//        REQUIRE(!(sourceEntity2->m_check));
//    }
//
//    SECTION( "Multiple ExpressionHandlers for the same EventExpression") {
//        shared_ptr<EventExpression> ev_expr1 = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        // This also tests having multiple handlers
//        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity->simpleExprHandler, //ev_expr1));
//        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity2->simpleExprHandler, //ev_expr1));
//        REQUIRE_NOTHROW(waitingEntity->waitOnce//(sourceEntity3->simpleExprHandler, ev_expr1));
//        int N = 3.;
//        double timestep = 10.;
//        for (int i = 1; i <= N; i++){
//            sourceEntity->scheduleAt(i * timestep, eventTypeA);
//        }
//        REQUIRE(sourceEntity->m_counter == 0);
//        REQUIRE(sourceEntity2->m_counter == 0);
//        REQUIRE(sourceEntity3->m_counter == 0);
//        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr1)//.size() == 3);
//        simEngine.run(timestep + 1);
//        REQUIRE(sourceEntity->m_counter == 1);
//        REQUIRE(sourceEntity2->m_counter == 1);
//        REQUIRE(sourceEntity3->m_counter == 1);
//        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr1)//.size() == 2);
//        // Override a continuous wait with wait once:
//        REQUIRE_NOTHROW(waitingEntity->waitOnce//(sourceEntity2->simpleExprHandler, ev_expr1));
//        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr1)//.size() == 2);
//        simEngine.run(2 * timestep + 1);
//        REQUIRE(sourceEntity->m_counter == 2);
//        REQUIRE(sourceEntity2->m_counter == 2);
//        REQUIRE(sourceEntity3->m_counter == 1);
//        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr1)//.size() == 1);
//        simEngine.run(N * timestep + 1);
//        REQUIRE(sourceEntity->m_counter == N);
//        REQUIRE(sourceEntity2->m_counter == 2);
//        REQUIRE(sourceEntity3->m_counter == 1);
//        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr1)//.size() == 1);
//    }
//
//    SECTION( "Dismissing of EventExpression objects") {
//        shared_ptr<EventExpression> ev_expr1 = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>//(sourceEntity2, eventTypeB, Event::ANY_ID);
//        // This also tests having multiple handlers
//        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity->simpleExprHandler, //ev_expr1));
//        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity2->simpleExprHandler, //ev_expr1));
//        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity->simpleExprHandler, //ev_expr2));
//        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity2->simpleExprHandler, //ev_expr2));
//        std::vector<std::unique_ptr<Event>> triggered_events = {};
//        double timestep = 10.;
//        triggered_events.push_back(std::unique_ptr<Event>(new Event//(sourceEntity->scheduleAt(timestep, eventTypeA))));
//        triggered_events.push_back(std::unique_ptr<Event>(new Event//(sourceEntity2->scheduleAt(timestep, eventTypeB))));
//        REQUIRE(sourceEntity->m_triggeredEvents.size() == 0);
//        REQUIRE(sourceEntity2->m_triggeredEvents.size() == 0);
//        simEngine.run(timestep + 1); // Activates the expression via event
//        // Check triggered events captured as expected (before dismissals)
//        REQUIRE(sourceEntity->m_triggeredEvents.size() == 2);
//        REQUIRE(sourceEntity2->m_triggeredEvents.size() == 2);
//        REQUIRE(std::equal(triggered_events.begin(), triggered_events.end(),
//                           sourceEntity->m_triggeredEvents.begin(),
//                           [](const std::unique_ptr<Event>& a, const //std::unique_ptr<Event>& b){ return *a == *b; }));
//        REQUIRE(std::equal(triggered_events.begin(), triggered_events.end(),
//                           sourceEntity2->m_triggeredEvents.begin(),
//                           [](const std::unique_ptr<Event>& a, const //std::unique_ptr<Event>& b){ return *a == *b; }));
//        // Now dismiss handler1 for expr1, and handler2 for all expressions
//        waitingEntity->dismiss(sourceEntity->simpleExprHandler, ev_expr1);
//        waitingEntity->dismiss(sourceEntity2->simpleExprHandler);
//        sourceEntity->m_triggeredEvents.clear();
//        sourceEntity2->m_triggeredEvents.clear();
//        timestep += 10.;
//        sourceEntity->scheduleAt(timestep, eventTypeA);
//        Event event2 = sourceEntity2->scheduleAt(timestep, eventTypeB);
//        simEngine.run(timestep + 1); // Activates the expression via event
//        // Check only handler1 triggered on expr2
//        REQUIRE(sourceEntity->m_triggeredEvents.size() == 1);
//        REQUIRE(sourceEntity2->m_triggeredEvents.size() == 0);
//        REQUIRE(*sourceEntity->m_triggeredEvents[0] == event2);
//    }
//
//    SECTION( "Nested waiting for EventExpression objects") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        REQUIRE(simEngine.getCurrentTime() == Approx(0));
//        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity->nestedExprHandler, //ev_expr));
//        sourceEntity->scheduleAfter(1,eventTypeA);
//        sourceEntity->m_counter = 0;
//        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr).//size() == 1);
//        simEngine.run(2); // Activates the expression via event, counter should //go to 1
//        REQUIRE(simEngine.getCurrentTime() == Approx(2));
//        REQUIRE(sourceEntity->m_counter == 1);
//        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr).//size() == 1);
//        sourceEntity->scheduleAfter(1,eventTypeA);
//        sourceEntity->m_check = false;
//        simEngine.run(4);
//        REQUIRE(simEngine.getCurrentTime() == Approx(4));
//        REQUIRE(sourceEntity->m_counter == 2);
//        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr).//size() == 1);
//        sourceEntity->scheduleAfter(1,eventTypeA);
//        sourceEntity->m_check = false;
//        simEngine.run(6);
//        REQUIRE(simEngine.getCurrentTime() == Approx(6));
//        REQUIRE(sourceEntity->m_counter == 3);
//        REQUIRE(simEngine.getHandlerInspector().getExpressionHandlersOf(ev_expr).//size() == 1);
//    }
//
//    SECTION( "ReRegistration and activation of combined EventExpression //objects") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>//(sourceEntity2, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>//(sourceEntity2, eventTypeB, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr4 = make_shared<EventExpression>//(sourceEntity, eventTypeB, Event::ANY_ID);
//        shared_ptr<EventExpression> combined = make_shared<EventExpression>//(*ev_expr && *ev_expr2 || *ev_expr3 && *ev_expr4);
//        shared_ptr<EventExpression> combined2 = make_shared<EventExpression>//(*ev_expr || *ev_expr3);
//        REQUIRE(simEngine.getCurrentTime() == Approx(0));
//        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity->simpleExprHandler, //combined));
//        REQUIRE_NOTHROW(waitingEntity->waitOnce//(sourceEntity2->simpleExprHandler, combined2));
//        REQUIRE(!(sourceEntity->m_check));
//        REQUIRE(!(sourceEntity2->m_check));
//        sourceEntity->scheduleAt(1,eventTypeA); // will activate ev_expr only
//        sourceEntity2->scheduleAt(3,eventTypeB); // will ativate ev_expr3 only
//        sourceEntity->scheduleAt(5,eventTypeB); // will ativate ev_expr4 only //and the combined expression
//        sourceEntity->m_counter = 0;
//        simEngine.run(2); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(2));
//        REQUIRE(!(sourceEntity->m_check)); // Not yet.
//        REQUIRE(isnan(combined->triggeredTime()));
//        REQUIRE(sourceEntity2->m_check); // Yes, should be active
//        REQUIRE(combined2->triggeredTime() == Approx(1));
//        REQUIRE(sourceEntity2->m_counter == 1);
//        simEngine.run(4); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(4));
//        REQUIRE(!(sourceEntity->m_check)); // Not yet.
//        REQUIRE(isnan(combined->triggeredTime()));
//        REQUIRE(sourceEntity2->m_counter == 1);  // second expression in OR //shouldn't call callback again
//        REQUIRE(combined2->triggeredTime() == Approx(1));
//        simEngine.run(6); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(6));
//        REQUIRE(sourceEntity->m_check); // Yes, should be active
//        REQUIRE(combined->triggeredTime() == Approx(5));
//        REQUIRE(sourceEntity->m_counter == 1);
//    }
//
//    SECTION( "Continuous waiting of combined EventExpression objects") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>//(sourceEntity2, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>//(sourceEntity2, eventTypeB, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr4 = make_shared<EventExpression>//(sourceEntity, eventTypeB, Event::ANY_ID);
//        shared_ptr<EventExpression> combined = make_shared<EventExpression>//(*ev_expr && *ev_expr2 || *ev_expr3 && *ev_expr4);
//        shared_ptr<EventExpression> combined2 = make_shared<EventExpression>//(*ev_expr || *ev_expr3);
//        REQUIRE(simEngine.getCurrentTime() == 0);
//        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity->simpleExprHandler, //combined));
//        REQUIRE_NOTHROW(waitingEntity->wait(sourceEntity2->simpleExprHandler, //combined2));
//        REQUIRE(!(sourceEntity->m_check));
//        REQUIRE(!(sourceEntity2->m_check));
//        sourceEntity->scheduleAt(1,eventTypeA); // will activate ev_expr only
//        sourceEntity2->scheduleAt(3,eventTypeB); // will ativate ev_expr3 only
//        sourceEntity->scheduleAt(5,eventTypeB); // will ativate ev_expr4 only //and the combined expression
//        sourceEntity2->scheduleAt(6,eventTypeA); // will ativate ev_expr2 only
//        sourceEntity->scheduleAt(9,eventTypeA); // will activate ev_expr only //and the combined expression
//        sourceEntity->m_counter = 0;
//        simEngine.run(2); // Activates the expression via event
//        REQUIRE(!(sourceEntity->m_check)); // Not yet.
//        REQUIRE(sourceEntity2->m_check); // Yes, should be active (never reset)
//        REQUIRE(isnan(combined2->triggeredTime()));  // reprimed, so again NaN
//        //REQUIRE(sourceEntity2->m_triggeredTimes.back() == Approx(1));
//        REQUIRE(sourceEntity2->m_counter == 1);
//        simEngine.run(4); // Activates the expression via event
//        REQUIRE(!(sourceEntity->m_check)); // Not yet.
//        REQUIRE(sourceEntity2->m_counter == 2);  // activated again
//        REQUIRE(isnan(combined2->triggeredTime()));  // reprimed, so again NaN
//        //REQUIRE(sourceEntity2->m_triggeredTimes.back() == Approx(3));
//        simEngine.run(6); // Activates the expression via event
//        REQUIRE(sourceEntity->m_check); // Yes, should be active
//        REQUIRE(sourceEntity->m_counter == 1);
//        REQUIRE(sourceEntity2->m_counter == 2);  // as before
//        REQUIRE(isnan(combined2->triggeredTime()));  // as before
//        //REQUIRE(sourceEntity2->m_triggeredTimes.back() == Approx(3));  // as //before
//        simEngine.run(7);
//        REQUIRE(sourceEntity->m_counter == 1); // as before
//        REQUIRE(sourceEntity2->m_counter == 2);  // as before
//        simEngine.run(10);
//        REQUIRE(sourceEntity->m_counter == 2); // + 1
//        REQUIRE(sourceEntity2->m_counter == 3);  // + 1
//        REQUIRE(isnan(combined2->triggeredTime()));
//        //REQUIRE(sourceEntity2->m_triggeredTimes.back() == Approx(9));
//    }
//
//    SECTION( "Nested waits in the callbacks of combined EventExpression //objects") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>//(sourceEntity2, eventTypeB, Event::ANY_ID);
//        shared_ptr<EventExpression> combined2 = make_shared<EventExpression>//(*ev_expr || *ev_expr3);
//        REQUIRE(simEngine.getCurrentTime() == Approx(0));
//        REQUIRE_NOTHROW(waitingEntity->waitOnce//(sourceEntity2->nestedExprHandler, combined2));
//        sourceEntity2->scheduleAt(1,eventTypeB); // will activate ev_expr3 only
//        sourceEntity2->scheduleAt(3,eventTypeB); // will ativate ev_expr3 only
//        sourceEntity2->scheduleAt(5,eventTypeB); // will ativate ev_expr3 only
//        sourceEntity->scheduleAt(7,eventTypeA); // will ativate ev_expr only
//        sourceEntity2->scheduleAt(9,eventTypeB); // will activate ev_expr3 only
//        sourceEntity2->m_counter = 0;
//        simEngine.run(2); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(2));
//        REQUIRE(sourceEntity2->m_counter == 1);
//        simEngine.run(4); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(4));
//        REQUIRE(sourceEntity2->m_counter ==
//                2);  // second expression in OR and because it was re-enabled in //the callback it should count 2
//        simEngine.run(6); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(6));
//        REQUIRE(sourceEntity2->m_counter == 3);
//        simEngine.run(8); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(8));
//        REQUIRE(sourceEntity2->m_counter == 4);
//        simEngine.run(10); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(10));
//        REQUIRE(sourceEntity2->m_counter == 5);
//    }
//
//    SECTION( "Nested waits in the callbacks of AND-OR combined EventExpression //objects") {
//        shared_ptr<EventExpression> ev_expr = make_shared<EventExpression>//(sourceEntity, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr2 = make_shared<EventExpression>//(sourceEntity2, eventTypeA, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr3 = make_shared<EventExpression>//(sourceEntity2, eventTypeB, Event::ANY_ID);
//        shared_ptr<EventExpression> ev_expr4 = make_shared<EventExpression>//(sourceEntity, eventTypeB, Event::ANY_ID);
//        shared_ptr<EventExpression> combined = make_shared<EventExpression>//(*ev_expr && *ev_expr2 || *ev_expr3 && *ev_expr4);
//        REQUIRE(simEngine.getCurrentTime() == Approx(0));
//        REQUIRE_NOTHROW(waitingEntity->waitOnce(sourceEntity->nestedExprHandler, //combined));
//        sourceEntity->scheduleAt(1,eventTypeA); // will activate ev_expr only
//        sourceEntity2->scheduleAt(1,eventTypeA); // will ativate ev_expr2 only
//        sourceEntity->scheduleAt(3,eventTypeB); // will ativate ev_expr4 only //and the combined expression
//        sourceEntity2->scheduleAt(5,eventTypeB); // will ativate ev_expr3 only
//        sourceEntity->scheduleAt(7,eventTypeA); // will activate ev_expr only
//        sourceEntity2->scheduleAt(7,eventTypeA); // will ativate ev_expr2 only
//        sourceEntity->m_counter = 0;
//        simEngine.run(2); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(2));
//        REQUIRE(sourceEntity->m_counter == 1);
//        simEngine.run(4); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(4));
//        REQUIRE(sourceEntity->m_counter == 1); // Not yet.
//        simEngine.run(6); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(6));
//        REQUIRE(sourceEntity->m_counter == 2); // Yes, should be active
//        simEngine.run(8); // Activates the expression via event
//        REQUIRE(simEngine.getCurrentTime() == Approx(8));
//        REQUIRE(sourceEntity->m_counter == 3);  // Combined and combined 2: both //affected.
//    }
//}
//
}
