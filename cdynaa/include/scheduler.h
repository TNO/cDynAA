// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_SCHEDULER_
#define INCLUDED_SCHEDULER_
/* Declaration of a base interface for the scheduler.
 */

#include "scheduleruserinterface.h"
#include "schedulerinspectorinterface.h"

namespace dynaa {
class Scheduler : public SchedulerUserInterface, public SchedulerInspectorInterface {
};
}

#endif
