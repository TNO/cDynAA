/*
file: defaultscheduler.cpp
author: Julio Oliveira and Coen van Leeuwen, TNO

Member functions of the class defaultscheduler
*/

#include <algorithm>
#include "../include/defaultscheduler.h"
#include "../include/simengine.h"
#include "../include/entity.h"

using namespace dynaa;
using namespace std;

void DefaultScheduler::scheduleNow(Event event) {
    m_timeline[SimulationEngine::getInstance().getCurrentTime()].insert(std::move(event));
}

void DefaultScheduler::scheduleAt(const SimTime instant, Event event) {
    if ( (instant < 0) || (instant < SimulationEngine::getInstance().getCurrentTime())) {
        throw InvalidTimePointException();
    }
    m_timeline[instant].insert(std::move(event));
}

void DefaultScheduler::scheduleAfter(const SimTime interval, Event event) {
    if (interval < 0)  {
        throw InvalidTimePointException();
    }
    SimTime currentTime = SimulationEngine::getInstance().getCurrentTime();
    if ( (currentTime - interval) >= _rounding_errors_warning_limit ) {
        std::cerr << "Scheduling intervals {" << interval << "} with large " <<
                  "differences to current simulation time {" << currentTime << "}  can " <<
                  "introduce rounding errors." << std::endl;
    }
    m_timeline[currentTime + interval].insert(std::move(event));
}

void DefaultScheduler::unschedule(const Event& event) {
    // NOTE unscheduling based on an event should be deprecated, as user no longer keeps access to events
    SimTime currentTime = SimulationEngine::getInstance().getCurrentTime();
    for (auto iter = m_timeline.lower_bound(currentTime);
            iter != m_timeline.end(); ++iter) {
        if (iter->second.erase(event) == 1) {
            // Event erased
            if (iter->second.empty()) {
                m_timeline.erase(iter);
            }
            return;
        }
    }
}

int DefaultScheduler::unschedule(const SourceUID sourceUID, const EventType& eventType, const EventID eventID) {
    // FIXME below implementation is very (too) expensive
    // Possible alternative approaches:
    // - Store list of unscheduled events within an Entity. When an event is triggered, check it's source to see
    //   whether it has been unscheduled. This list should stay pruned (by time?) to not grow too large.
    // - Make use of the scheduled time (instant) to find the event in the timeline. The time instant could
    //   be optional parameter to unschedule, and returned for convenience by the schedule methods,
    SimTime currentTime = SimulationEngine::getInstance().getCurrentTime();
    int count = 0;
    const bool uniqueEvent = (eventType != Event::ANY_TYPE && eventID != Event::ANY_ID);
    // loop over instants on timeline
    for (Timeline::iterator iter = m_timeline.lower_bound(currentTime);
            iter != m_timeline.end();) {
        for (auto it = iter->second.cbegin(); it != iter->second.cend();) {
            const Event& event = *it;
            // Check if event type matches
            if (eventType != Event::ANY_TYPE && eventType != event.type()) {
                ++it;
                continue;
            }
            // Check if event ID matches
            if (eventID != Event::ANY_ID && eventID != event.id()) {
                ++it;
                continue;
            }
            // Check element still has valid source, and if so if it matches
            shared_ptr<Entity> eventSource;
            try {
                eventSource = event.source();
            } catch (InvalidEventSourceException & e) {
                // Let sourceless event be triggered and removed naturally later
                ++it;
                continue;
            }
            if (sourceUID != eventSource->getUID()) {
                ++it;
                continue;
            }
            // Everything matches -> remove event
            it = iter->second.erase(it);
            count++;
            if (uniqueEvent) {
                break;
            }
        }
        if (iter->second.empty()) {
            iter = m_timeline.erase(iter);
        } else {
            ++iter;
        }
        if (count > 0 && uniqueEvent) {
            break;
        }
    }
    return count;
}

SimTime DefaultScheduler::getNextInstantTime() const {
    // Get the next time instant on the timeline after the current time.
    // Assume here that there are no instants in the timeline 'before' the actual instant.
    if (m_timeline.empty()) {
        return SchedulerInspectorInterface::NO_NEXT_EVENT;
    } else {
        return m_timeline.begin()->first;
    }
}

SimTime DefaultScheduler::getNextEventTime() const {
    // Get the time of the next event, which can be the same instant if there
    // are more events in the current instant, or the time of the next instant
    return (currentEvents.empty()) ? getNextInstantTime() : currentInstant;
}

EventSet DefaultScheduler::removeEventsAt(const SimTime instant) {
    auto search = m_timeline.find(instant);
    if (search != m_timeline.end()) {
        EventSet elements = std::move(search->second);
        m_timeline.erase(search);
        return elements;
    }
    return EventSet();
}

Event DefaultScheduler::popNextEvent() {
    if (currentEvents.empty()) {
        if (m_timeline.empty()) {
            //return Event();
        }
        std::tie(currentInstant, currentEvents) = *m_timeline.begin();
        m_timeline.erase(m_timeline.begin());
    }
    Event event = *currentEvents.begin();
    currentEvents.erase(currentEvents.begin());
    return event;
}

vector<SimTime> DefaultScheduler::getInstants() const {
    vector<SimTime> instants = vector<SimTime>();
    for (auto entry : m_timeline) {
        instants.insert(instants.begin(), entry.first);
    }
    return instants;
}

long DefaultScheduler::getNumberOfInstants() const {
    if (m_timeline.begin()->first == SimulationEngine::getInstance().getCurrentTime()) {
        return m_timeline.size();
    } else {
        return m_timeline.size() + ((currentEvents.empty()) ? 0 : 1);
    }
}

long DefaultScheduler::getNumberOfEvents() const {
    long num_events = 0;
    for (auto entry : m_timeline) {
        num_events += entry.second.size();
    }
    return num_events + currentEvents.size();
}

EventSet DefaultScheduler::getEventsAt(const SimTime instant) const {
    auto entry = m_timeline.find(instant);
    if (entry == m_timeline.end()) {
        return EventSet();
    }
    return entry->second;

}

void DefaultScheduler::reset() {
    m_timeline.clear();
    currentEvents.clear();
    currentInstant = 0;
}

Timeline DefaultScheduler::getTimeline() const {
    SimTime current_time = SimulationEngine::getInstance().getCurrentTime();
    if (currentEvents.size() == 0 || m_timeline.begin()->first == current_time) {
        return m_timeline;
    } else {
        Timeline full_timeline = m_timeline;
        full_timeline.insert(full_timeline.begin(), std::make_pair(current_time, currentEvents));
        return full_timeline;
    }
}
