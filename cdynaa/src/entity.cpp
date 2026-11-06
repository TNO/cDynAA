/*
file: entitiy.cpp
author: Julio Oliveira, Coen van Leeuwen, TNO

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

const EventType dummyEvent = EventType::create("DUMMY", "DUMMYTYPE");

long Entity::getUID() const {
    return m_uid;
}


void Entity::dismiss(shared_ptr<EventHandler> eventHandler)  {
    dismissHandler(eventHandler, Event::ANY_SOURCE, Event::ANY_TYPE, Event::ANY_ID);
}

void Entity::dismiss(shared_ptr<EventHandler> eventHandler, const EventSource source)  {
    dismissHandler(eventHandler, source, Event::ANY_TYPE, Event::ANY_ID);
}

void Entity::dismiss(shared_ptr<EventHandler> eventHandler, const EventType &eventType)  {
    dismissHandler(eventHandler, Event::ANY_SOURCE, eventType, Event::ANY_ID);
}

void Entity::dismiss(shared_ptr<EventHandler> eventHandler, const EventType &eventType, const long id)  {
    dismissHandler(eventHandler, Event::ANY_SOURCE, eventType, Event::ANY_ID);
}

void Entity::dismiss(shared_ptr<EventHandler> eventHandler, const Event &event)  {
    dismissHandler(eventHandler, event.source(), event.type(), event.id());
}

void Entity::dismiss(shared_ptr<EventHandler> eventHandler, const EventSource source, const EventType &eventType)  {
    dismissHandler(eventHandler, source, eventType, Event::ANY_ID);
}

void Entity::dismiss(shared_ptr<EventHandler> eventHandler, const EventSource source, const EventType &eventType,
                     const long id) {
    dismissHandler(eventHandler, source, eventType, id);
}

void Entity::dismissHandler(shared_ptr<EventHandler> eventHandler, const EventSource source, const EventType &eventType,
                            const long eventId)  {
    hRegistry.unregisterHandler(eventHandler, source, eventType, eventId);
}

void Entity::dismiss(const shared_ptr<ExpressionHandler>& expressionHandler,
                     const shared_ptr<EventExpression>& evExpr) {
    hRegistry.unregisterHandler(expressionHandler, evExpr);
}

void Entity::dismiss(const shared_ptr<ExpressionHandler>& expressionHandler) {
    hRegistry.unregisterHandler(expressionHandler, nullptr);
}

Event Entity::scheduleAfter(const SimTime interval, const EventType &eventType) {
    Event event = Event(shared_from_this(), eventType);
    scheduler.scheduleAfter(interval, event);
    return event;
}

Event Entity::scheduleAt(const SimTime instant, const EventType &eventType) {
    Event event = Event(shared_from_this(), eventType);
    scheduler.scheduleAt(instant, event);
    return event;
}

Event Entity::scheduleNow(const EventType &eventType) {
    Event event = Event(shared_from_this(), eventType);
    scheduler.scheduleNow(event);
    return event;
}

Event Entity::scheduleAfter(const SimTime interval, const shared_ptr<EventType>& eventType) {
    Event event = Event(shared_from_this(), eventType);
    scheduler.scheduleAfter(interval, event);
    return event;
}

Event Entity::scheduleAt(const SimTime instant, const shared_ptr<EventType>& eventType) {
    Event event = Event(shared_from_this(), eventType);
    scheduler.scheduleAt(instant, event);
    return event;
}

Event Entity::scheduleNow(const shared_ptr<EventType>& eventType) {
    Event event = Event(shared_from_this(), eventType);
    scheduler.scheduleNow(event);
    return event;
}

void Entity::wait(shared_ptr<EventHandler> eventHandler) const {
        waitOnEvent(eventHandler, Event::ANY_SOURCE, Event::ANY_TYPE, Event::ANY_ID, false);
}

void Entity::wait(shared_ptr<EventHandler> eventHandler, const EventSource entity) const {
        waitOnEvent(eventHandler, entity, Event::ANY_TYPE, Event::ANY_ID, false);
}

void Entity::wait(shared_ptr<EventHandler> eventHandler, const EventType &eventType, const long id) const {
        waitOnEvent(eventHandler, Event::ANY_SOURCE, eventType, id, false);
}

void Entity::wait(shared_ptr<EventHandler> eventHandler, const EventSource entity,
                  const EventType &eventType, const long id) const {
        waitOnEvent(eventHandler, entity, eventType, Event::ANY_ID, false);
}

void Entity::wait(shared_ptr<EventHandler> eventHandler, const EventSource entity, const EventType &eventType) const {
        waitOnEvent(eventHandler, entity, eventType, Event::ANY_ID, false);
}

void Entity::wait(shared_ptr<EventHandler> eventHandler, const EventType &eventType) const {
        waitOnEvent(eventHandler, Event::ANY_SOURCE, eventType, Event::ANY_ID, false);
}

void Entity::wait(shared_ptr<EventHandler> eventHandler, const long id) const {
        waitOnEvent(eventHandler, Event::ANY_SOURCE, Event::ANY_TYPE, id, false);
}

void Entity::wait(shared_ptr<ExpressionHandler> expressionHandler, shared_ptr<EventExpression> evExpr) const {
    bool reprime = true;
    hRegistry.registerHandler(expressionHandler, evExpr, false, reprime);
}

void Entity::wait(shared_ptr<EventHandler> eventHandler, const EventSource entity, const long id) const {
        waitOnEvent(eventHandler, entity, Event::ANY_TYPE, id, false);
}

void Entity::waitOnce(shared_ptr<EventHandler> eventHandler, const EventType &eventType) const {
        waitOnEvent(eventHandler, Event::ANY_SOURCE, eventType, Event::ANY_ID, true);
}

void Entity::waitOnce(shared_ptr<EventHandler> eventHandler, const EventSource entity,
                      const EventType &eventType) const {
        waitOnEvent(eventHandler, entity, eventType, Event::ANY_ID, true);
}

void Entity::waitOnce(shared_ptr<EventHandler> eventHandler, const EventSource entity) const {
        waitOnEvent(eventHandler, entity, Event::ANY_TYPE, Event::ANY_ID, true);
}

void Entity::waitOnce(shared_ptr<EventHandler> eventHandler, const EventType &eventType, const long id) const {
        waitOnEvent(eventHandler, Event::ANY_SOURCE, eventType, id, true);
}

void Entity::waitOnce(shared_ptr<EventHandler> eventHandler, const Event &event) const {
        waitOnEvent(eventHandler, event.source(), event.type(), event.id(), true);
}

void Entity::waitOnce(shared_ptr<EventHandler> eventHandler, const long id) const {
        waitOnEvent(eventHandler, Event::ANY_SOURCE, Event::ANY_TYPE, id, true);
}

void Entity::waitOnce(shared_ptr<EventHandler> eventHandler) const {
        waitOnEvent(eventHandler, Event::ANY_SOURCE, Event::ANY_TYPE, Event::ANY_ID, true);
}

void Entity::waitOnce(shared_ptr<EventHandler> eventHandler, const EventSource entity, const long id) const {
        waitOnEvent(eventHandler, entity, Event::ANY_TYPE, id, true);
}

void Entity::waitOnce(shared_ptr<EventHandler> eventHandler, const EventSource entity, const EventType &eventType,
                      const long id) const {
        waitOnEvent(eventHandler, entity, eventType, id, true);
}

void Entity::waitOnce(shared_ptr<ExpressionHandler> expressionHandler, shared_ptr<EventExpression> evExpr) const {
    bool reprime = true;
    hRegistry.registerHandler(expressionHandler, evExpr, true, reprime);
}

void Entity::waitOnEvent(shared_ptr<EventHandler> eventHandler, const EventSource source, const EventType &eventType,
                         long eventId, bool once) const {
    try {
        hRegistry.registerHandler(eventHandler, source, eventType, eventId, once);
    } catch (...) {
        std::stringstream ss;
        ss << "Could not register handler " << eventHandler << 
                " to wait on source ";
        if(auto source_ref = source.lock()) {
            ss << *source_ref;
        } else {
            ss << "unknown";
        }
        ss << ", event type " << eventType << ", event id " << eventId;
        std::string exception_message = ss.str();
        std::throw_with_nested(std::runtime_error(exception_message));
    }
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
* Overloads the << operator
*/
std::ostream &dynaa::operator<<(std::ostream &strm, const Entity &entity) {
    return strm << Entity::ENTITY_PREFIX << "_" << entity.getUID();
}
