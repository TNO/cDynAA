// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_EXPRESSIONSTATE_
#define INCLUDED_EXPRESSIONSTATE_

#include "eventexpression.h"
#include "eventmask.h"
#include "handlerreport.h"

#include <functional>

/**
 * This file declares two possible implementations for the
 * EventExpressionState interface (@see handlerrecord).
 *
 * The first implementation (AtomicExpressionState) is an optimized state when the event expression is
 * an atomic expression. In this case, it is not necessary to produce
 * evaluation functions nor store vectors.
 *
 * The second implementation (GeneralExpressionState) is an optimized state register for when we have a
 * complex event expression. Note that GeneralExpressionState can be used for simple atoms, but not
 * otherwise.
 *
 * The internal representation used in this implementations is based on the
 * AtomState structure.
 */

namespace dynaa {

/**
 * Represents the state of one atom in an expression
 */
struct AtomState {
    EventExpression atom_ee;
    EventMask mask;
    bool triggered;
    Event trigger_event;
    SimTime trigger_time;

    /**
     * Factory creation for AtomStates
     */
    static AtomState fromAtom(EventExpression atom_ee);

    /**
     * Conversion to EventReport (report)
     */
    static HandlerReport::EventReport stateReportOf(const AtomState& atom_state);
};

/**
 * EventExpressionState is an interface for objects that manipulate an event expression state.
 *
 * We use two ways to implement the state of an event expression state: a GeneralExpressionState
 * and a AtomicExpressionState.  The first one deals with any possible event expression, while the
 * second one is optimized for carrying the state of simple, atomic expressions.
 *
 * Both implementations use the same interface: the EventExpressionState.
 */
class EventExpressionState {

public:
    virtual ~EventExpressionState() {};
    virtual std::shared_ptr<EventExpressionState> clone() const = 0;

    /**
         * Retrieves the event expression related to this state
         *
         * @return
         *  the event expression related to this state
         */
    virtual const EventExpression& getEventExpression() const = 0;

    /**
         * Returns the masks of every atom in the underlying event expression
         *
         * Use to register these atoms in the registry.
         */
    virtual std::vector<EventMask> getEventMasks() const = 0;

    /**
         * Returns the state in a report form
         *
         * @return
         *      this state (all atomic states) in a report format
         *
         * @see HandlerReport::EventReport for the report form of each atom
         */
    virtual vector<HandlerReport::EventReport> getStateReport() const = 0;

    /**
         * Evaluates the event expression with the actual state
         */
    virtual bool evaluateExpressionState() const = 0;

    /**
         * Updates the state of this record according to the given mask and event.
         *
         * This method should be called upon the triggering of an event.  It updates the
         * state of the event expression in the record accordingly:
         *
         * 1) If the event triggers any (atomic) expression, the state of this expression is updated.
         * 2) Updated means: the event and time are registered as the triggers for the atom.
         * 3) Updated does not take place if the state was already updated before.
         *
         */
    virtual void update(const EventMask& triggeringMask, const Event& trigger_event, SimTime trigger_time) = 0;

    /**
         * Resets this state
         *
         * The state for each atom in the expression is reset: trigger_event is released, trigger_time becomes invalid,
         * and value reset to false;
         *
         * In other words, the state is ready to track the event evolution again.
         */
    virtual void reset() = 0;
};

/**
* Factory to safely create expression states
*/
std::shared_ptr<EventExpressionState> createExpressionState(const EventExpression& event_expression);

/**
 * Represents the state of an expression
 *
 * Can represent states of both atomic and non-atomic expressions
 *
 * @see documentation of virtual functions in EventExpressionState interface
 */
class GeneralExpressionState : public EventExpressionState {
    EventExpression m_event_expression;
    std::vector<AtomState> states;
    function<bool()> evalState;

public:
    GeneralExpressionState(const EventExpression& event_expression)
        : m_event_expression(event_expression)
        , states(createStatesFromExpression(event_expression))
        , evalState(createEvalFunction(event_expression)) {};

