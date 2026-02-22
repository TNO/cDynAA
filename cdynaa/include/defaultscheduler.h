// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_SCHEDULER_
#define INCLUDED_SCHEDULER_

#include <map>
#include <vector>

#include "dynaatypes.h"
#include "schedulerinspectorinterface.h"
#include "scheduleruserinterface.h"

namespace dynaa {

/**
 * Core class offering a default scheduler implementation for cDynAA
 *
 * The DefaultScheduler is offers a facade to
 *
 */
class DefaultScheduler :  virtual public SchedulerInspectorInterface,  virtual public SchedulerUserInterface {

private:

    Timeline m_timeline;
    EventSet currentEvents;
    SimTime currentInstant;
    SimTime getNextInstantTime() const;

public:
    /**
    *  Default inlined constructor
    */
    DefaultScheduler() : m_timeline() {}

    void scheduleNow(Event event);
    void scheduleAt(SimTime instant, Event event);
    void scheduleAfter(SimTime interval, Event event);
    void unschedule(const Event& event);
    int unschedule(const SourceUID sourceUID, const EventType& eventType, const EventID eventID);
    SimTime getNextEventTime() const;
    EventSet removeEventsAt(const SimTime instant);
    EventSet getEventsAt(const SimTime instant) const;
    Event popNextEvent();
    std::vector<SimTime> getInstants() const;
    long getNumberOfInstants() const;
    long getNumberOfEvents() const;
    Timeline getTimeline() const;
    void reset();
};
}

#endif
