#include <string>
#include <iostream>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <numeric>
#include "../include/entity.h"
#include "../include/dynaatypes.h"
#include "../include/diagnostics.h"


using namespace dynaa;

const HandlerToken Diagnostics::invalid_token = HandlerToken();

// Define structs here

Diagnostics& Diagnostics::getInstance() {
    static Diagnostics singletonDiagnostics;
    return singletonDiagnostics;
}

void Diagnostics::addDiagnosticsRecord(const HandlerToken token, const HandlerReport& report) {
    if (isDiagnosticsStorageActive()) {
        diagRecHanStorage->addDiagnosticsRecord(SimulationEngine::getInstance().getCurrentTime(), report, token);
    }
}

void Diagnostics::addDiagnosticsRecord(const Event& event) {
    if (isDiagnosticsStorageActive()) {
        diagRecEvStorage->addDiagnosticsRecord(SimulationEngine::getInstance().getCurrentTime(), event);
    }
}

void Diagnostics::startNewRun() {
    diagRecHanStorage->setLabel(diagRecHanStorage->getLabel() + 1);
    diagRecEvStorage->setLabel(diagRecEvStorage->getLabel() + 1);
    storeRunCounterValues();
    resetRunCounterValues();
}

DiagnosticsEventTimeline Diagnostics::getPastEvents() const {
    return formatRecords(diagRecEvStorage->getRecords());
}

DiagnosticsEventTimeline Diagnostics::getPastEventsLastRun() const {
    return getPastEvents(diagRecEvStorage->getLabel());
}

DiagnosticsEventTimeline Diagnostics::getPastEvents(const int run)
const { // Int because it might be more pythonable than unsigned?
    if (run > diagRecEvStorage->getLabel()) {
        return getPastEventsLastRun();
    } else if (run >= 0) {
        DiagnosticsEventRecordsList records = diagRecEvStorage->getRecords();
        // Get subset of records for the specified run
        DiagnosticsEventRecordsList records_for_run;
        for (auto it = records.begin(); it < records.end(); it++) {
            if (it->label == (unsigned) run) {
                records_for_run.push_back(*it);
            }
        }
        // Format subset of records
        return formatRecords(records_for_run);
    } else {
        // Return all records
        return getPastEvents();
    }
}

std::vector<HandlerToken> Diagnostics::getPastHandlers(const SimTime time) const {
    if (time > SimulationEngine::getInstance().getCurrentTime()) return std::vector<HandlerToken>();
    DiagnosticsHandlerReportsList records = diagRecHanStorage->getRecords();
    std::vector<HandlerToken> query;
    for (auto it = records.begin(); it < records.end(); it++) {
        if (time == it->timestamp) {
            query.push_back(it->hToken);
        }
    }
    return query;
}

DiagnosticsHandlerTimeline Diagnostics::getPastHandlerReports(const HandlerToken token) const {
    DiagnosticsHandlerReportsList records = diagRecHanStorage->getRecords();
    std::vector<std::pair<SimTime, shared_ptr<HandlerReport>>> query;
    int i = 0;
    for (auto it = records.begin(); it < records.end(); it++) {
        if (token == it->hToken) {
            query.push_back(make_pair(it->timestamp, std::make_shared<HandlerReport>(it->hReport)));
        }
        i++;
    }
    return query;
}

