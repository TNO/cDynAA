#include <string>
#include <iostream>
#include <memory>
#include "catch.h"
#include "entitytest.h"
#include "../include/eventhandler.h"
#include "../include/entity.h"

using namespace std;

TEST_CASE( "Entity creation, comparison and printing", "[entity][stable]" ) {

    shared_ptr<EntityTest> entity1, entity2;
    entity1 = make_shared<EntityTest>();
    entity2 = make_shared<EntityTest>();

    SECTION( "ID is unique to event and non-zero" ) {
        REQUIRE( entity1->getUID() != 0 );
        REQUIRE( entity2->getUID() != 0 );
        REQUIRE( entity1->getUID() != entity2->getUID() );
    }
    SECTION( "Events are equal if unique ids match" ) {
        REQUIRE( *entity1 == *entity1 );
        REQUIRE( *entity2 == *entity2 );
        REQUIRE( *entity1 != *entity2 );
    }
    SECTION( "Convert to string for printing" ) {
        std::ostringstream ss_test,  ss_result;
        ss_test << *entity1;
        ss_result << Entity::ENTITY_PREFIX << "_" << entity1->getUID();
        REQUIRE( ss_test.str() == ss_result.str() );
    }
}

TEST_CASE( "Entity can issue events", "[entity][stable]" ) {

    shared_ptr<EntityTest> entity = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType::create("TYPE_A", "Description A");

    REQUIRE( SimulationEngine::getInstance().getCurrentTime() == 0);

    SECTION( "Scheduling 'now' returns valid event" ) {
        Event event = entity->scheduleNow(eventTypeA);
        REQUIRE( event.source() == entity );
        REQUIRE( event.type() == eventTypeA);
    }
    SECTION( "Scheduling 'after' returns valid event" ) {
        SimTime delta_t = 1.;
        Event event = entity->scheduleAfter(delta_t, eventTypeA);
        REQUIRE( event.source() == entity );
        REQUIRE( event.type() == eventTypeA);
        REQUIRE_THROWS_AS( entity->scheduleAfter(-1, eventTypeA), InvalidTimePointException &);
    }
    SECTION( "Scheduling 'at' returns valid event" ) {
        SimTime t = 1.;
        Event event = entity->scheduleAt(t, eventTypeA);
        REQUIRE( event.source() == entity );
        REQUIRE( event.type() == eventTypeA);
        REQUIRE_THROWS_AS( entity->scheduleAfter(-1, eventTypeA), InvalidTimePointException &);
    }
    SECTION( "Incorrectly scheduling 'after' throws exception" ) {
        SimTime delta_t = -1.;
        REQUIRE(entity.use_count() == 1);
        REQUIRE_THROWS( entity->scheduleAfter(delta_t, eventTypeA));
    }
    SECTION( "Incorrectly scheduling 'at' throws exception" ) {
        SimTime t = -1.;
        REQUIRE_THROWS( entity->scheduleAt(t, eventTypeA) );
        SimTime timeAtPast = SimulationEngine::getInstance().getCurrentTime() - .1;
        REQUIRE_THROWS( entity->scheduleAt(timeAtPast, eventTypeA) );
    }
}

TEST_CASE( "Entity can register to listen for events", "[entity][stable]" ) {
    shared_ptr<EntityTest> entity = make_shared<EntityTest>();
    const dynaa::EventType &eventTypeA = EventType::create("TYPE_A", "Description A");
    EventID eventID = 10;

    // Creates event handler to be used as mock
    shared_ptr<EventHandler> eventHandler = EventHandler::create(EntityTest::callback);

    SECTION("Register to listen for events: all wildcards") {
        REQUIRE_NOTHROW( entity->wait(eventHandler) );
    }
    SECTION("Register to listen for events: wildcards variations") {
        REQUIRE(entity.use_count() == 1); // use countere of the pointer should not increment when using wait functions
        REQUIRE_NOTHROW( entity->wait(eventHandler, entity) );
        REQUIRE(entity.use_count() == 1);
        REQUIRE_NOTHROW( entity->wait(eventHandler, entity, eventTypeA) );
        REQUIRE(entity.use_count() == 1);
        REQUIRE_NOTHROW( entity->wait(eventHandler, eventTypeA) );
        REQUIRE_NOTHROW( entity->wait(eventHandler, eventTypeA, eventID) );
        REQUIRE_NOTHROW( entity->wait(eventHandler, entity, eventID) );
        REQUIRE(entity.use_count() == 1);
    }
}

TEST_CASE( "Entity can dismiss event handlers", "[entity]" ) {
    // These tests are performed when testing the handler register.
}

TEST_CASE( "Entity memory management", "[entity][stable]" ) {

    EventType dummyEvent = EventType::create("DUMMY","Dummy");
    auto volatileEntity = make_shared<EntityTest>();
    Event event = volatileEntity->scheduleNow(dummyEvent);

    SECTION( "Unique safe guard to a member function as callback" ) {
        REQUIRE(volatileEntity.use_count() == 1);
        vector<weak_ptr<Entity>> safe_guards = {volatileEntity};
        shared_ptr<EventHandler> eventHandler = EventHandler::create(
                bind(&EntityTest::callback_method, volatileEntity.get(), std::placeholders::_1), safe_guards);
        REQUIRE(volatileEntity.use_count() == 1);
        REQUIRE_NOTHROW(eventHandler->handle(event));
        volatileEntity.reset();
        REQUIRE_THROWS_AS(eventHandler->handle(event), InvalidHandler &);
    }
    //SECTION( "Several safe guards to a member function as callback" ) {
    //    auto otherVolatileEntity = make_shared<EntityTest>();
    //    shared_ptr<EventHandler> eventHandler = EventHandler::create(
    //            bind(&EntityTest::callback_method, volatileEntity.get(), std::placeholders::_1),
    //    {volatileEntity, otherVolatileEntity});
    //    REQUIRE_NOTHROW(eventHandler->handle(event));
    //    otherVolatileEntity.reset();
    //    REQUIRE_THROWS_AS(eventHandler->handle(event), InvalidHandler &);
    //}
    SECTION( "Several safe guards to a member function as callback" ) {
        auto otherVolatileEntity = make_shared<EntityTest>();
        vector<weak_ptr<Entity>> safe_guards;
        safe_guards.push_back(volatileEntity);
        safe_guards.push_back(otherVolatileEntity);
        EventHandler eventHandler = EventHandler(
                                        bind(&EntityTest::callback_method, volatileEntity.get(), std::placeholders::_1),
                                        safe_guards);
        REQUIRE_NOTHROW(eventHandler.handle(event));
        otherVolatileEntity.reset();
        REQUIRE_THROWS_AS(eventHandler.handle(event), InvalidHandler &);
    }

}