    // Default constructors do not work well because evaluation function have to be re-calculated.
    // Copy expression and states, not evalfunction.
    GeneralExpressionState(const GeneralExpressionState& other)
        : m_event_expression(other.m_event_expression)
        , states(other.states)
        , evalState(createEvalFunction(other.m_event_expression)) {};

    // Move expression and states, not eval function.
    GeneralExpressionState(GeneralExpressionState&& other)
        : m_event_expression(std::move(other.m_event_expression))
        , states(std::move(other.states))
        , evalState(createEvalFunction(m_event_expression)) {};

    GeneralExpressionState& operator=(const GeneralExpressionState& other) {
        GeneralExpressionState tmp(other);
        std::swap(*this, tmp);
        evalState = createEvalFunction(m_event_expression);
        return *this;
    }
    GeneralExpressionState& operator=(GeneralExpressionState&& other) {
        GeneralExpressionState tmp(std::move(other));
        std::swap(*this, tmp);
        evalState = createEvalFunction(m_event_expression);
        return *this;
    }

    std::shared_ptr<EventExpressionState> clone() const override {
        return make_shared<GeneralExpressionState>(*this); // Needs a copy constructor.
    }

    const EventExpression& getEventExpression() const override;
    std::vector<EventMask> getEventMasks() const override;
    vector<HandlerReport::EventReport> getStateReport() const override;
    bool evaluateExpressionState() const override;
    void update(const EventMask& triggeringMask, const Event& trigger_event, SimTime trigger_time) override;
    void reset() override;

private:
    /**
     * Helper functions to create state vector from expression
     *
     * @param event_expression
     *      The event expression used to create the states
     * @return
     *      A vector of states for tracking the atoms in the given event expression
     */
    static std::vector<AtomState> createStatesFromExpression(const EventExpression& event_expression);

    /**
     * Creates an evaluation function for the given expression
     *
     * Uses the set of internal states for that.
     *
     * This method creates a dynamic object correctly linked to the corresponding states that
     * efficiently evaluates the total event expression.  It avoids searching the states every
     * time the evaluation is invoked.
     *
     * Important: the evaluation funcion (object) has to be regenerated if the internal atomic
     * vector is re-created or re-allocated.  That happens because the functional object retain
     * references to memory locations in the state vector.
     *
     * @param expression
     *      The expression to be evaluated
     * @return
     *      A function object that when called evaluates the expression with the current state.
     */
    const function<bool()> createEvalFunction(const EventExpression& expression);

    /**
     * Indexes and retrieves the atom state, according to the atom it represents.
     *
     * For internal use only.  Do not expose atomstates directly outside of this class.
     */
    AtomState& stateOfAtom(const EventExpression& atom_ee);
};

/**
 * Represents the state of an atomic expression
 *
 * Cannot be used to represent the state of a non-atomic event expression, but this
 * implementation is optimized for manipulating the state of a single atomic expression
 *
 *
 * @see documentation of virtual functions in EventExpressionState interface
 */
class AtomicExpressionState : public EventExpressionState {
    /**
     * Stores the state of an atomic expression
     * Retains also the event expression itself.
     */
    AtomState state;

public:
    AtomicExpressionState(EventExpression event_expression)
        : state(AtomState::fromAtom(std::move(event_expression))) {};

    // All default constructors in place.

    std::shared_ptr<EventExpressionState> clone() const override {
        return make_shared<AtomicExpressionState>(*this); // Needs a copy constructor.
    }

    const EventExpression& getEventExpression() const override;
    std::vector<EventMask> getEventMasks() const override;
    vector<HandlerReport::EventReport> getStateReport() const override;
    bool evaluateExpressionState() const override;
    void update(const EventMask& triggeringMask, const Event& trigger_event, SimTime trigger_time) override;
    void reset() override;
};

}

#endif
