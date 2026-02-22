/*
file: eventexpression.cpp
author: Julio Oliveira, TNO

Member function of the class EventExpression
*/

#include "../include/eventexpression.h"
#include "../include/entity.h"

using namespace std;
using namespace dynaa;

const EventExpression EventExpression::any_event(Event::ANY_SOURCE, Event::ANY_TYPE, Event::ANY_ID);

EventExpression::EventExpression(string name, const EventSource& entity, EventType evType,
                                 EventID id) :
    m_descriptor(make_shared<string>(std::move(name))),
    m_entity(entity),
    m_entity_id(entity.lock() ? entity.lock()->getUID() : -1L),
    m_evType(std::move(evType)),
    m_id(id),
    m_type(Atomic) {}

EventExpression::EventExpression(const EventSource& entity, EventType evType, EventID id) :
    m_entity(entity),
    m_entity_id(entity.lock() ? entity.lock()->getUID() : -1L),
    m_evType(std::move(evType)),
    m_id(id),
    m_type(Atomic) {}

EventExpression::EventExpression(const EventExpression &evExpression) :
    m_descriptor(evExpression.m_descriptor),
    m_entity(evExpression.m_entity),
    m_entity_id(evExpression.m_entity_id),
    m_evType(evExpression.m_evType),
    m_id(evExpression.m_id),
    m_type(evExpression.m_type) {
    if (evExpression.m_type != Atomic) {
        if(evExpression.m_firstTerm) {
            m_firstTerm = evExpression.m_firstTerm;
        }
        if(evExpression.m_secondTerm) {
            m_secondTerm = evExpression.m_secondTerm;
        }
    }
}

EventExpression::EventExpression(const Event &event) {
    try {
        m_entity = event.source();
        m_entity_id = event.source()->getUID();
    } catch(InvalidEventSourceException & e) {
        m_entity = EventSource();
        m_entity_id = -1L;
    }
    m_evType = event.type();
    m_id = event.id();
    m_type = Atomic;
};

EventExpression::~EventExpression() {
    if (m_firstTerm) {
        m_firstTerm.reset(); // This reset is from the shared pointer that holds the term.  Releasing it.
    }
    if (m_secondTerm) {
        m_secondTerm.reset();// This reset is from the shared pointer that holds the term.  Releasing it.
    }
    if (m_descriptor) {
        m_descriptor.reset();
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

EventSourceID EventExpression::atomicEventSourceID() const {
    return m_entity_id;
}

EventExpressionType EventExpression::type() const {
    return m_type;
}

const string EventExpression::descriptor() const {
    if (m_descriptor) {
        return *m_descriptor;
    }
    return string();
}

int EventExpression::numberOfAtomicExpressions() const {
    switch ( m_type ) {
    case AND:
    case OR:
        return m_firstTerm->numberOfAtomicExpressions() + m_secondTerm->numberOfAtomicExpressions();
    default:
        return 1;
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

void EventExpression::atoms(vector<EventExpression> &collector) const {
    if (m_type == Atomic) {
        collector.push_back(*this);
    } else {
        m_firstTerm->atoms(collector);
        m_secondTerm->atoms(collector);
    }
}

vector<EventExpression > EventExpression::atoms() const {
    auto collector = vector<EventExpression >();
    atoms(collector);
    return collector;
}

bool EventExpression::isFullyQualified() const {
    if (m_type != Atomic) {
        return false;
    }
    return (m_evType != Event::ANY_TYPE && m_entity_id != Event::ANY_SOURCE_ID && m_id != Event::ANY_ID);
}

/**
* Overloads the << operator
*/
std::ostream& dynaa::operator<<(std::ostream &strm, const EventExpression &expression) {
    if (auto entity = expression.m_entity.lock()) {
        if (entity == Event::ANY_SOURCE) {
            return strm << expression.m_descriptor << "(ANY_SOURCE," << expression.m_evType << "," << expression.m_id << ")";
        }
        return strm << expression.m_descriptor << "(" << entity << "," << expression.m_evType << "," << expression.m_id << ")";
    }
    return strm << expression.m_descriptor << "(?," << expression.m_evType << "," << expression.m_id << ")";
}

EventExpression EventExpression::operator&&(EventExpression second) const {
    EventExpression result;
    result.m_type = AND;
    result.m_firstTerm = make_shared<EventExpression>(*this);
    result.m_secondTerm = make_shared<EventExpression>(std::move(second));
    return result;
}

EventExpression dynaa::operator&&(const Event& event, const EventExpression& ee) {
    return EventExpression(event) && ee;
}

EventExpression EventExpression::operator||(EventExpression second) const {
    EventExpression result;
    result.m_type = OR;
    result.m_firstTerm = make_shared<EventExpression>(*this);
    result.m_secondTerm = make_shared<EventExpression>(std::move(second));
    return result;
}

EventExpression dynaa::operator||(const Event& event, const EventExpression& ee) {
    return EventExpression(event) || ee;
}

bool EventExpression::operator==(const EventExpression& second) const {
    bool sameType = (m_type == second.m_type);
    if (!sameType) {
        return false;
    }
    switch ( m_type ) {
    case Atomic: {
        bool sameSource = (m_entity_id == second.m_entity_id);
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