// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_EVENT_EXPRESSION_
#define INCLUDED_EVENT_EXPRESSION_
/* Definition of the EventExpression class
 */

#include "event.h"
#include "dynaatypes.h"
#include <string>
#include <memory>
#include <vector>

using namespace std;
namespace dynaa {

/**
 * Enumeration of possible types of event expressions
 */
enum EventExpressionType {
    Atomic,
    AND,
    OR
};

typedef SourceUID EventSourceID;

/**
 * Core class for an event expression
 *
 * An event expression is a boolean (logical) expression on one or more event triggering conditions.
 * It represents a complex logical combination of events that are of interest.
 *
 * An event triggering condition is a triplet (source, type, id) indicating a condition
 * to be satisfied by an event.  Given a certain set of events, it is possible to evaluate the event expression,
 * that is, the logical relation of its event triggering conditions to a boolean veredict (true or false).
 *
 * EventExpressions objects are "only" a representation of the logical expressions.  It does not track issued events,
 * nor evaluates it against a set of events, nor invokes a event handler.
 *
 * The idea behind event expressions is to be able to represent combinations of event conditions.
 * Examples:
 *
 *  -  We want to represent event combinations when both "any event coming from source_A"  AND  "any event coming from source_B" occur.
 *  -  we want to represent event combinations when "any event of type T1" OR "any event of type T2 coming from source_C" occur.
 */
class EventExpression {

    // Friendship needed to set the private trigger event; CHECK set differently?
    friend class DefaultHandlerRegistry;

private:

    std::shared_ptr<string> m_descriptor;
    EventSource m_entity;
    EventSourceID m_entity_id;
    EventType m_evType;
    EventID m_id;

    EventExpressionType m_type;

    // Sub-expressions if applicable
    std::shared_ptr<EventExpression> m_firstTerm;
    std::shared_ptr<EventExpression> m_secondTerm;

    /**
     * Helper function to collect this expressions atoms
     */
    void atoms(vector<EventExpression> &collector) const;

public:

    /**
     * Commonly used expressions -- easy to use
     */
    static const EventExpression any_event;

    /**
    * Constructor for EventExpression.
    *
    * Using this consctructor creates always a basic (or atomic) event expression.
    *
    * This atomic EventExpression may be combined with others by means of boolean operators
    * && and || to create more complex EventExpressions.
    *
    * @param name
    *      An identifier for this atomic event expression.
    * @param entity
    *      An EventSource from where the triggering event should come from, use Event::ANY_SOURCE for a wildcard on the source.
    * @param evType
    *      An EventType for the event that should trigger this event expression, use Event::ANY_TYPE for a wildcard on the type.
    * @param id
    *      An identifier for the specific event that should trigger this event expression, use Event::ANY_ID for a wildcard on the type.
    */
    EventExpression(string name, const EventSource& entity, EventType evType, EventID id);
    EventExpression(const EventSource& entity, EventType evType, EventID id);

    /**
    * Default Constructor .
    */
    EventExpression() : EventExpression(Event::ANY_SOURCE, Event::ANY_TYPE, Event::ANY_ID) {}; //Constructor

    /**
    * Copy constructor.
    *
    * Ensures deep copy of child expressions. Does not copy triggered events.
    */
    EventExpression(const EventExpression &evExpression);

    /**
     * Event based constructor.
     *
     * Constructs an atomic EventExpression that represents exactly the event given.
     *
     * Assuming an event 'ev', this constructor is similar to a call for
     * EventExpression(ev.source(),ev.type(),ev.id())
     *
     *
     * @param event
     *      The event to be expressed by the create EventExpression
     */
    // DESIGN_DECISION: This allows us to use an event in every place where an event expression is used (static).
    //                    Conversion is done by the compiler.  If we do not want this, we should declare as 'explicit'
    EventExpression(const Event &event);

    /**
     * Default destructor
     *
     * Higher level expressions, such as combined AND and OR expressions
     * own a copy of their internal expressions.  They are responsible for
     * releasing it.
     *
     * Note that higher level expressions hold a shared pointer to its internal
     * expressions, which in turn can be shared with other parties.  Destroying
     * a higher level expression releases this pointer.  If the expression is
     * the only one holding the pointer, the sub-expressions are also destroyed.
     * If not, it is preserved to whoever else is holding it.
     *
     */
    ~EventExpression();

    /**
     * Retrieve event type of an atomic expression.
     *
     * If this is not an atomic expression, then ANY_TYPE is returned.
     *
     * @return EventType.
     */
    const EventType &atomicEventType() const;

    /**
     * Retrieve event ID of an atomic expression.
     *
     * If this is not an atomic expression, then ANY_ID is returned.
     *
     * @return EventID.
     */
    EventID atomicEventID() const;

    /**
     * Retrieve event source (entity) of an atomic expression.
     *
     * If this is not an atomic expression, then ANY_SOURCE is returned.
     *
     * @return EventSource.
     */
    EventSource atomicEventSource() const;

    /**
     * Retrieve the id of this event source (entity) if an atomic expression.
     *
     * If this is not an atomic expression, then the id for ANY_SOURCE is returned.
     *
     * This is a helper function.  It is meant to be a helper and used for efficiency.
     * It is NOT a guarantee that the source still exists -- check atomicEventSource().lock() for that.
     *
     * The following contract for the value of the id:
     *   - the id is the source id at the time of the expression creation;
     *   - if there is no valid source at the time of creation, the source id is -1L;
     *   - if this expression is not atomic, returns the id of EVENT::ANY_SOURCE
     *
     * @return the ID of the event source (entity) in this expression. See conditions above.
     */
    EventSourceID atomicEventSourceID() const;

