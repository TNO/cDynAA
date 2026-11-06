/*
file: eventhandler.cpp
*/
#include <string>
#include "../include/expressionhandler.h"
#include <memory>

using namespace dynaa;
using namespace std;


// This occurs if the ExpressionHandler is cast and used an EventHandler
// In this case convert the event to an event expression
void ExpressionHandler::handle(Event event) {
    shared_ptr<EventExpression> atomicEventExpr = EventExpression::wrapTriggeredEvent(event);
    handle(atomicEventExpr);
}

// Helper function to append all triggered to a vector
void appendTriggeredEvents(const shared_ptr<EventExpression>& evExpr, std::vector<Event>& triggeredEvents) {
    if (evExpr->type() == Atomic) {
        triggeredEvents.push_back(move(*evExpr->triggeredEvent()));
    } else {
        appendTriggeredEvents(evExpr->firstTermAsPtr(), triggeredEvents);
        appendTriggeredEvents(evExpr->secondTermAsPtr(), triggeredEvents);
    }
}

void ExpressionHandler::handle(shared_ptr<EventExpression> evExpr) const {
    if (!evExpr->value()) {
        // Only handle triggered event expressions
        return;
    }
    if (all_of(safe_guards.begin(), safe_guards.end(), EventHandler::safeCheck)) {
        if (m_exprCallback != nullptr) {
            // Call the event expression callback
            m_exprCallback(evExpr);
        } else if (m_eventCallback != nullptr) {
            // Call the event callback with one of the triggered events
            vector<Event> triggeredEvents = vector<Event>();
            appendTriggeredEvents(evExpr, triggeredEvents);
            if (!triggeredEvents.empty()) {
                m_eventCallback(triggeredEvents.front());
            }
        }
    } else {
        throw InvalidHandler(); // TODO: Remove after logging is in place, substitute by logging message
    }
}

bool ExpressionHandler::operator< (const ExpressionHandler& other) const {
    if (priority == other.priority) {
        return id < other.id;
    }
    return priority < other.priority;
}
