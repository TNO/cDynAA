/*
file: eventhandler.cpp
*/
#include "../include/eventhandler.h"
#include <string>
#include <algorithm>
#include <memory>

using namespace dynaa;
using namespace std;


long EventHandler::m_handlerSequenceNumber = 1;
const int EventHandler::DEFAULT_PRIORITY_LEVEL = 5;
const int EventHandler::HIGHEST_PRIORITY_LEVEL = 1;
const int EventHandler::LOWEST_PRIORITY_LEVEL = 10;

shared_ptr<EventHandler> EventHandler::create(function<void(Event)> callback) {
    return make_shared<EventHandler>(callback);
}

shared_ptr<EventHandler> EventHandler::create(function<void(Event)> callback, int priority) {
    return make_shared<EventHandler>(callback, priority);
}

shared_ptr<EventHandler> EventHandler::create(function<void(Event)> callback, string identifier) {
    return make_shared<EventHandler>(callback, identifier);
}

shared_ptr<EventHandler> EventHandler::create(function<void(Event)> callback, string identifier, int priority) {
    return make_shared<EventHandler>(callback, identifier, priority);
}

shared_ptr<EventHandler> EventHandler::create(function<void(Event)> callback, string identifier,
        vector<weak_ptr<Entity>> safe_guards) {
    return make_shared<EventHandler>(callback, identifier, safe_guards);
}

shared_ptr<EventHandler> EventHandler::create(function<void(Event)> callback, string identifier,
        vector<weak_ptr<Entity>> safe_guards, int priority) {
    return make_shared<EventHandler>(callback, identifier, safe_guards, priority);
}

shared_ptr<EventHandler> EventHandler::create(function<void(Event)> callback,
        vector<weak_ptr<Entity>> safe_guards) {
    return make_shared<EventHandler>(callback, safe_guards);
}

shared_ptr<EventHandler> EventHandler::create(function<void(Event)> callback,
        vector<weak_ptr<Entity>> safe_guards, int priority) {
    return make_shared<EventHandler>(callback, safe_guards, priority);
}

bool EventHandler::safeCheck(weak_ptr<Entity> safe_guard) {
    return !safe_guard.expired();
}

void EventHandler::handle(Event event) {
    if (all_of(safe_guards.begin(), safe_guards.end(), EventHandler::safeCheck)) {
        if (m_eventCallback != nullptr) {
            m_eventCallback(event);
        }
    } else {
        throw InvalidHandler(); // TODO: Remove after logging is in place, substitute by logging message
    }
};

const string EventHandler::getIdentifier() {
    return identifier;
};

//bool EventHandler::isOnce() const {
//    return once;
//}

int EventHandler::getPriorityLevel() const {
    return priority;
}

bool EventHandler::operator<(const EventHandler &other) const {
    //return id < other.id;
    if (priority == other.priority) {
        return id < other.id;
    }
    return priority < other.priority;
}
