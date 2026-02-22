/*
file: event.cpp
author: Julio Oliveira, Coen van Leeuwen, TNO

Member function of the class Event
*/

#include "../include/entity.h"
#include "../include/event.h"

#include <iostream>

using namespace std;
using namespace dynaa;

EventID Event::m_idSequenceNumber = 1;
const EventType Event::INVALID_EVENT_TYPE("INVALID_EVENT_TYPE", "Events with invalid type.");
const EventType Event::ANY_TYPE("ANY_EVENT", "Type representing all event types");
shared_ptr<Entity> Event::ANY_SOURCE = make_shared<Entity>();
const EventID Event::ANY_ID = -1;
const long Event::ANY_SOURCE_ID = Event::ANY_SOURCE->getUID();

EventID Event::id() const {
    return m_id;
}

shared_ptr<Entity> Event::source() const {
    if (auto source = m_source.lock()) {
        return source;
    }
    throw InvalidEventSourceException();
}

const EventType& Event::type() const {
    return m_type;
}

/**
* Overrides the comparison (equality) operator
*/
bool Event::operator==(const Event &other) const {
    return (m_id == other.m_id) &&
           (m_source.lock() == other.m_source.lock()) &&
           (m_type == other.m_type);
}

/**
* Overrides the comparison (difference) operator
*/
bool Event::operator!=(const Event &other) const {
    return !(*this == other);
}

/*
* Available mainly to allow Events to be used in data structures as keys
*/
bool Event::operator<(const Event &other) const {
    if (m_id == other.m_id) {
        if (*m_source.lock() == *other.m_source.lock()) {
            return (m_type.id() < other.m_type.id());
        }
        return (*m_source.lock() < *other.m_source.lock());
    }
    return (m_id < other.m_id);
}

/**
* Overloads the << operator
*/
std::ostream& dynaa::operator<<(std::ostream &strm, const Event &ev) {
    return strm << "ev_" << ev.id();
}

Event Event::eventFactory(EventSource source, const EventType& type) {
    return Event(source, type);
}
