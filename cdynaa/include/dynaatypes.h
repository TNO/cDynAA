// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_DYNAATYPES_
#define INCLUDED_DYNAATYPES_

#include <stdio.h>
#include <map>
#include <unordered_set>

namespace dynaa {

//Forward declarations
class Event;

struct EventSetHash;

typedef std::unordered_set<Event, EventSetHash> EventSet;

typedef long double SimTime;

typedef std::map<SimTime, EventSet> Timeline;

typedef long SourceUID;

}

#endif
