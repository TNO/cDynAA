// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_HANDLERREPORT_
#define INCLUDED_HANDLERREPORT_
/* File eventreport.h
 */


#include "handlertoken.h"
#include "eventexpression.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <set>
#include <utility>

namespace dynaa {

/**
 * HandlerReport
 */
class HandlerReport {
public:
    /**
     * Represents the state of one event in an expression
     */
    struct EventReport {
        EventExpression atom_ee;
        bool value;  // FIXME rename this to something clearer like 'triggered' ?
        EventExpression last_trigger;  // TODO why 'last', can it be re-triggered?
        // TODO could be more convenient if the event contained fields like
        // trigger_source, trigger_type, trigger_id
        SimTime trigger_time;

        bool operator==(const EventReport& other) const;
    };

    using state_map_type = std::unordered_map<EventExpression, EventReport, EventExpressionHash>;

private:
    /**
     * The HandlerToken associated with this report
     */
    HandlerToken m_handlerToken;
    /**
     * The event expression associated to this report
     */
    EventExpression m_event_expression;

    /**
     * The priority of the handler triggered by this report
     */
    int m_handler_priority;

    /**
     * Indicates if the associated handler was registered as once or not
     */
    bool m_handler_once;

    /**
     * Holds the state report for each event in the expression (and their triggering event, if existent)
     */
    vector<EventReport> m_event_states;

public:
    /**
     * Constructor
     */
    HandlerReport(HandlerToken handlerToken,
                  EventExpression event_expression,
                  int handler_priority,
                  bool handler_once,
                  vector<EventReport> states): m_handlerToken(std::move(handlerToken))
        , m_event_expression(std::move(event_expression))
        , m_handler_priority(handler_priority)
        , m_handler_once(handler_once)
        , m_event_states(std::move(states)) {};

    /**
     * Retrieves the event expression associated with this report
     *
     * @return
     *      the event expression associated with this report
     */
    EventExpression getEventExpression() const;

    /**
     * Retrieves the handler token associated with this report
     *
     * @return
     *      the handler token associated with the event handler registration addressed by this report
     */
    HandlerToken getHandlerToken() const;

    /**
     * Retrieves the priority of the handler associated with this report
     *
     * @return
     *      the priority associated with the registered handler of this report
     */
    int getHandlerPriority() const;


    // TODO Can we think of a clearer name for this method than 'once'?
    // it's not obvious to user that this refer to the callback's handler.
    /**
     * Is the handler associated to this report flagged as once?
     *
     * @return
     *      true if the handler associated to this report was flagged as once, false otherwise
     */
    bool once() const;

    /**
     * Retrieves the report for the states of each event in the expression
     *
     * Returns a vector of states with one state per distinct atom in the reported expression.
     * Each state reports if the atom was triggered (its value is true in this case, false otherwise).
     * If it was triggered, the state has a reference for the triggering event and the time it was triggered.
     *
     * @return
     *      a vector of states (EventReport structure) with one state report per distinct atom in the reported expression.
     */
    vector<EventReport> getEventStateReport() const;

    /**
     * Retrieves the report for the states of each atom in the expression
     *
     * Returns a vector of states with one state per distinct atom in the reported expression.
     * Each state reports if the atom was triggered (its value is true in this case, false otherwise).
     * If it was triggered, the state has a reference for the triggering event and the time it was triggered.
     *
     * @return
     *      a vector of states (EventReport structure) with one state report per distinct atom in the reported expression.
     */
    state_map_type getEventStateReportAsMap() const;
};

}

#endif