    /**
     * Retrieves the type of this EventExpression
     *
     * Type of an EventExpression can be Atomic, AND or OR.
     * Type of an EventExpression is defined by the type EventExpressionType.
     *
     * @return EventExpressionType indicating the type of this EventExpression: Atomic, AND, or OR
     *
     * @see EventExpressionType
     */
    EventExpressionType type() const;

    /**
     * Retrieves the descriptor name of this EventExpression
     *
     * The descriptor is a helper for the user on managing event expressions.
     *
     * It is not relevant for comparison, nor any other operation.
     *
     * @return string a descriptor identifying the event expression.
     */
    const string descriptor() const;

    /**
     * Returns the first term of this expression.
     *
     * AND and OR expression are binary expressions.  In this case the
     * first term is returned. E.g. A && B, returns A.
     *
     * Atomic expressions is a unary expression.  In this case we consider
     * that the first, and the second term are the expression itself.  Thus,
     * retrieving the first term of an atomic expression, returns itself.
     *
     * @return the first term of this expression.
     */
    const EventExpression &firstTerm() const;

    /**
     * Returns the second term of this expression.
     *
     * AND and OR expression are binary expressions.  In this case the
     * second term is returned. E.g. A && B, returns B.
     *
     * Atomic expressions is a unary expression.  In this case we consider
     * that the first, and the second term are the expression itself.  Thus,
     * retrieving the second term of an atomic expression, returns itself.
     *
     * @return the second term of this expression.
     */
    const EventExpression &secondTerm() const;

    /**
     * Returns the number of atomic expressions of this expression.
     *
     * @return integer with the total number of atomic expressions of this expression.
     * This function does not take into account whether the expressions are distinct,
     * for example for the expression (( A && B ) || ( A && C )), the result would be 4.
     */
    int numberOfAtomicExpressions() const;

    /**
     * Returns a vector with all the atomic terms in this expression.
     *
     * If this expression is itself an atomic expression, returns
     * a vector with itself included.
     *
     * @return vector<EventExpression > list of atoms in this event expression
     */
    vector<EventExpression > atoms() const;

    /**
    * Returns true if the EventExpression is atomic and fully qualified.
    */
    bool isFullyQualified() const;

    /**
     * Utility print std::function
     */
    friend std::ostream &operator<<(std::ostream &strm, const EventExpression &expression);

    /**
     * Defines && operator for event expressions.
     *
     * Produces always an AND EventExpression with this as the first term and second as the second term.
     */
    EventExpression operator&&(EventExpression second) const;

    /**
     * Defines || operator for event expressions.
     *
    * Produces always an OR EventExpression with this as the first term and second as the second term.
    */
    EventExpression operator||(EventExpression second) const;

    /**
    * Defines an equality operator for event expressions.
    *
    * The name given to an event expression does not matter in equality comparisons.
    * Comparison is made based on the qualifiers of the expression.
    *
    * Default operator does not work, because there is no deep comparison.
    */
    bool operator==(const EventExpression &second) const;

    /**
    * Defines an inequality operator for event expressions.
    */
    bool operator!=(const EventExpression &second) const;

    /**
     * Default constructors
     **/
    EventExpression& operator=(EventExpression&&) = default;
    EventExpression& operator=(const EventExpression&) = default;
    EventExpression(EventExpression &&) = default;

};

struct EventExpressionHash {
    /**
     * Hash operator for this event mask. Hashing is important for using event masks as keys in the
     * registry
     *
     * EventExpressionHash is a hash struct for event expressions.
     *
     * NOTE: Avoid using non-atomic event expression as map keys or in sets.
     *      Calculating the hash on an atomic event expression is cheap and efficient,
     *      but calculating the hash on a non-atomic event expression can be very inneficient and expensive
     *      due to the possible composition of the expression.
     **/
    inline std::size_t operator()(const EventExpression& k) const {
        if (k.type() == Atomic) {
            return (k.atomicEventSourceID()) ^ ((k.atomicEventType().id() << 13) >> 7) ^ (k.atomicEventID() << 3);
        }
        return (*this)(k.firstTerm()) + (*this)(k.secondTerm());
    }
};

/**
 * Overloads the << operator
 */
std::ostream &operator<<(std::ostream &strm, const EventExpression &expression);

/**
 * Overload operators to create expressions with events
 */
EventExpression operator&&(const Event& event, const EventExpression& expression);
EventExpression operator||(const Event& event, const EventExpression& expression);

} // namespace dynaa

/**
 * DISCUSSIONS AND DECISIONS
 *
 * Redesign of EventExpresion --------------
 *
 * 1) EventExpression is no longer an Entity and can no longer 'wait'.
 *    To wait on an event expression the waitOnce() method of an existing entity should
 *    be used.
 *
 * 2) EventExpression no longer holds a callback.
 *    An EventExpressionHandler holds the callback to be called when an event expression
 *    is triggered.
 *
 * 3) The new EventExpressions are **only** a representation of the logical expression.
 * They do not retain any state, nor register of the triggering of the events anymore. This is the
 * main difference for objects of the type EventExpression.  In a way, (new) EventExpressions are a lot like the event descriptors
 * when waiting for event patterns.
 * They represent a more complex event descriptor, formed by a logic combination of atomic events.
 *
 * 4) The main difference for the old architecture is not in its API, but in the fact that the new EventExpression objects
 * do not track any activation of events, nor trigger any handler.  They have less responsabilities (single responsability principle).
 * The subtracted functionality will be implemented in other way.
 *
 **/

#endif
