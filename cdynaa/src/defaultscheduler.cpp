/*
file: defaultscheduler.cpp
author: Julio Oliveira and Coen van Leeuwen, TNO

Member functions of the class defaultscheduler
*/

#include <algorithm>
#include "../include/defaultscheduler.h"
#include "../include/simengine.h"

using namespace dynaa;
using namespace std;

void DefaultScheduler::scheduleNow(const Event event) {
    m_timeline[SimulationEngine::getInstance().getCurrentTime()].insert(event);
}

void DefaultScheduler::scheduleAt(const SimTime instant, const Event event) {
    if ( (instant < 0) || (instant < SimulationEngine::getInstance().getCurrentTime())) {
        throw InvalidTimePointException();
    }
    m_timeline[instant].insert(event);
}

void DefaultScheduler::scheduleAfter(const SimTime interval, const Event event) {
    if (interval < 0)  {
        throw InvalidTimePointException();
    }
    SimTime actualTime = SimulationEngine::getInstance().getCurrentTime();
    if ( (actualTime - interval) >= _rounding_errors_warning_limit ) {
        std::cerr << "Scheduling intervals {" << interval << "} with large " <<
         "differences to current simulation time {" << actualTime << "}  can " << 
         "introduce rounding errors." << std::endl;
    } 
    m_timeline[actualTime + interval].insert(event);
}

void DefaultScheduler::unschedule(const Event event) {
    // TODO if events has timestamps, could use here to unschedule more efficiently!
    SimTime currentTime = SimulationEngine::getInstance().getCurrentTime();
    for (Timeline::iterator iter = m_timeline.lower_bound(currentTime);
            iter != m_timeline.end(); ++iter) {
        if (iter->second.erase(event) == 1) {
            // Event erased
            if (iter->second.empty()) {
                m_timeline.erase(iter);
                return;
            }
        }
    }
}

SimTime DefaultScheduler::getNextInstant() const {
    // Get the next time instant on the timeline after the current time.
    // Assume here that there are no instants in the timeline 'before' the actual instant.
    if (m_timeline.empty()) {
        return SchedulerInspectorInterface::NO_NEXT_INSTANT;
    } else {
        // Let n be the number of events in the timeline.
        // Using lower_bound and the current time gives  O(log(n)) lookup
        //SimTime currentTime = SimulationEngine::getInstance().getCurrentTime();
        //Timeline::const_iterator iter = m_timeline.lower_bound(currentTime);
        //return iter->first;
        // Because the timeline is ordered, and instants are removed before proceeding
        // to the next, O(1) is also possible access
        return m_timeline.begin()->first;
    }
}

EventSet DefaultScheduler::removeEventsAt(const SimTime instant) {
    auto search = m_timeline.find(instant);
    if (search != m_timeline.end()) {
        EventSet elements = search->second;
        m_timeline.erase(search);
        return elements;
    } else {
        return EventSet();
    }
}

vector<SimTime> DefaultScheduler::getInstants() const {
    vector<SimTime> instants = vector<SimTime>();
    for (auto entry = m_timeline.begin(); entry != m_timeline.end(); entry++) {
        instants.insert(instants.begin(), entry->first);
    }
    return instants;
}

long DefaultScheduler::getNumberOfInstants() const {
    return m_timeline.size();
}

long DefaultScheduler::getNumberOfEvents() const {
    long num_events = 0;
    for (auto entry = m_timeline.begin(); entry != m_timeline.end(); entry++) {
        num_events += entry->second.size();
    }
    return num_events;
}

EventSet DefaultScheduler::getEventsAt(const SimTime instant) const {
    auto entry = m_timeline.find(instant);
    if (entry == m_timeline.end()) {
        return EventSet();
    } else {
        return entry->second;
    }
}

void DefaultScheduler::reset() {
    m_timeline.clear();
}
