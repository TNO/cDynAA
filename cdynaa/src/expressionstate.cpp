/*
file: expressionstate.cpp
*/
#include <memory>
#include <string>
#include "../include/expressionstate.h"
#include "../include/entity.h"
#include "../include/diagnostics.h"
//#include <memory>


using namespace dynaa;
using namespace std;

const SimTime invalidTime = -1;
const Event invalidEvent;

/**
 * Factory to create the correct type of EventExpressionState
 */
std::shared_ptr<EventExpressionState> dynaa::createExpressionState(
    const EventExpression& event_expression) {
    if (event_expression.type() == Atomic) {
        return std::make_shared<AtomicExpressionState>(event_expression);
    }
    return std::make_shared<GeneralExpressionState>(event_expression);
}


/**
 * Implementation block of a complex event expression state
 *
 * GeneralExpressionState
 *
 */
bool GeneralExpressionState::evaluateExpressionState() const {
    if (m_event_expression.type()== Atomic) {
        auto single_state = states.begin();
        return single_state->triggered;
    }
    return evalState();
}

const EventExpression& GeneralExpressionState::getEventExpression() const {
    return m_event_expression;
}

std::vector<EventMask> GeneralExpressionState::getEventMasks() const {
    std::vector<EventMask> masks(states.size());
    std::transform(states.begin(), states.end(), masks.begin(),
                   [](const AtomState& astate) -> EventMask { return astate.mask; });
    return masks;
}

void GeneralExpressionState::reset() {
    for (auto& atomState : states) {
        atomState.trigger_event = invalidEvent;
        atomState.trigger_time = invalidTime;
        atomState.triggered = false;
    }
}

void GeneralExpressionState::update(const EventMask& triggeringMask,
                                    const Event& trigger_event, SimTime trigger_time) {
    for (AtomState& atomState : states) {
        if (EventMask::matches(atomState.mask,triggeringMask) && !atomState.triggered) {
            // Update mask matches and no previous event
            atomState.trigger_event = trigger_event;
            atomState.trigger_time = trigger_time;
            atomState.triggered = true;
            Diagnostics::getInstance().incrementTriggeredAtomExprCounter();
        }
    }
}

vector<HandlerReport::EventReport> GeneralExpressionState::getStateReport() const {
    vector<HandlerReport::EventReport> statesReport;
    statesReport.reserve(states.size());
    for(const auto& atom_state : states) {
        statesReport.push_back(AtomState::stateReportOf(atom_state));
    }
    return statesReport;
}

HandlerReport::EventReport AtomState::stateReportOf(const AtomState& atom_state) {
    return {atom_state.atom_ee,
            atom_state.triggered,
            atom_state.triggered ? atom_state.trigger_event : EventExpression::any_event,
            atom_state.trigger_time};

}

// Private and helper methods
AtomState AtomState::fromAtom(EventExpression atom_ee) {
    if (atom_ee.type() != EventExpressionType::Atomic) {
        throw invalid_argument("Can only use atomic event expressions for creating atomic states.");
    }
    EventMask event_mask = EventMask::generateMask(atom_ee.atomicEventSourceID(),
                           atom_ee.atomicEventType(),
                           atom_ee.atomicEventID());
    return {std::move(atom_ee), //Moves the atomic expression avoiding copies.
            event_mask,
            false,
            invalidEvent,
            invalidTime};
}

vector<AtomState> GeneralExpressionState::createStatesFromExpression(const EventExpression& expression) {
    auto atoms = expression.atoms();
    std::vector<AtomState> states(atoms.size());
    // Move iterators make sure that the atomic expressions are moved, not copied, when generating the
    // AtomState
    std::transform(make_move_iterator(atoms.begin()),
                   make_move_iterator(atoms.end()),
                   states.begin(), AtomState::fromAtom);
    return states;
}

AtomState& GeneralExpressionState::stateOfAtom(const EventExpression &atom_ee) {
    for (auto &atomState : states) {
        if (atomState.atom_ee == atom_ee) {
            return atomState;
        }
    }
    throw runtime_error("Malformed expression state: state missing for expression.");
}

