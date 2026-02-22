/*
file: entity.cpp

Implementation file for the class Entity
*/

#include <stdexcept>
#include <sstream>

#include "../include/entity.h"
#include "../include/coreexceptions.h"

using namespace dynaa;
using namespace std;

long Entity::m_idseed = 1L;

const string Entity::ENTITY_PREFIX = "entity";

long Entity::getUID() const {
    return m_uid;
}


void Entity::dismiss(const HandlerToken& token)  {
    dismissHandler(token);
}

void Entity::dismissHandler(const HandlerToken& token)  {
    hRegistry.unregisterHandler(token);
}

EventExpression Entity::scheduleAfter(const SimTime interval, const EventType &eventType) {
    const Event event = Event(shared_from_this(), eventType);
    scheduler.scheduleAfter(interval, event);
    return EventExpression(event);
}

EventExpression Entity::scheduleAt(const SimTime instant, const EventType &eventType) {
    const Event event = Event(shared_from_this(), eventType);
    scheduler.scheduleAt(instant, event);
    return EventExpression(event);
}

EventExpression Entity::scheduleNow(const EventType &eventType) {
    const Event event = Event(shared_from_this(), eventType);
    scheduler.scheduleNow(event);
    return EventExpression(event);
}

int Entity::unschedule(const EventType& type, const EventID id) {
    return scheduler.unschedule(m_uid, type, id);
}

HandlerToken Entity::wait(const shared_ptr<EventHandler> &eventHandler, const EventType &eventType,
                          const long id) const {
    return wait(eventHandler, EventExpression(Event::ANY_SOURCE, eventType, id));
}

HandlerToken Entity::wait(const shared_ptr<EventHandler> &eventHandler, const EventSource &entity,
                          const EventType &eventType, const long id) const {
    return wait(eventHandler, EventExpression(entity, eventType, id));
}

HandlerToken Entity::wait(const shared_ptr<EventHandler> &eventHandler, const long id) const {
    return wait(eventHandler, EventExpression(Event::ANY_SOURCE, Event::ANY_TYPE, id));
}

HandlerToken Entity::wait(const shared_ptr<EventHandler> &eventHandler, const EventExpression& evExpr) const {
    return hRegistry.registerHandler(eventHandler, evExpr, false);
}

HandlerToken Entity::wait(const shared_ptr<EventHandler> &eventHandler, const EventSource& entity,
                          const long id) const {
    return wait(eventHandler, EventExpression(entity, Event::ANY_TYPE, id));
}

HandlerToken Entity::waitOnce(const shared_ptr<EventHandler> &eventHandler, const EventType &eventType,
                              const long id) const {
    return waitOnce(eventHandler, EventExpression(Event::ANY_SOURCE, eventType, id));
}

HandlerToken Entity::waitOnce(const shared_ptr<EventHandler> &eventHandler, const long id) const {
    return waitOnce(eventHandler, EventExpression(Event::ANY_SOURCE, Event::ANY_TYPE, id));
}

HandlerToken Entity::waitOnce(const shared_ptr<EventHandler> &eventHandler, const EventSource &entity,
                              const long id) const {
    return waitOnce(eventHandler, EventExpression(entity, Event::ANY_TYPE, id));
}

HandlerToken Entity::waitOnce(const shared_ptr<EventHandler> &eventHandler, const EventSource &entity,
                              const EventType &eventType,
                              const long id) const {
    return waitOnce(eventHandler, EventExpression(entity, eventType, id));
}

HandlerToken Entity::waitOnce(const shared_ptr<EventHandler> &eventHandler, const EventExpression& evExpr) const {
    return hRegistry.registerHandler(eventHandler, evExpr, true);
}

/**
* Overrides the comparison (equality) operator
*/
bool Entity::operator==(const Entity &other) const {
    return  m_uid == other.m_uid;
}

/**
* Overrides the comparison (difference) operator
*/
bool Entity::operator!=(const Entity &other) const {
    return !(*this == other);
}

/**
* Overrides the comparison (less than) operator
*/
bool Entity::operator<(const Entity &other) const {
    return !(m_uid < other.m_uid);
}

/**
* Overloads the << operator
*/
std::ostream &dynaa::operator<<(std::ostream &strm, const Entity &entity) {
    return strm << Entity::ENTITY_PREFIX << "_" << entity.getUID();
}
