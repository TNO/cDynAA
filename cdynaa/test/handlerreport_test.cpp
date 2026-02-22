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
using namespace dynaa;

/** Test cases for HandlerReport  */

TEST_CASE("HandlerReport creation", "[handlerreport][stable]") {

    const EventType eventTypeA("TYPE_A", "Description A");
    auto sourceEntity = make_shared<EntityTest>();
    EventExpression ev_expr("namedExpression", sourceEntity, eventTypeA, Event::ANY_ID);
    shared_ptr<EventHandler> eventHandler = make_shared<EventHandler>(EntityTest::callback);
    EventExpression event1 = sourceEntity->scheduleNow(eventTypeA);
    HandlerToken token;

    SECTION("Creation of event reports") {
        REQUIRE_NOTHROW(HandlerReport(token,
                                      ev_expr,
                                      5,
                                      true,
                                      vector<HandlerReport::EventReport>()));
    }

    SECTION("Retrieval of basic members") {
        HandlerReport report(token, ev_expr, 5, true,vector<HandlerReport::EventReport>());
        HandlerReport report2(token, ev_expr, 5, true,vector<HandlerReport::EventReport>(1, {ev_expr, false, event1, -1}));
        REQUIRE(report.getEventExpression() == ev_expr);
        REQUIRE(report.getHandlerToken() == token);
        REQUIRE(report.getHandlerPriority() == 5);
        REQUIRE(report.once());
        REQUIRE(report.getEventStateReport().empty());
        REQUIRE(report.getEventStateReportAsMap().empty());

        REQUIRE(report2.getEventExpression() == ev_expr);
        REQUIRE(report2.getHandlerToken() == token);
        REQUIRE(report2.getHandlerPriority() == 5);
        REQUIRE(report2.once());
        auto stateAsVector = report2.getEventStateReport();
        REQUIRE(stateAsVector.size() == 1);
        auto stateAsMap = report2.getEventStateReportAsMap();
        REQUIRE(stateAsMap.size() == 1);
        REQUIRE(stateAsMap[ev_expr] == stateAsVector[0]);

    }


}