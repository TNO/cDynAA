// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_SCHEDULERINSPECTOR_
#define INCLUDED_SCHEDULERINSPECTOR_
/* Scheduler inspector interface
 */

//#include "dynaatypes.h"
#include "event.h"
#include <functional>
#include <unordered_set>
#include <vector>

namespace dynaa {

struct EventSetHash {
    std::size_t operator()(const Event &event) const {
        std::size_t h1 = std::hash<EventID> {}(event.id());
        return h1;
    }
};

/**
* SchedulerInspectorInterface is an interface for inspectors of the DynAA simulator scheduler.
* It provides a restricted (inspector) view for the scheduler, which allows to monitor the
* state of the schedule but without acting (e.g. scheduling events) in the schedule.
*
* For the design of DynAA, it means that all classes that needs to get information from the schedule,
* BUT are not entities should use the SchedulerInspectorInterface as a way to avoid messing
* around with the issueing of schedules.
*
*/
class SchedulerInspectorInterface {
public:
    static constexpr SimTime NO_NEXT_EVENT = -1;

    /**
       * Get the list of all events that occur on a specific timepoint. After
       * calling this function the events will still be present at the given
       * instant.
       *
       * @param timepoint
       *            A double specifying the point in time to get all events from
       * @return An array of Events that occur at the given time
       * @see #removeEventsAt(double)
       */
    virtual EventSet getEventsAt(const SimTime timepoint) const = 0;

    /**
       * Returns a list of all points in time where one or more events are
       * scheduled.
       *
       * @return an array of doubles which represents the points in time.
       * @see #getNextInstant()
       */
    virtual std::vector<SimTime> getInstants() const = 0;

    /**
       * Returns the next point in time where one or more events are scheduled.
       *
       * @return the double which represents the point in time.
       * @see #getInstants()
       */
    virtual SimTime getNextEventTime() const = 0;

    /**
       * Returns a copy of the future timeline
       *
       * @return Timeline as a map using SimTime as key and EventSet as values
       */
    virtual Timeline getTimeline() const = 0;

    /**
       * Remove all events on a point from the timeline. After calling this
       * function the events will no longer be visible to the Scheduler. The
       * previous values will be returned, or null will be returned if no events
       * were set at that timepoint.
       *
       * @param timepoint
       *            A double specifying the point in time to get all events from
       * @return An array of Events that occur at the given time
       * @see #getEventsAt(double)
       *
       */
    virtual EventSet removeEventsAt(SimTime timepoint) = 0;

    /**
     * Constructors, virtual destructor and assignment operator.
     */
    SchedulerInspectorInterface() = default;
    SchedulerInspectorInterface(const SchedulerInspectorInterface&) = default;
    SchedulerInspectorInterface(SchedulerInspectorInterface&&) = default;
    virtual ~SchedulerInspectorInterface() = default;
    SchedulerInspectorInterface& operator=(const SchedulerInspectorInterface&) = default;
    SchedulerInspectorInterface& operator=(SchedulerInspectorInterface&&) noexcept = default;
};
}

#endif
