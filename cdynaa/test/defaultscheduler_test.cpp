#include "catch.h"
#include "../include/defaultscheduler.h"
#include "../include/event.h"
#include "../include/simengine.h"
#include "entitytest.h"
#include "simenginetest.h"


#include <string>
#include <iostream>
#include <functional>

using namespace std;
using namespace dynaa;

TEST_CASE( "Checks the functionality of get number of instants in the scheduler", "[defaultscheduler][stable]" ) {
    DynAASim &simulator = DynAASim::getInstance();
    simulator.reset();
    DefaultScheduler scheduler = DefaultScheduler();
    Event event1;
    Event event2;

    SECTION( "Empty timeline" ) {
        REQUIRE(scheduler.getNumberOfInstants()==0);
    }
    SECTION( "Scheduled event using scheduleAt" ) {
        scheduler.scheduleAt(2,event1);
        REQUIRE(scheduler.getNumberOfInstants()==1);
    }
    SECTION( "Scheduled event using scheduleNow" ) {
        scheduler.scheduleNow(event1);
        REQUIRE(scheduler.getNumberOfInstants()==1);
    }
    SECTION( "Scheduled event using scheduleAt" ) {
        scheduler.scheduleAfter(2,event1);
        REQUIRE(scheduler.getNumberOfInstants()==1);
    }
    SECTION( "Unscheduling an event" ) {
        scheduler.scheduleAt(2,event1);
        scheduler.unschedule(event1);
        REQUIRE(scheduler.getNumberOfInstants()==0);
    }
    SECTION( "Schedule and popEvent" ) {
        scheduler.scheduleAt(2,event1);
        scheduler.scheduleAt(0,event2);
        scheduler.popNextEvent();
        REQUIRE(scheduler.getNumberOfInstants()==1);
    }
    SECTION( "Schedule multiple in an instant and popEvent" ) {
        scheduler.scheduleAt(2,event1);
        scheduler.scheduleAt(0,event1);
        scheduler.scheduleAt(0,event2);
        scheduler.popNextEvent();
        REQUIRE(scheduler.getNumberOfInstants()==2);
    }
    SECTION( "Schedule multiple and popEvent and scheduleNow" ) {
        scheduler.scheduleAt(2,event1);
        scheduler.scheduleAt(0,event2);
        scheduler.popNextEvent();
        scheduler.scheduleNow(event2);
        REQUIRE(scheduler.getNumberOfInstants()==2);
    }
    SECTION( "Schedule multiple in an instant and popEvent and scheduleNow" ) {
        scheduler.scheduleAt(2,event1);
        scheduler.scheduleAt(0,event1);
        scheduler.scheduleAt(0,event2);
        scheduler.popNextEvent();
        scheduler.scheduleNow(event2);
        REQUIRE(scheduler.getNumberOfInstants()==2);
    }
}