const function<bool()> GeneralExpressionState::createEvalFunction(const EventExpression& expression) {
    // Creates on-the-fly a functional object to evaluate the expression.
    function<bool()> evaluationFunction;
    // If expression is atomic, it evaluates to true if the unique atom state is triggered.
    if (expression.type() == Atomic) {
        const AtomState* atomState = &stateOfAtom(expression);
        evaluationFunction = [atomState]() {
            return atomState->triggered;
        };
        // If expression is of form A <op> B, it evaluates to A->triggered <op> B-->triggered.  <op> can be && or ||.
    } else if ((expression.firstTerm().type() == Atomic) && (expression.secondTerm().type() == Atomic)) {
        const AtomState* atomState_first = &stateOfAtom(expression.firstTerm());
        const AtomState* atomState_second = &stateOfAtom(expression.secondTerm());
        if (expression.type() == AND) {
            evaluationFunction = [atomState_first, atomState_second]() {
                return atomState_first->triggered && atomState_second->triggered;
            };
        } else {
            evaluationFunction = [atomState_first, atomState_second]() {
                return atomState_first->triggered || atomState_second->triggered;
            };
        }
        // If expression is of form A <op> (B <op> C), it evaluates to A->triggered <op> (B <op> C).  <op> can be && or ||.
    } else if (expression.firstTerm().type() == Atomic) {
        const AtomState* atomState_first = &stateOfAtom(expression.firstTerm());
        auto second = createEvalFunction(expression.secondTerm());
        if (expression.type() == AND) {
            evaluationFunction = [atomState_first, second]() {
                return atomState_first->triggered && second();
            };
        } else {
            evaluationFunction = [atomState_first, second]() {
                return atomState_first->triggered || second();
            };
        }
        // If expression is of form (A <op> B) <op> C (where C atomic), it evaluates to C->triggered <op> (A <op> B).  <op> can be && or ||.
    } else if (expression.secondTerm().type() == Atomic) {
        auto first = createEvalFunction(expression.firstTerm());
        const AtomState* atomState_second = &stateOfAtom(expression.secondTerm());
        if (expression.type() == AND) {
            evaluationFunction = [first, atomState_second]() {
                return atomState_second->triggered && first();
            };
        } else {
            evaluationFunction = [first, atomState_second]() {
                return atomState_second->triggered || first();
            };
        }
        // else, the functional object cannot be optimized, so use recursion to build it.
        // Its evaluation is the evaluation of the first term operated <&& or ||> with the
        // evaluation of the second term.
    } else {
        auto first = createEvalFunction(expression.firstTerm());
        auto second = createEvalFunction(expression.secondTerm());
        if (expression.type() == AND) {
            evaluationFunction = [first, second]() {
                return first() && second();
            };
        }
        if (expression.type() == OR) {
            evaluationFunction = [first, second]() {
                return first() || second();
            };
        }
    }
    return evaluationFunction;
}

/**
 * Implementation block of a atomic event expression state
 *
 * AtomicExpressionState
 */

bool AtomicExpressionState::evaluateExpressionState() const {
    return state.triggered;
}

const EventExpression& AtomicExpressionState::getEventExpression() const {
    return state.atom_ee;
}

std::vector<EventMask> AtomicExpressionState::getEventMasks() const {
    return std::vector<EventMask>(1,state.mask);
}

void AtomicExpressionState::reset() {
    state.trigger_event = invalidEvent;
    state.trigger_time = invalidTime;
    state.triggered = false;
}

void AtomicExpressionState::update(const EventMask& triggeringMask,
                                   const Event& trigger_event, SimTime trigger_time) {
    if (EventMask::matches(state.mask,triggeringMask) && !state.triggered) {
        // Updates if mask matches and no previous event
        state.trigger_event = trigger_event;
        state.trigger_time = trigger_time;
        state.triggered = true;
    }
}

vector<HandlerReport::EventReport> AtomicExpressionState::getStateReport() const {
    std::vector<HandlerReport::EventReport> stateReport;
    stateReport.insert(stateReport.begin(),AtomState::stateReportOf(state));
    return stateReport;
}

