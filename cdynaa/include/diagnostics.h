// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_DIAGNOSTICS_
#define INCLUDED_DIAGNOSTICS_

/*
 * Parameters for configuration
 */
#define MAX_COMPLEXITY 256

/*
 * Definition of the Diagnostics class
 */

#include <string>
#include <vector>
#include <array>
#include <iostream>
#include "eventexpression.h"
#include "diagnosticseventstorageinterface.h"
#include "diagnosticseventstorage.h"
#include "diagnosticshandlerstorageinterface.h"
#include "diagnosticshandlerstorage.h"
#include "diagnosticsinterface.h"


namespace dynaa {

struct DiagnosticsRunCounters {
    int events; //number of events
    int handlers; //number of handlers
    int callbacks; //number of callbacks
    int triggered_user_expressions; //number of triggered user expressions
    int registered_user_expressions; //number of registered user expressions
    int triggered_atom_expressions; //number of triggered atom expressions
    int registered_atom_expressions; //number of registered atom expressions
};

class Diagnostics: public DiagnosticsInterface {

private:

    /**
     * Storage of diagnostics event records
     */
    DiagnosticsEventStorageInterface* diagRecEvStorage;

    /**
     * Storage of diagnostics handler records
     */
    DiagnosticsHandlerStorageInterface* diagRecHanStorage;

    /**
     * Value which stores whether diagnostics is on or off
     */
    bool m_diagnosticsStorageActive;

    /**
     * Counter keeping track of events in the current run
     */
    int m_events_current_run;

    /**
     * Counter keeping track of handlers in the current run
     */
    int m_handlers_current_run;

    /**
     * Counter keeping track of function callbacks in the current run
     */
    int m_callbacks_current_run;

    /**
     * Counter keeping track of events in the current run
     */
    int m_events_total;

    /**
     * Counter keeping track of handlers in the current run
     */
    int m_handlers_total;

    /**
     * Counter keeping track of function callbacks in the current run
     */
    int m_callbacks_total;

    /**
     * Counter keeping track of atomic expressions triggered in the current run
     */
    int m_trig_atom_expr;

    /**
     * Counter keeping track of atomic expressions registered in the current run
     */
    int m_reg_atom_expr;

    /**
     * Array storing the number of user expressions registered with a complexity
     * equal to the array index. Complexity 0 means off-bounds (larger than MAX_COMPLEXITY_REGISTERED)
     */
    std::array<long, MAX_COMPLEXITY> m_number_of_registered_expressions_with_i_complexity = {0};

    /**
     * Array storing the number of user expressions triggered with a complexity
     * equal to the array index. Complexity 0 means off-bounds (larger than MAX_COMPLEXITY_REGISTERED)
     */
    std::array<long, MAX_COMPLEXITY> m_number_of_triggered_expressions_with_i_complexity = {0};

    /**
     * Storage of the counter values at the end of previous runs
     */
    std::vector<DiagnosticsRunCounters> m_counters_history;

    /**
     * Static constant invalid token, to fill in records of events without handler
     */
    static const HandlerToken invalid_token;

    /**
     * Construct a new diagnostics engine. This function is private because it is
     * a singleton class. If any other class wants an instantiation of the
     * class, the function getInstance() should be used.
     *
     * @see #getInstance()
     */
    Diagnostics(): diagRecEvStorage(new DiagnosticsEventStorage()),
        diagRecHanStorage(new DiagnosticsHandlerStorage()),
        m_diagnosticsStorageActive(false) {}

    /**
     * Deletes and blocks the copy constructor and the assignment operator!
     */
    Diagnostics(const Diagnostics &other) = delete;
    Diagnostics& operator=(const Diagnostics &other) = delete;

    /**
     * Calculates variance of a vector
     */
    double calculateVariance(const std::vector<double> vector) const;

    /**
     * Stores the values of the current run counters in m_counters_history, not
     * including the counters for the total of all the runs
     */
    void storeRunCounterValues();

    /**
     * Resets to 0 the values of the current run counters in m_counters_history,
     * not including the counters for the total of all the runs
     */
    void resetRunCounterValues();

    /**
     * Transforms DiagnosticsFullRecordsList to DiagnosticTimeline
     */
    DiagnosticsEventTimeline formatRecords(const DiagnosticsEventRecordsList records) const;

    /**
     * Gets records and calculates average/variance of events per instant
     * and average/variance of handlers per event, returns everthing as map
     * The entries are:
     *      average_events_per_instant
     *      variance_events_per_instant
     */
    std::map<std::string, double> generateStatisticsFromRecords() const;

    /**
     * Computes variance in the complexity of expressions based on
     * the vector keeping the instances of each complexity
     */
    double complexityVariance(const std::array<long, MAX_COMPLEXITY> complexities) const;

public:
    /**
     * API towards cDynAA - Not for user
     */