std::map<std::string, double> Diagnostics::getPastStatistics() const { // Extend as needed
    std::map<std::string, double> stats_from_records = generateStatisticsFromRecords();
    double user_expressions_triggered = std::accumulate(m_number_of_triggered_expressions_with_i_complexity.begin(),
                                        m_number_of_triggered_expressions_with_i_complexity.end(), 0);
    double user_expressions_registered = std::accumulate(m_number_of_registered_expressions_with_i_complexity.begin(),
                                         m_number_of_registered_expressions_with_i_complexity.end(), 0);
    std::map<std::string, double> statistics = {
        // event stats: number processed, mean/stddev per time instant
        {"total_events_triggered", m_events_total},
        {"current_run_events_triggered", m_events_current_run},
        {"average_events_per_instant", stats_from_records.at("average_events_per_instant")},
        {"variance_events_per_instant", stats_from_records.at("variance_events_per_instant")},
        // handler stats: numbered triggered, number registered, mean/stddev triggered per time instant. (NOTE this is handlers registered by user, not internally)
        {"total_handlers_triggered", m_handlers_total},
        {"current_run_handlers_triggered", m_handlers_current_run},
        {"average_handlers_per_event", (double) m_handlers_current_run / m_events_current_run},
        {"average_handlers_per_instant", stats_from_records.at("average_handlers_per_instant")},
        {"variance_handlers_per_instant", stats_from_records.at("variance_handlers_per_instant")},
        // event expression stats: number of user expressions triggered and numbered registered (should match with handler stats), number of atom expressions triggered and registered, mean/stddev of triggered and registered expression complexity (:= number of atoms per expression)
        {"user_expressions_triggered", user_expressions_triggered},
        {"user_expressions_registered", user_expressions_registered},
        {"atomic_expressions_triggered", m_trig_atom_expr},
        {"atomic_expressions_registered", m_reg_atom_expr},
        {"average_complexity_registered_expressions", (double) m_reg_atom_expr / user_expressions_registered},
        {"variance_complexity_registered_expressions", complexityVariance(m_number_of_registered_expressions_with_i_complexity)},
        {"average_complexity_triggered_expressions", (double) m_trig_atom_expr / user_expressions_triggered},
        {"variance_complexity_triggered_expressions", complexityVariance(m_number_of_triggered_expressions_with_i_complexity)},
        // implementation insight stats: total number of callback function calls, including as part of internal implementation
        {"number_of_callback_functions", m_callbacks_total},
        {"number_of_callback_functions_current_run", m_callbacks_current_run}
    };
    return statistics;
}

DiagnosticsEventTimeline Diagnostics::getFutureEvents() const {
    Timeline timeline = SimulationEngine::getInstance().getEventInspector().getTimeline();
    DiagnosticsEventTimeline formated_timeline;
    // convert to DiagnosticsEventTimeline format with empty vectors for handlers
    for (auto itx = timeline.begin(); itx != timeline.end(); itx++) {
        auto eventSet = itx->second;
        for (auto ity = eventSet.begin(); ity != eventSet.end(); ity++) {
            formated_timeline.push_back(std::pair<SimTime, EventExpression> {itx->first,
                                        (EventExpression(*ity))
                                                                            });
        }
    }
    return formated_timeline;
}

std::map<std::string, double> Diagnostics::getFutureStatistics() const {
    Timeline timeline = SimulationEngine::getInstance().getEventInspector().getTimeline();
    std::vector<double> deltas;
    std::vector<double> events;
    double previous_instant;

    for (auto it = timeline.begin(); it != timeline.end(); it++) {
        if (it != timeline.begin()) deltas.push_back(it->first - previous_instant);
        previous_instant = it->first;
        events.push_back(it->second.size());
    }
    double average_delta_time_between_instants = (deltas.size() == 0)? 0 :
            (double) std::accumulate(deltas.begin(), deltas.end(), 0.0) / deltas.size();
    double average_num_events_scheduled = (events.size() == 0)? 0 :
                                          (double) std::accumulate(events.begin(), events.end(), 0.0) / events.size();
    std::map<std::string, double> statistics = {
        {"num_current_instants", timeline.size()},
        {"average_delta_time_between_instants", average_delta_time_between_instants},
        {"variance_delta_time_between_instants", calculateVariance(deltas)},
        {"num_events_scheduled", std::accumulate(events.begin(), events.end(), 0)},
        {"average_num_events_scheduled", average_num_events_scheduled},
        {"variance_num_events_scheduled", calculateVariance(events)},
        {"num_registered_handlers", SimulationEngine::getInstance().getHandlerInspector().getNumberOfHandlers()}
    };
    return statistics;
}

