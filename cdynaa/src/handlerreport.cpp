/*
file: handlerreport.cpp
*/
#include "../include/handlerreport.h"
#include <string>
#include <algorithm>
#include <memory>

using namespace dynaa;
using namespace std;

bool HandlerReport::EventReport::operator==(const HandlerReport::EventReport& other) const {
    return (atom_ee == other.atom_ee) &&
           (value == other.value) &&
           (last_trigger == other.last_trigger) &&
           (trigger_time == trigger_time);
}

EventExpression HandlerReport::getEventExpression() const {
    return m_event_expression;
}

HandlerToken HandlerReport::getHandlerToken() const {
    return m_handlerToken;
}

int HandlerReport::getHandlerPriority() const {
    return m_handler_priority;
}

bool HandlerReport::once() const {
    return m_handler_once;
}

vector<HandlerReport::EventReport> HandlerReport::getEventStateReport() const {
    return m_event_states;
}

std::unordered_map<EventExpression, HandlerReport::EventReport, EventExpressionHash>
HandlerReport::getEventStateReportAsMap() const {
    std::unordered_map<EventExpression, HandlerReport::EventReport, EventExpressionHash> res;
    for (auto& atom_state: m_event_states) {
        res.emplace(atom_state.atom_ee, atom_state);
    }
    return res;
}