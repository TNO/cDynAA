// NOTE: a guard with license and author info is auto added to package
// distributions (see setup.py)
#ifndef INCLUDED_HANDLERRECORD_
#define INCLUDED_HANDLERRECORD_

/* Definition of the HandlerRecord class */

#include <vector>
#include <set>

#include "eventhandler.h"
#include "eventexpression.h"
#include "eventmask.h"
#include "handlertoken.h"
#include "expressionstate.h"

namespace dynaa {

/**
 * HandlerRecords register associations between an event handler and an event expression.
 *
 * The engine updates records in the registry after each event, if the event matches terms
 * of the event expression of the record.  In other words, the engine uses records to
 * track the evolution in time of the event expression state.
 *
 * The state of an event expression (within a record) can be used to evaluate if the
 * event expression is true.  If so, the event handler (and its internal callback) is
 * safely invoked.  Safely means other conditions, such as memory safety are checked.
 *
 * Additionally, the record keeps track of meta information associated with the
 * (EventHandler, EventExpression) pair.  Examples of such meta information are once flag,
 * validity, priority, etc.
 *
 */
class HandlerRecord {

private:

    /**
      * Seed for identifier sequences
      */
    static long m_idseed;

    /**
      * Entity unique identifier
      */
    HandlerToken m_record_uid;

    /* Holds the state for this record */
    std::shared_ptr<EventExpressionState> m_expression_state;

    /* Holds the handler associated to this record */
    /* DESIGN_DISCUSSION: If eventHandlers are merely representations without state, there is no need for using pointers in cDynAA
                          side. I will start here with pointers because of the interface with pydynaa.
                          The real pointer to something external is internal in the event handler.*/
    std::shared_ptr<EventHandler> m_event_handler;

    /**
     * Validity flag for this record
     */
    bool m_valid;

    /**
     * Controls the lifetime span of this record
     *
     * If m_once flag is true, means the record will be invalidated immediatelly before the handler is activated.
     * If m_once flag is false, the record will be reset immediatelly before the handler is activated.
     */
    bool m_once;

public:

    /**
     * Constructor
     *
     * Builds a new HandlerRecord with the given event expression and event handler.
     *
     * @param m_event_expression
     *            the event expression associated with this record
     * @param m_event_handler
     *            the event handler associated with this record
     */
    HandlerRecord(const EventExpression& eventExpression, const shared_ptr<EventHandler>& eventHandler)
        : m_record_uid(createHandlerToken(++HandlerRecord::m_idseed,
                                          eventHandler ? eventHandler->getPriorityLevel() : throw runtime_error("Cannot register invalid/inexisting handler")))
        , m_expression_state(createExpressionState(eventExpression))
        , m_event_handler(eventHandler)
        , m_valid(true)
        , m_once(true) {};

    HandlerRecord(const EventSource& entity, const EventType &evType, const EventID &id,
                  const shared_ptr<EventHandler>& eventHandler) :
        HandlerRecord(EventExpression(entity, evType, id), eventHandler) {};

    /**
     * Constructors
     */
    HandlerRecord(const HandlerRecord& other)
        : m_record_uid(other.m_record_uid)
        , m_expression_state(other.m_expression_state->clone())
        , m_event_handler(make_shared<EventHandler>(*(other.m_event_handler)))
        , m_valid(other.m_valid)
        , m_once(other.m_once) {};

    /**
     * Retrieves a unique ID for this Entity.
     *
     * @return an unique ID for this entity.
     */
    HandlerToken getUID() const;

    /**
     * Retrieves the event expression of this record
     *
     * @return EventExpression a copy of the event expression associated with this record
     */
    EventExpression getEventExpression() const;

    /**
     * Retrieves the event handler of this record
     *
     * @return shared_ptr<EventHandler> a shared pointer to the event handler associated with this record
     */
    std::shared_ptr<EventHandler> getEventHandler() const;

    /**
     * Retrieves the set of masks associated with the event expressions (atoms) in this record.
     *
     * For efficiency reasons, this method returns a view (immutable) for the internal storage set.
     *
     * @return set<EventMask> a set of masks for the atoms in the record's event expressions.
     */
    std::vector<EventMask> getEventMasks() const;

    /**
     * Creates an event report based on the actual state of this record.
     *
     * @return
     *      event report object reporting on the actual state of this record.
     */
    HandlerReport createEventReport() const;

    /**
     * Check if the event expression in this record triggered
     *
     * @return bool true if the event expression in this record evaluates to true
     */
    bool triggered() const;

    /**
     * Resets this record and all its states.
     *
     * Reset occurs only if the record is valid.
     *
     * Note that reset cleans the state and as such retriggers the expression to fire again.
     *
     * Typical use of reset is to re-start the record after the event is handled and the
     * record is not signed as once.
     */
    void reset();

    /**
     * Sets the value of the once flag
     *
     * If the record is flag as once, it will be invalidated after triggering the handler
     *
     * @param once_flag
     *      true if this record is flagged as "once" (this is the default flag), false otherwise
     */
    void set_once(bool once_flag);

    /**
     * Invalidates this record
     *
     * Invalid records cannot be validated again. They are marked for later deletion.
     */
    void invalidate();

    /**
     * Checks if this record is valid
     *
     * @return
     *      true if this record is valid, false otherwise
     */
    bool is_valid() const;

    /**
     * Checks if this record is flagged as once
     *
     * If the record is flag as once, it will be invalidated after triggering the handler
     *
     * @return
     *      true if this record is flagged as "once" (this is the default flag), false otherwise
     */
    bool is_once() const;

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
     * The first variant of the function builds internally the correct mask from the event. It
     * invokes the second variant afterwards.
     *
     * The second variant can be used if many records are updated with the same event.
     * The event can be then built once.  Small win in performance.
     *
     */
    void update(const Event& trigger_event, SimTime trigger_time);

};

} // namespace dynaa

#endif