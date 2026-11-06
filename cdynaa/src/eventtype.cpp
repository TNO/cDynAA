/*
file: eventtype.cpp
author: Julio Oliveira, Coen van Leeuwen, TNO

*/

#include "../include/eventtype.h"

#include <iostream>

using namespace dynaa;
using namespace std;

long EventType::m_typeSequenceNumber = 1;

string EventType::name() const {
    return m_name;
};

string EventType::description() const {
    return m_description;
};

long EventType::id() const {
    return m_id;
};

EventType EventType::create(const string& name, const string& description) {
    return EventType(name, description);
};


/**
* Overrides the comparison (equality) operator
*/
bool EventType::operator==(const EventType &other) const {
    return (m_id == other.m_id);
}

/**
* Overrides the comparison (difference) operator
*/
bool EventType::operator!=(const EventType &other) const {
    return !(*this == other);
}

/**
* Overrides the << operator
*/
std::ostream& dynaa::operator<<(std::ostream &strm, const EventType &ev) {
    return strm << ev.name() << "_" << ev.m_id;
}

