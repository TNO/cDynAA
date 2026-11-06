/*
file: eventexpression.cpp
author: Julio Oliveira, TNO

Member function of the class EventExpression
*/

#include "../include/eventexpression.h"
//#include "../include/eventhandler.h"
//#include "../include/simengine.h"

#include <iostream>
#include <memory>
#include <cmath>

using namespace std;
using namespace dynaa;

EventExpression::EventExpression(const EventExpression &evExpression) :
            enable_shared_from_this<EventExpression>(*this),
            m_entity(evExpression.m_entity),
            m_evType(evExpression.m_evType),
            m_id(evExpression.m_id),
            //m_value(false),
            //m_callback(evExpression.m_callback) {
            m_type(evExpression.m_type) {
    if (evExpression.m_type != Atomic) {
        if(evExpression.m_firstTerm) {
            m_firstTerm = make_shared<EventExpression>(*evExpression.m_firstTerm);
        }
        if(evExpression.m_secondTerm) {
            m_secondTerm = make_shared<EventExpression>(*evExpression.m_secondTerm);
        }
    }
}

/*
void EventExpression::trigger(Event ev) {
    switch( m_type ) {
    case Atomic:
        if( !m_value ) {
            m_value = true;
            if (m_callback) {
                m_callback();
            }
        }
        break;
    default:
        break;
    }
}
*/

/*
void EventExpression::intern_trigger() {
    if( !m_value) {
        if( m_type == Atomic ) {
            m_value = true;
        }
        if (value()) {
            m_value = true;
            if (m_callback) {
                m_callback();
            }
        }
    }
}
*/

EventExpression::~EventExpression() {
    if (m_firstTerm) {
        m_firstTerm.reset(); // This reset is from the shared pointer that holds the term.  Releasing it.
    }
    if (m_secondTerm) {
        m_secondTerm.reset();
    }
}

const EventType& EventExpression::atomicEventType() const {
    return m_evType;
}

EventID EventExpression::atomicEventID() const {
    return m_id;
}

EventSource EventExpression::atomicEventSource() const {
    return m_entity;
}


EventExpressionType EventExpression::type() const {
    return m_type;
}

bool EventExpression::value() const {
    switch( m_type ) {
    case Atomic:
        //return m_value;
        return m_trigger_event.get() != nullptr;
    case AND:
        return m_firstTerm->value() && m_secondTerm->value();
    case OR:
        return m_firstTerm->value() || m_secondTerm->value();
    default:
        return false;
    }
}


SimTime EventExpression::triggeredTime() const {
    if (value()) {
        switch( m_type ) {
        case Atomic:
            return m_trigger_timestamp;
        case AND:
            return max(m_firstTerm->triggeredTime(), m_secondTerm->triggeredTime());
        case OR:
            if (m_firstTerm->value()) {
                if (m_secondTerm->value()) {
                    // In case both expressions have triggered
                    return min(m_firstTerm->triggeredTime(), m_secondTerm->triggeredTime());
                } else {
                    return m_firstTerm->triggeredTime();
                }
            } else {
                return m_secondTerm->triggeredTime();
            }
        }
    }
    // not triggered so return NaN
    return std::nan("1");
}

unique_ptr<Event> EventExpression::triggeredEvent() const {
    if (m_trigger_event)
        // Copy the event
        return unique_ptr<Event>(new Event(*m_trigger_event));
    else
        return nullptr;
}

shared_ptr<EventExpression> EventExpression::wrapTriggeredEvent(Event event) {
    shared_ptr<EventExpression> evExpr;
    try {
        evExpr = make_shared<EventExpression>(event.source(), event.type(), event.id());
        evExpr->m_trigger_event = unique_ptr<Event>(new Event(event));
    } catch (InvalidEventSourceException & e) {
        cout << "Warning! Tried to create event expression from an event whose source was invalid(already de-allocated). "
             << "Ignoring the event." << endl; // TODO: replace cout by an entry in logging engine.
    }
    return evExpr;
}

const shared_ptr<EventExpression> EventExpression::firstTermAsPtr()  {
    switch ( m_type ) {
    case AND:
    case OR:
        return m_firstTerm;
    default:
        return enable_shared_from_this<EventExpression>::shared_from_this();
    }
}

const EventExpression& EventExpression::firstTerm() const {
    switch ( m_type ) {
    case AND:
    case OR:
        return *m_firstTerm;
    default:
        return *this;
    }
}

const EventExpression& EventExpression::secondTerm() const {
    switch ( m_type ) {
    case AND:
    case OR:
        return *m_secondTerm;
    default:
        return *this;
    }
}

const shared_ptr<EventExpression> EventExpression::secondTermAsPtr() {
    switch ( m_type ) {
    case AND:
    case OR:
        return m_secondTerm;
    default:
        return enable_shared_from_this<EventExpression>::shared_from_this();
    }
}

void EventExpression::reprime() {
    // Reset pointer to event
    m_trigger_event.reset();  // = nullptr;
    m_trigger_timestamp = std::nan("1");  // Set to NaN for safety
    if (m_firstTerm)
        m_firstTerm.get()->reprime();
    if (m_secondTerm)
        m_secondTerm.get()->reprime();
}

/**
* Overloads the << operator
*/
std::ostream& dynaa::operator<<(std::ostream &strm, const EventExpression &expression) {
    if (auto entity = expression.m_entity.lock()) {
        if (entity == Event::ANY_SOURCE) {
            return strm << "(ANY_SOURCE," << expression.m_evType << "," << expression.m_id << ")";
        }
        return strm << "(" << entity << "," << expression.m_evType << "," << expression.m_id << ")";
    }
    return strm << "(?," << expression.m_evType << "," << expression.m_id << ")";
}

std::ostream& dynaa::operator<<(std::ostream &strm, const shared_ptr<EventExpression>& expression) {
    if (expression) {
        return strm << *expression ;
    } else {
        return strm << " <null> pointer ";
    }
}

EventExpression EventExpression::operator&&(const EventExpression& second) {
    EventExpression result;
    result.m_type = AND;
    result.m_firstTerm = make_shared<EventExpression>(*this);
    result.m_secondTerm = make_shared<EventExpression>(second);
    return result;
}

// Creates a new expression so want to always give shared pointer
shared_ptr<EventExpression> EventExpression::logicalAnd(const shared_ptr<EventExpression>& second) {
    return make_shared<EventExpression>(*this && *second);
}

EventExpression EventExpression::operator||(const EventExpression& second) {
    EventExpression result;
    result.m_type = OR;
    result.m_firstTerm = make_shared<EventExpression>(*this);
    result.m_secondTerm = make_shared<EventExpression>(second);
    return result;
}

shared_ptr<EventExpression> EventExpression::logicalOr(const shared_ptr<EventExpression>& second) {
    return make_shared<EventExpression>(*this || *second);
}

bool EventExpression::operator==(const EventExpression& second) const {
    bool sameType = (m_type == second.m_type);
    if (!sameType)
        return false;
    switch ( m_type ) {
    case Atomic: {
        bool sameSource = (m_entity.lock() == second.m_entity.lock());
        bool sameEventType = (m_evType == second.m_evType);
        bool sameEventId = (m_id == second.m_id);
        return sameSource && sameEventType && sameEventId;
    }
    case AND:
    case OR:
        bool sameFirstTerm = (*m_firstTerm == *(second.m_firstTerm));
        bool sameSecondTerm = (*m_secondTerm == *(second.m_secondTerm));
        return sameFirstTerm && sameSecondTerm;
    }
    return false;
}

bool EventExpression::operator!=(const EventExpression& second) const {
    return !(*this == second);
}
