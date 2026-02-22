/*
file: entitytest.h

Definition of an EntityTest class to facilitate testing.
*/

#ifndef _INCLUDED_ENTITYTEST_
#define _INCLUDED_ENTITYTEST_

#include "../include/entity.h"
#include "../include/eventexpression.h"
//#include "../include/expressionhandler.h"

#include <memory>
#include <vector>

using namespace dynaa;

/**
 * Test wrapper class for Entity class
 *
 * Wraps the Entity class to give access to protected methods for testing
 */
class EntityTest : public Entity {

public:
    bool m_check;
    int m_counter = 0;
    std::vector<std::unique_ptr<Event>> m_triggeredEvents;
    std::vector<double> m_triggeredTimes;
    std::shared_ptr<EventHandler> simpleExprHandler;
    std::shared_ptr<EventHandler> nestedExprHandler;

    EntityTest() : Entity(), m_check(false) {
        simpleExprHandler = std::make_shared<EventHandler>(bind(&EntityTest::simple_callback, this,
                            std::placeholders::_1), "simple");
        nestedExprHandler = std::make_shared<EventHandler>(bind(&EntityTest::nested_callback, this,
                            std::placeholders::_1), "nested");
        m_triggeredEvents = std::vector<std::unique_ptr<Event>>();
        m_triggeredTimes = std::vector<double>();
    }

    /**
     * Public access to protected methods:
     */
    EventExpression scheduleNow(const EventType & evType) {
        return Entity::scheduleNow(evType);
    }
    EventExpression scheduleAfter(SimTime interval, const EventType & evType) {
        return Entity::scheduleAfter(interval, evType);
    }
    EventExpression scheduleAt(SimTime instant, const EventType & evType) {
        return Entity::scheduleAt(instant, evType);
    }
    int unschedule(const EventType & evType, const EventID eventID) {
        return Entity::unschedule(evType, eventID);
    }

    void wait(std::shared_ptr<EventHandler> eventHandler) const {
        Entity::wait(eventHandler);
    }
    HandlerToken waitOnce(std::shared_ptr<EventHandler> eventHandler, Event event) {
        return Entity::waitOnce(eventHandler, event);
    }
    HandlerToken waitOnce(std::shared_ptr<EventHandler> eventHandler, EventExpression event_expression) {
        return Entity::waitOnce(eventHandler, event_expression);
    }
    void wait(std::shared_ptr<EventHandler> eventHandler, const EventSource entity) const {
        Entity::wait(eventHandler, entity);
    }
    void wait(std::shared_ptr<EventHandler> eventHandler, const EventSource entity, const EventType &eventType) const {
        Entity::wait(eventHandler, entity, eventType);
    }
    void wait(std::shared_ptr<EventHandler> eventHandler, const EventType &eventType) const {
        Entity::wait(eventHandler, eventType);
    }
    void wait(std::shared_ptr<EventHandler> eventHandler, const EventType &eventType, const EventID eventID) const {
        Entity::wait(eventHandler, eventType, eventID);
    }
    void wait(std::shared_ptr<EventHandler> eventHandler, const EventSource source, const EventID eventID) const {
        Entity::wait(eventHandler, source, eventID);
    }
    void wait(std::shared_ptr<EventHandler> eventHandler, const EventExpression& eventExpr) const {
        Entity::wait(eventHandler, eventExpr);
    }

    static void callback(HandlerReport handler_report) {}  // Dummy callback function

    void callback_method(HandlerReport handler_report) {}  // Dummy callback member function

    void simple_callback(HandlerReport handler_report) {
        // TODO: Code below needs an equivalent
        // m_check = true;
        // m_counter++;
        // if (evExpr->type() == Atomic) {
        //     m_triggeredEvents.push_back(evExpr->triggeredEvent());
        //     m_triggeredTimes.push_back(evExpr->triggeredTime());
        //     // Keep sorted
        //     std::sort(m_triggeredEvents.begin(), m_triggeredEvents.end(),
        //               compare_triggered_events);
        //     std::sort(m_triggeredTimes.begin(), m_triggeredTimes.end());
        // }
    }

    void nested_callback(HandlerReport handler_report) {
        // TODO: Code below needs equivalent
        // if (evExpr.get() == nullptr) {
        //     return;
        // } else {
        //     m_counter++;
        //     waitOnce(nestedExprHandler, evExpr);
        // }
    }

    // Compare < in events makes no sense, right?
    // TODO: review.  Maybe dead code.
    // static bool compare_triggered_events(const std::unique_ptr<Event>& a, const std::unique_ptr<Event>& b) {
    //     return (*a < *b);
    // }
};

#endif