    /**
     * The Diagnostics Engine is a singleton class, so internally an instance is
     * kept to the only instance that will be created. Using this function this
     * instance can be obtained.
     *
     * @return an instance of the singleton diagnostics engine
     */
    static Diagnostics& getInstance();

    /**
     * addDiagnosticsRecord adds an event record (time, event) to the event storage
     *
     * @param event
     *      Event that will be stored
     */
    void addDiagnosticsRecord(const Event& event);

    /**
     * addDiagnosticsRecord adds a handler record (time, handler token, handler report)
     * to the handler storage
     *
     * @param token
     *      HandlerToken related to the provided HandlerReport
     * @param record
     *      HandlerReport that will be stored
     */
    void addDiagnosticsRecord(const HandlerToken token, const HandlerReport& report);

    /**
     * Increment events counter for the current run
     */
    void incrementEventsCounter();

    /**
     * Increment handlers counter for the current run
     */
    void incrementHandlersCounter();

    /**
     * Increment triggered user expressions counter for the current run
     */
    void storeTriggeredUserExprComplexity(const int complexity);

    /**
     * Increment triggered atomic expressions counter for the current run
     */
    void incrementTriggeredAtomExprCounter();

    /**
     * Increment registered atomic expressions counter for the current run
     * by the amount specified in amount
     */
    void incrementRegisteredAtomExprCounter(const int amount);

    /**
     * Increment callbacks counter for the current run
     */
    void incrementCallbacksCounter();

    /**
     * Increases the size of the counter vectors, increments by one the
     * value of m_current_run, and runs the newRun() function in the storage
     */
    void startNewRun();

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
    DiagnosticsEventTimeline getPastEvents() const;
    DiagnosticsEventTimeline getPastEventsLastRun() const;
    DiagnosticsEventTimeline getPastEvents(const int run) const;

    /**
     * Gets past handler tokens that fired in the requested time instant. This function ignores runs.
     *
     * @param time
     *      SimTime for which we are getting the tokens
     * @return
     *      The function returns the handler tokens of the handlers triggered at the requested time instant.
     */
    std::vector<HandlerToken> getPastHandlers(const SimTime time) const;

    /**
     * Gets past handler reports related to the given token in a std::vector<std::pair<SimTime, HandlerReport>> format.
     * This function ignores runs.
     *
     * @param token
     *      HandlerToken for which we are getting the reports
     * @return
     *      The function returns all the stored pairs of reports and times related to the given token.
     */
    DiagnosticsHandlerTimeline getPastHandlerReports(const HandlerToken token) const;

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
     *      average_handlers_per_instant: average number of handlers per instant for all the stored runs
     *      variance_handlers_per_instant: average number of handlers per instant for all the stored runs
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
    std::map<std::string, double> getPastStatistics() const;

    // FORWARDS IN TIME

    /**
     * Gets future events from the timeline. This allows to inspect the current
     * state of the schedule, but as the simulation continues, new events will probably
     * be scheduled.
     *
     * @return
     *      DiagnosticsEventTimeline with the events currently scheduled
     */
    DiagnosticsEventTimeline getFutureEvents() const;

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
    std::map<std::string, double> getFutureStatistics() const;

    /**
     * Gets time of last triggered event
     * @result SimTime of the last triggered event
     */
    SimTime getTimeOfMostRecentEvent() const;

    /**
     * Gets time of next scheduled event
     * @return SimTime of the next event in the schedule, if there are no more events,
     *      and invalid SimTime is returned (-1)
     */
    SimTime getTimeOfNextEvent() const;

    /**
     * Gets the next n events in the timeline
     * @param number_of_events number of events that we want to get
     *
     * @return
     *      Returns a DiagnosticsEventTimeline with n events if there are at least n events scheduled
     *      in the future. Otherwise if returns the events that are scheduled.
     */
    DiagnosticsEventTimeline getNextEvents(const int number_of_events) const;

    /**
     * Gets all the events scheduled for the nearest instant for which events are scheduled
     * @return
     *      Returns a DiagnosticsEventTimeline with all the events scheduled for the next time instant,
     *      if there isn't any next time instant, it returns an empty DiagnosticsEventTimeline
     */
    DiagnosticsEventTimeline getEventsAtNextInstant() const;

    // MANAGEMENT

    /**
     * Allows the activation or deactivation of the diagnostics.
     * When diagnostics are deactivated, the performance is better but the module doesn't work
     */
    void activateDiagnosticsStorage();
    void deactivateDiagnosticsStorage();
    bool isDiagnosticsStorageActive();

    /**
     * Clears the memory, deletes the values in the counters vectors, resets
     * conters to 0, and runs the clear() function in the storages.
     */
    void reset();
};

}
#endif //INCLUDED_DIAGNOSTICS_


/**
 * Design discussions :
 *
 * 1) Singleton design:
 * For the same reason as other functions in this program, we use a singleton pattern.
 *
 * 2) Everything using runs assumes that the runs are numbered starting from 0 (not from 1)
 */