SimTime Diagnostics::getTimeOfMostRecentEvent() const {
    return SimulationEngine::getInstance().getCurrentTime();
}

SimTime Diagnostics::getTimeOfNextEvent() const {
    auto timeline = SimulationEngine::getInstance().getEventInspector().getTimeline();
    if (timeline.size()) {
        return SimulationEngine::getInstance().getEventInspector().getTimeline().begin()->first;
    } else {
        return -1;
    }
}

DiagnosticsEventTimeline Diagnostics::getNextEvents(const int number_of_events) const {
    if (number_of_events < 1) return DiagnosticsEventTimeline();
    DiagnosticsEventTimeline full_timeline = getFutureEvents();
    if ((unsigned)number_of_events > full_timeline.size()) {
        DiagnosticsEventTimeline reduced_timeline(full_timeline.begin(), full_timeline.end());
        return reduced_timeline;
    } else {
        DiagnosticsEventTimeline reduced_timeline(full_timeline.begin(), full_timeline.begin() + number_of_events);
        return reduced_timeline;
    }
}

DiagnosticsEventTimeline Diagnostics::getEventsAtNextInstant() const {
    auto timeline = SimulationEngine::getInstance().getEventInspector().getTimeline();
    int n = 0;
    if (timeline.size()) {
        n = timeline.begin()->second.size();
    }
    return getNextEvents(n);
}

double Diagnostics::calculateVariance(const std::vector<double> vector) const {
    //TODO: Quite expensive routine.  I think there are cmath or std libs optimized for this.
    double variance = 0;
    if (vector.size() > 1) {
        double average = (double) std::accumulate(vector.begin(), vector.end(), 0.0) / vector.size();
        for (auto it = vector.begin(); it < vector.end(); it++) {
            variance += (*it - average) * (*it - average);
        }
        variance = variance / (vector.size() - 1);
    }
    return variance;
}

void Diagnostics::activateDiagnosticsStorage() {
    m_diagnosticsStorageActive = true;
}

void Diagnostics::deactivateDiagnosticsStorage() {
    m_diagnosticsStorageActive = false;
}

bool Diagnostics::isDiagnosticsStorageActive() {
    return m_diagnosticsStorageActive;
}

void Diagnostics::reset() {
    diagRecEvStorage->clear();
    diagRecHanStorage->clear();
    resetRunCounterValues();
    m_events_total = 0;
    m_handlers_total = 0;
    m_callbacks_total = 0;
    m_number_of_registered_expressions_with_i_complexity.fill(0);
    m_number_of_triggered_expressions_with_i_complexity.fill(0);
}

DiagnosticsEventTimeline Diagnostics::formatRecords(const DiagnosticsEventRecordsList records) const {
    DiagnosticsEventTimeline formated_timeline;
    for (auto it = records.begin(); it < records.end(); it++) {
        formated_timeline.push_back({it->timestamp, EventExpression(it->event)});
    }

    return formated_timeline;
}

