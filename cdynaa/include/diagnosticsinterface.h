// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_DIAGNOSTICS_INTERFACE_
#define INCLUDED_DIAGNOSTICS_INTERFACE_

/*
 * Definition of the DiagnosticsInterface class
 */

#include <vector>
#include <iostream>
#include "eventexpression.h"
#include "diagnosticseventstorageinterface.h"
#include "diagnosticshandlerstorageinterface.h"


namespace dynaa {

std::vector<std::pair<SimTime, EventExpression>> typedef DiagnosticsEventTimeline;
std::vector<std::pair<SimTime, std::shared_ptr<HandlerReport>>> typedef DiagnosticsHandlerTimeline;

class DiagnosticsInterface {

public:
    /**
     * API towards python - For user
     */

    // BACKWARDS IN TIME

    /**
     * Gets past events
     * Different flavours:  getPastEvents() - returns all events
     *                      getPastEventsLastRun() - returns events for the last run
     *                      getPastEvents(int run) - returns events for specified run (see NOTE)
     * NOTE: If run is too high, returns last run. If it is negative returns all runs
     *
     * @param run
     *      OPTIONAL: Run for which to return the Timeline of past events (DiagnosticsEventTimeline)
     *      A negative value returns all runs. A value larger than the last run, returns the last run.
     *
     * @return DiagnosticsEventTimeline is a vector of pairs of time and event, it doesn't include handler tokens
     *      The run depends on the flavour of the function called
     */
    virtual DiagnosticsEventTimeline getPastEvents() const = 0;
    virtual DiagnosticsEventTimeline getPastEventsLastRun() const = 0;
    virtual DiagnosticsEventTimeline getPastEvents(const int run) const = 0;

    /**
     * Gets past handler tokens that fired in the requested time instant. This function ignores runs.
     *
     * @param time
     *      SimTime for which we are getting the tokens
     * @return
     *      The function returns the handler tokens of the handlers triggered at the requested time instant.
     */
    virtual std::vector<HandlerToken> getPastHandlers(const SimTime time) const = 0;

    /**
     * Gets past handler reports related to the given token in a std::vector<std::pair<SimTime, HandlerReport>> format.
     * This function ignores runs.
     *
     * @param token
     *      HandlerToken for which we are getting the reports
     * @return
     *      The function returns all the stored pairs of reports and times related to the given token.
     */
    virtual DiagnosticsHandlerTimeline getPastHandlerReports(const HandlerToken token) const = 0;

    /**
     * Gets past statistical data, this includes these values:
     * @return
     *      total_events_triggered: total number of events triggered in all the stored runs
     *      current_run_events_triggered: number of events triggered in the current run
     *      average_events_per_instant: average number of events per instant for all the stored runs
     *      variance_events_per_instant: variance of the events per instant for all the stored runs
     *      total_handlers_triggered: total number of handlers triggered in all the stored runs
     *      current_run_handlers_triggered: number of handlers triggered in the current run
     *      average_handlers_per_event: average number of handlers per event for the current run
     *      average_handlers_per_instant: average number of handlers per event for all the stored runs
     *      variance_handlers_per_instant: average number of handlers per event for all the stored runs
     *      user_expressions_triggered: number of user expressions evaluated to true in the current run
     *      user_expressions_registered: number of user expressions triggered in the current run
     *      atomic_expressions_triggered: number of atomic expressions evaluated to true in the current run
     *      atomic_expressions_registered: number of atomic expressions triggered in the current run
     *      average_complexity_registered_expressions: average number of atomic expressions per user expression registered in the current run
     *      variance_complexity_registered_expressions: variance in the number of atomic expressions per user expression registered in the current run
     *      average_complexity_triggered_expressions: average number of atomic expressions per user expression triggered in the current run
     *      variance_complexity_triggered_expressions: variance number of atomic expressions per user expression triggered in the current run
     *      number_of_callback_functions: Number of function callbacks executed
     *      number_of_callback_functions_current_run: Number of function callbacks executed in the current run
     */
    virtual std::map<std::string, double> getPastStatistics() const = 0;

    // FORWARDS IN TIME

    /**
     * Gets future events from the timeline. This allows to inspect the current
     * state of the schedule, but as the simulation continues, new events will probably
     * be scheduled.
     *
     * @return
     *      DiagnosticsEventTimeline with the events currently scheduled
     */
    virtual DiagnosticsEventTimeline getFutureEvents() const = 0;

    /**
     * Gets future statistical data, includes these values:
     * @return
     *      num_current_instants: number of instants in the future for which an event has been scheduled
     *      average_delta_time_between_instants: average time span between 2 consecutive instants in the future
     *      variance_delta_time_between_instants: variance in the time span between 2 consecutive instants in the future
     *      num_events_scheduled: number of events scheduled in the future
     *      average_num_events_scheduled: average number of events scheduled per instant in the future
     *      variance_num_events_scheduled: variance in the number of events scheduled per instant in the future
     *      num_registered_handlers: number of handlers currently registered
     */
    virtual std::map<std::string, double> getFutureStatistics() const = 0;

    /**
     * Gets time of last triggered event
     * @result SimTime of the last triggered event
     */
    virtual SimTime getTimeOfMostRecentEvent() const = 0;

    /**
     * Gets time of next scheduled event
     * @return SimTime of the next event in the schedule, if there are no more events,
     *      and invalid SimTime is returned (-1)
     */
    virtual SimTime getTimeOfNextEvent() const = 0;

    /**
     * Gets the next n events in the timeline
     * @param number_of_events number of events that we want to get
     *
     * @return
     *      Returns a DiagnosticsEventTimeline with n events if there are at least n events scheduled
     *      in the future. Otherwise if returns the events that are scheduled.
     */
    virtual DiagnosticsEventTimeline getNextEvents(const int number_of_events) const = 0;

    /**
     * Gets all the events scheduled for the nearest instant for which events are scheduled
     * @return
     *      Returns a DiagnosticsEventTimeline with all the events scheduled for the next time instant,
     *      if there isn't any next time instant, it returns an empty DiagnosticsEventTimeline
     */
    virtual DiagnosticsEventTimeline getEventsAtNextInstant() const = 0;

    // MANAGEMENT

    /**
     * Allows the activation or deactivation of the diagnostics.
     * When diagnostics are deactivated, the performance is better but the module doesn't work
     */
    virtual void activateDiagnosticsStorage() = 0;
    virtual void deactivateDiagnosticsStorage() = 0;
    virtual bool isDiagnosticsStorageActive() = 0;

    /**
     * Clears the memory, deletes the values in the counters vectors, resets
     * conters to 0, and runs the clear() function in the storages.
     */
    virtual void reset() = 0;
};

}
#endif //INCLUDED_DIAGNOSTICS_INTERFACE_


/**
 * Design discussions :
 *
 * 1) Singleton design:
 * For the same reason as other functions in this program, we use a singleton pattern.
 *
 * 2) Everything using runs assumes that the runs are numbered starting from 0 (not from 1)
 */