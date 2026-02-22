# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
"""
Definition of Cython classes.
"""
from libcpp.memory cimport shared_ptr
from libcpp.memory cimport weak_ptr
from libcpp.vector cimport vector
from libcpp.map cimport map
from libcpp cimport bool
from libcpp.string cimport string as libcpp_string
from libcpp.vector cimport vector as libcpp_vector
from pydynaa.PointerHolder cimport PointerHolder as _PointerHolder
from pydynaa.Event cimport Event as _Event
from pydynaa.Entity cimport Entity as _Entity
from pydynaa.EventHandler cimport EventHandler as _EventHandler
from pydynaa.EventType cimport EventType as _EventType
from pydynaa.HandlerReport cimport HandlerReport as _HandlerReport
from pydynaa.HandlerToken cimport HandlerToken as _HandlerToken
from pydynaa.CyEntity cimport CyEntity as _CyEntity
from pydynaa.SimulationEngine cimport SimulationEngine as _SimulationEngine
from pydynaa.EventExpression cimport EventExpression as _EventExpression
from pydynaa.dynaatypes cimport SimTime as _SimTime
from pydynaa.Diagnostics cimport DiagnosticsEventTimeline as _DiagnosticsEventTimeline
from pydynaa.Diagnostics cimport DiagnosticsHandlerTimeline as _DiagnosticsHandlerTimeline
from pydynaa.Diagnostics cimport Diagnostics as _Diagnostics

cdef class EventType:
    cdef _EventType* __inst 
    @staticmethod
    cdef __wrap__(const _EventType & event_type) 

cdef class HandlerToken:
    cdef _HandlerToken* __inst
    @staticmethod
    cdef __wrap__(const _HandlerToken & handler_token) 

cdef class Entity:
    cdef shared_ptr[_CyEntity] __inst
    cdef shared_ptr[_CyEntity] inst(self) 
    cpdef _schedule_now(self, EventType event_type)
    cpdef _schedule_after(self, double interval, EventType event_type)
    cpdef _schedule_at(self, double instant, EventType event_type)
    cpdef _unschedule(self, EventType event_type=*, long event_id=*)
    cpdef _wait_once(self, handler, Entity entity=*,
                     EventType event_type=*, long event_id=*,
                     EventExpression expression=*)
    cpdef _wait(self, handler, Entity entity=*,
                EventType event_type=*, long event_id=*,
                bool once=*, EventExpression expression=*)
    cpdef _dismiss(self, HandlerToken handlerToken)

cdef class EventHandler:
    cdef shared_ptr[_EventHandler] __inst
    cdef shared_ptr[_EventHandler] inst(self)
    @staticmethod
    cdef __add_to_safe_guards__(vector[weak_ptr[_Entity]] vec, Entity entity)


cdef class EventReport:
    cdef EventExpression _atom_ee
    cdef EventExpression _trigger_ee  
    cdef double _trigger_time

cdef class HandlerReport:
    cdef shared_ptr[_HandlerReport] __inst
    cdef shared_ptr[_HandlerReport] inst(self)
    @staticmethod
    cdef __wrap__(const _HandlerReport & other)
    @staticmethod
    cdef __wrap_ptr__(shared_ptr[_HandlerReport] & event_report_ptr)
    cdef list _state_report
    cdef list _triggered_events


cdef class EventExpression:
    cdef _EventExpression* __inst
    @staticmethod
    cdef __wrap__(_EventExpression other)


cdef class SimulationEngine:
    cdef _PointerHolder[_SimulationEngine] __inst
    cdef _PointerHolder[_SimulationEngine] inst(self)
    cdef double get_current_time(self)
    cdef int get_state(self)
    cdef void _run(self, object end_time) except *
    cdef void _runEvent(self, object events) except *
    cdef void _runInstant(self, object instants) except *

cdef class Diagnostics:
    cdef _PointerHolder[_Diagnostics] __inst
    cdef _PointerHolder[_Diagnostics] inst(self)
    cdef _cEvTimelineToPyEvTimeline(self, _DiagnosticsEventTimeline ctimeline)
    cdef _cHanTimelineToPyHanTimeline(self, _DiagnosticsHandlerTimeline cTimeline)
    cdef _getPastEventsLastRun(self)
    cdef _getPastEvents(self, int run)
    cdef _getPastStatistics(self)
    cdef _getPastHandlers(self, _SimTime time)
    cdef _getPastHandlerReports(self, _HandlerToken token)
    cdef _getFutureEvents(self)
    cdef _getFutureStatistics(self)
    cdef _SimTime _getTimeOfMostRecentEvent(self)
    cdef _SimTime _getTimeOfNextEvent(self)
    cdef _getNextEvents(self, int n)
    cdef void _activateDiagnosticsStorage(self)
    cdef void _deactivateDiagnosticsStorage(self)
    cdef _isDiagnosticsStorageActive(self)
    cdef void _reset(self)