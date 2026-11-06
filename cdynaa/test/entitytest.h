/*
file: entitytest.h

Definition of an EntityTest class to facilitate testing.
*/

#ifndef _INCLUDED_ENTITYTEST_
#define _INCLUDED_ENTITYTEST_

#include "../include/entity.h"
#include "../include/eventexpression.h"
#include "../include/expressionhandler.h"

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
    std::shared_ptr<ExpressionHandler> simpleExprHandler;
    std::shared_ptr<ExpressionHandler> nestedExprHandler;

    EntityTest() : Entity(), m_check(false) {
        simpleExprHandler = std::make_shared<ExpressionHandler>(bind(&EntityTest::simple_callback, this,
                                                                std::placeholders::_1), "simple");
        nestedExprHandler = std::make_shared<ExpressionHandler>(bind(&EntityTest::nested_callback, this,
                                                                std::placeholders::_1), "nested");
        m_triggeredEvents = std::vector<std::unique_ptr<Event>>();
        m_triggeredTimes = std::vector<double>();
    }

    /**
     * Public access to protected methods:
     */
    Event scheduleNow(const EventType & evType) {
        return Entity::scheduleNow(evType);
    }
    Event scheduleAfter(SimTime interval, const EventType & evType) {
        return Entity::scheduleAfter(interval, evType);
    }
    Event scheduleAt(SimTime instant, const EventType & evType) {
        return Entity::scheduleAt(instant, evType);
    }

    void wait(std::shared_ptr<EventHandler> eventHandler) const {
        Entity::wait(eventHandler);
    }
    void waitOnce(std::shared_ptr<EventHandler> eventHandler, Event event) {
        Entity::waitOnce(eventHandler, event);
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
    void wait(std::shared_ptr<ExpressionHandler> exprHandler, std::shared_ptr<EventExpression> eventExpr) const {
        Entity::wait(exprHandler, eventExpr);
    }
    void waitOnce(std::shared_ptr<ExpressionHandler> exprHandler, std::shared_ptr<EventExpression> eventExpr) const {
        Entity::waitOnce(exprHandler, eventExpr);
    }
    void dismiss(std::shared_ptr<ExpressionHandler> exprHandler, std::shared_ptr<EventExpression> eventExpr) {
        Entity::dismiss(exprHandler, eventExpr);
    }
    void dismiss(std::shared_ptr<ExpressionHandler> exprHandler) {
        Entity::dismiss(exprHandler);
    }

    static void callback(Event event) {}  // Dummy callback function

    void callback_method(Event event) {}  // Dummy callback member function

    void simple_callback(std::shared_ptr<EventExpression> evExpr) {
        m_check = true;
        m_counter++;
        if (evExpr->type() == Atomic) {
            m_triggeredEvents.push_back(evExpr->triggeredEvent());
            m_triggeredTimes.push_back(evExpr->triggeredTime());
            // Keep sorted
            std::sort(m_triggeredEvents.begin(), m_triggeredEvents.end(),
                      compare_triggered_events);
            std::sort(m_triggeredTimes.begin(), m_triggeredTimes.end());
        }
    }

    void nested_callback(std::shared_ptr<EventExpression> evExpr) {
        if (evExpr.get() == nullptr) {
            return;
        } else {
            m_counter++;
            waitOnce(nestedExprHandler, evExpr);
        }
    }

    static bool compare_triggered_events(const std::unique_ptr<Event>& a, const std::unique_ptr<Event>& b) {
        return (*a < *b);
    }
};

#endif