std::map<std::string, double> Diagnostics::generateStatisticsFromRecords() const {
    std::vector<double> sums;
    DiagnosticsEventTimeline timeline = getPastEvents();
    double average_events = 0;
    double variance_events = 0;
    double average_handlers = 0;
    double variance_handlers = 0;
    if (timeline.size() != 0) {
        int sum = 0;
        SimTime current_time = timeline.begin()->first;
        for (auto it = timeline.begin(); it < timeline.end(); it++) {
            if (it->first != current_time) {
                sums.push_back(sum);
                sum = 0;
                current_time = it->first;
            }
            sum++;
        }
        if (sum > 0) sums.push_back(sum);
        average_events = std::accumulate(sums.begin(), sums.end(), 0.0) / sums.size();
        variance_events = calculateVariance(sums);
    }
    sums.clear();
    int sum = 0;
    DiagnosticsHandlerReportsList records = diagRecHanStorage->getRecords();
    if (records.size() != 0) {
        SimTime current_time = records.begin()->timestamp; //FIXME, TODO, How to do this?
        for (auto it = records.begin(); it < records.end(); it++) {
            if (it->timestamp != current_time) {
                sums.push_back(sum);
                sum = 0;
                current_time = it->timestamp;
            }
            sum ++;
        }
        if ((sum > 0) || (sums.size() == 0)) sums.push_back(sum);
        average_handlers = std::accumulate(sums.begin(), sums.end(), 0.0) / sums.size();
        variance_handlers = calculateVariance(sums);
    }
    std::map<std::string, double> stats;
    stats.insert({"average_events_per_instant", average_events});
    stats.insert({"variance_events_per_instant", variance_events});
    stats.insert({"average_handlers_per_instant", average_handlers});
    stats.insert({"variance_handlers_per_instant",  variance_handlers});

    return stats;
}

double Diagnostics::complexityVariance(const std::array<long, MAX_COMPLEXITY> complexities) const {
    long number_of_expressions = 0;
    double sum_of_complexities = 0;
    for (int i=1; i < MAX_COMPLEXITY; i++) {
        sum_of_complexities += complexities[i] * i;
        number_of_expressions += complexities[i];
    }
    if (number_of_expressions > 1) {
        double average = sum_of_complexities/number_of_expressions;
        double sum_of_diff_squared = 0;
        for (int i=1; i < MAX_COMPLEXITY; i++) {
            sum_of_diff_squared += complexities[i] * ( i - average ) * ( i - average );

        }
        return sum_of_diff_squared/ (number_of_expressions - 1);
    }
    return 0;
}

void Diagnostics::storeRunCounterValues() {
    m_counters_history.push_back({m_events_current_run,
                                  m_handlers_current_run,
                                  m_callbacks_current_run,
                                  std::accumulate(m_number_of_triggered_expressions_with_i_complexity.begin(),
                                          m_number_of_triggered_expressions_with_i_complexity.end(), 0),
                                  std::accumulate(m_number_of_registered_expressions_with_i_complexity.begin(),
                                          m_number_of_registered_expressions_with_i_complexity.end(), 0),
                                  m_trig_atom_expr,
                                  m_reg_atom_expr});
}

void Diagnostics::resetRunCounterValues() {
    m_events_current_run = 0;
    m_handlers_current_run = 0;
    m_callbacks_current_run = 0;
    m_trig_atom_expr = 0;
    m_reg_atom_expr = 0;
    m_number_of_registered_expressions_with_i_complexity.fill(0);
    m_number_of_triggered_expressions_with_i_complexity.fill(0);
}

void Diagnostics::incrementHandlersCounter() {
    m_handlers_current_run++;
    m_handlers_total++;
}

void Diagnostics::incrementEventsCounter() {
    m_events_current_run++;
    m_events_total++;
}

void Diagnostics::storeTriggeredUserExprComplexity(const int complexity) {
    if (complexity < MAX_COMPLEXITY) {
        m_number_of_triggered_expressions_with_i_complexity[complexity]++;
    } else {
        m_number_of_triggered_expressions_with_i_complexity[0]++;
    }
}

void Diagnostics::incrementTriggeredAtomExprCounter() {
    m_trig_atom_expr++;
}

void Diagnostics::incrementRegisteredAtomExprCounter(const int amount) {
    m_reg_atom_expr += amount;
    if (amount < MAX_COMPLEXITY) {
        m_number_of_registered_expressions_with_i_complexity[amount]++;
    } else {
        m_number_of_registered_expressions_with_i_complexity[0]++;
    }
}

void Diagnostics::incrementCallbacksCounter() {
    m_callbacks_current_run++;
    m_callbacks_total++;
}