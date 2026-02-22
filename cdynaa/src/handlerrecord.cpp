/*
Member functions of the class HandlerRecord
*/

#include "../include/handlerrecord.h"
#include "../include/entity.h"
#include "../include/expressionstate.h"

#include <iostream>

using namespace std;
using namespace dynaa;

long HandlerRecord::m_idseed = 1L;
bool initialEvaluationFunction() {
    return false;
}

HandlerToken HandlerRecord::getUID() const {
    return m_record_uid;
}

EventExpression HandlerRecord::getEventExpression() const {
    return m_expression_state->getEventExpression();
}

std::shared_ptr<EventHandler> HandlerRecord::getEventHandler() const {
    return m_event_handler;
}

std::vector<EventMask> HandlerRecord::getEventMasks() const {
    return m_expression_state->getEventMasks();
}

bool HandlerRecord::triggered() const {
    return m_expression_state->evaluateExpressionState();
}

void HandlerRecord::reset() {
    if (m_valid) {
        m_expression_state->reset();  // ATTENTION: We mean to reset the state, NOT the pointer to the state.
    }
}

void HandlerRecord::invalidate() {
    m_valid = false;
}

bool HandlerRecord::is_valid() const {
    return m_valid;
}

bool HandlerRecord::is_once() const {
    return m_once;
}

void HandlerRecord::set_once(bool once_flag) {
    m_once = once_flag;
}

void HandlerRecord::update(const Event& trigger_event, SimTime trigger_time) {
    auto const source = trigger_event.source();
    if (!source) return; // Cannot update if source does not exist. //CHECK: should we signalize?
    m_expression_state->update({source->getUID(), trigger_event.type().id(), trigger_event.id()},
                               trigger_event, trigger_time);
}

HandlerReport HandlerRecord::createEventReport() const {
    return {m_record_uid,
            m_expression_state->getEventExpression(),
            m_event_handler->getPriorityLevel(),
            m_once,
            m_expression_state->getStateReport()};
}

