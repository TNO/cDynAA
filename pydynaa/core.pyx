# This file uses NumPy style docstrings: https://github.com/numpy/numpy/blob/master/doc/HOWTO_DOCUMENT.rst.txt
# cython: embedsignature=True
# cython: profile=True

"""PyDynAA is a python wrapping (using Cython) for the core of the cDynAA
discrete event simulator.

"""

# General notes:
# - to avoid naming conflicts all C++ class and function declarations are prefixed with an underscore
#   e.g _ExampleClass, _static_method_from_ExampleClass
# - Python wrapper classes have no underscore, and in some cases are named slightly differently
#   e.g. _CyEntity is wrapped by the Python class Entity
#
# Memory management:
# - The cDynAA classes EventExpression and EventType are immutable and copy 
#   constructed or moved in many cases.
#   The new design makes copies or move these object types whenever possible. 
#   This way there is no need to care about lifetime management of the C++ object.
# - The python Entity constructs a c++ CyEntity for which it holds a shared pointer. Events created by this entity
#   hold a weak pointer to this c++ Entity (casted from CyEntity). When the python Entity leaves scope and is garbage collected
#   it reduces its pointer count to the CyEntity; this probably frees the CyEntity, as by default nothing else shares it pointer.
#   The CyEntity holds a PyObject pointer to the python Entity, which is used for event source lookups. This reference is also
#   weak in the sense that the reference count isn't incremeneted/decremented, and thus also not circular.
# - The cDynAA EventHandler is created by its Python wrapper using a shared_ptr. This shared_ptr is also
#   used directly in the simulation engine, which means both the user and the engine share ownership:
#   either can delete the EventHandler or let it leave scope without affecting the memory of the other.
# - The singleton nature of the C++ SimulationEngine means that it persists in memory
#   while the Python interpretter is exiting and doing final garbage collection.
#   If the Simulation engine isn't reset, it can continue to hold a set of waiting EventHandlers in its
#   handler registry, which in turn hold Python callback functions. This can cause a segmentation fault.
#   To avoid this we reset the SimulationEngine when this module exits (using the atexit module).
#
# Chosen "Pythonic" features:
# - getter methods are replaced by 'property' decorated methods e.g. entity.getUID() -> entity.uid
# - classes with static creator methods (e.g. 'create' or 'getInstance') are wrapped using their class constructors
#   e.g. SimulationEngine() always returns the same instance

import sys
import copy
import warnings
import array
from collections import namedtuple
from cython.operator cimport dereference as deref
from cython.operator cimport postincrement as incr
from cpython.ref cimport PyObject
from libcpp.string cimport string as libcpp_string
from libcpp.unordered_map cimport unordered_map
from libcpp cimport bool
from libcpp.memory cimport shared_ptr
from libcpp.memory cimport make_shared
from libcpp.memory cimport weak_ptr
from libcpp.vector cimport vector
from libcpp.set cimport set
from libcpp.cast cimport dynamic_cast
from libc.math cimport nearbyint, isnan
from libc.math cimport abs as c_abs
from cysignals.signals cimport sig_check, sig_on, sig_off, sig_error
from libcpp.utility cimport move

from pydynaa.PointerHolder cimport PointerHolder as _PointerHolder
from pydynaa.PyObjWrapper cimport PyObjWrapper as _PyObjWrapper

# Import C++ class and function definitions:
from pydynaa.dynaatypes cimport SimTime as _SimTime
from pydynaa.Event cimport EventID as _EventID
from pydynaa.Event cimport ANY_TYPE as _ANY_TYPE_Event
from pydynaa.Event cimport ANY_SOURCE as _ANY_SOURCE_Event
from pydynaa.Event cimport ANY_ID as _ANY_ID_Event
from pydynaa.Event cimport EventSource as _EventSource
from pydynaa.CyEntity cimport CyEntity as _CyEntity
from pydynaa.CyEntity cimport add_to_ptr_vector as _add_to_ptr_vector
from pydynaa.Entity cimport Entity as _Entity
from pydynaa.Entity cimport ENTITY_PREFIX as _ENTITY_PREFIX_Entity
from pydynaa.EventType cimport EventType as _EventType
from pydynaa.EventHandler cimport EventHandler as _EventHandler
from pydynaa.EventHandler cimport DEFAULT_PRIORITY_LEVEL as _DEFAULT_PRIORITY_LEVEL_EventHandler
from pydynaa.EventHandler cimport HIGHEST_PRIORITY_LEVEL as _HIGHEST_PRIORITY_LEVEL_EventHandler
from pydynaa.EventHandler cimport LOWEST_PRIORITY_LEVEL as _LOWEST_PRIORITY_LEVEL_EventHandler
from pydynaa.HandlerToken cimport HandlerToken as _HandlerToken
from pydynaa.EventExpression cimport EventExpression as _EventExpression
from pydynaa.EventExpression cimport EventExpressionType as _EventExpressionType
from pydynaa.HandlerRegistry cimport HandlerTokenSet as _HandlerTokenSet
from pydynaa.SimulationEngine cimport SimulationEngine as _SimulationEngine
from pydynaa.SimulationEngine cimport getInstance as _getInstance_SimulationEngine
from pydynaa.HandlerReport cimport HandlerReport as _HandlerReport

from pydynaa.Diagnostics cimport Diagnostics as _Diagnostics
from pydynaa.Diagnostics cimport DiagnosticsEventTimeline as _DiagnosticsEventTimeline
from pydynaa.Diagnostics cimport DiagnosticsHandlerTimeline as _DiagnosticsHandlerTimeline
from pydynaa.Diagnostics cimport getInstance as _getInstance_Diagnostics
from pydynaa.cythonutils cimport logicalAnd as _logicalAnd
from pydynaa.cythonutils cimport logicalOr  as _logicalOr
from pydynaa.cythonutils cimport hashit as _hashit

# Import Cython class definitions:
from pydynaa.core cimport Entity
from pydynaa.core cimport EventType
from pydynaa.core cimport EventHandler
from pydynaa.core cimport EventExpression
from pydynaa.core cimport SimulationEngine
from pydynaa.core cimport HandlerToken
from pydynaa.core cimport EventReport
from pydynaa.core cimport HandlerReport
from pydynaa.core cimport Diagnostics

# Record deprecation warnings so that we only give them once
_deprecation_warnings = {}

######## Utility functions  ########

cdef bool _default_richcmp_for_type_error(int op = 2):
    if op==2:
        return False
    if op==3:
        return True
    return False

def _warn_deprecated(message, key=None, stacklevel=2):
    """Warn that the called functionality is deprecated.

    Parameters
    ----------
    message : str
        Warning message to print.
    key: str or None, optional
        Unique string for warning to ensure it is only printed once.
    stacklevel : int, optional
        Level in stack to show information such as filename and linenumber for.
        If 1, current location is shown, if 2 location of the caller, etc.
        This wrapper function is ignored (it is stacklevel 0).

    """
    if key is None or _deprecation_warnings.get(key) is None:
        warnings.warn(message, DeprecationWarning, stacklevel=(stacklevel + 1))
        if key is not None:
            _deprecation_warnings[key] = True


cdef public void _call_obj(obj, _HandlerReport handler_report) except * with gil:
    """Invokes a python callable object (functor) 

    Invokes the callable python object 'obj' with the given event.
    This function is used to call python functions as event handlers (callbacks).

    Parameters
    ----------
    obj : a python callable object
        A function that receives a DynAA handler report as parameter is expected.
    handler_report : a DynAA handler report (_HandlerReport)
        The report about which conditions triggered this event handler 
        execution.  See HandlerReport.

    Notes
    -----
        The handler report is a value type in cDynAA and is moved here
        efficiently to the handler report. 

    """
    sig_check()
    try:
        obj(HandlerReport.__wrap__(move(handler_report)))
    except Exception as e:
        _getInstance_SimulationEngine().stop()
        raise e

######## end of utility functions #########

"""Static constants representing any event id, any event type, or any event source.

Notes
------
_ANY_TYPE_Event is an alias to Event::ANY_TYPE in cDynaa engine
_ANY_SOURCE_Event is an alias to Event::ANY_SOURCE in cDynaa engine
_ANY_ID_Event is an alias to Event::ANY_ID in cDynaa engine
"""
ANY_EVENT_TYPE = EventType.__wrap__(_ANY_TYPE_Event)
#TODO: There is a discussion ongoing about representing any source with None.
    # Follow decision of this discussion
ANY_EVENT_SOURCE = Entity() #None 
ANY_EVENT_ID = _ANY_ID_Event



cdef class EventType:
    """Type specification of an event.

    As well as *waiting* for specific events, an :py:class:`~pydynaa.core.Entity`
    can wait for any event of a given *type*.

    Parameters
    ----------
    name : str
        Name of the event type. Does not need to be unique, and should
        thereby not be used as a unique identifier.
    description : str
        Description for the event type. Does not need to be unique, and should
        thereby not be used as a unique identifier.

    Notes
    -----
        The ``name`` and ``description`` of an event type are not unique identifiers.
        Use comparison operators to check equality.

        Constructor creates a cDynAA equivalent event type and wraps it in a python object.

    """
    def __cinit__(self, str name = None, str description = None):
        if (name is None) and (description is None):
            self.__inst = new _EventType()
        else:
            if not isinstance(name, str):
                raise TypeError('arg name wrong type')
            if not isinstance(description, str):
                raise TypeError('arg description wrong type')
            self.__inst = new _EventType(name.encode(), description.encode())

    def __dealloc__(self):
        # Deallocates cDynAA wrapped object
        del self.__inst

    @staticmethod
    cdef __wrap__(const _EventType& event_type):
        # Wraps a copy of the given cDynAA event type.
        # Notice that the given event_type itself is NOT wrapped, but a copy of it.
        # Such approach is slightly less efficient, but safer in terms of memory management
        # in the interface between pydynaa and cDynaa.
        cdef EventType py_result = EventType.__new__(EventType)
        py_result.__inst = new _EventType(event_type)
        return py_result

    @property
    def name(self):
        """str: the name of this event type."""
        return self.__inst.name().decode()

    @property
    def description(self):
        """str: a short description of this event type."""
        return self.__inst.description().decode()
   
    @property
    def id(self):
        """int: a unique identifier for the event type."""
        return self.__inst.id()

    def __richcmp__(self, EventType other, op):
        """Provides comparison operators for event types.

        An event type is equal to other event if it is the same object reference.
        Each event type should be created only once and referenced from the event.

        Returns
        -------
        op_res : bool
            If ``==`` is used, returns true if event types are equal.
            If ``!=`` is used, returns the inverse of ``==``.

        Raises
        ------
        Exception
            If a non-supported operation is used.

        Notes
        -----
            Provides the C++ ``operator==()`` and ``operator!=()`` operations.
            There is no sense in other comparison operations, such as >, <=, etc..

        """
        if op not in (2, 3):
            op_str = {0: '<', 2: '==', 4: '>', 1: '<=', 3: '!=', 5: '>='}[op]
            raise Exception("comparions operator %s not implemented" % op_str)
        if not isinstance(other,EventType):
            return _default_richcmp_for_type_error(op)
        if op==2:
            return deref(self.__inst) == deref(other.__inst)
        if op==3:
            return deref(self.__inst) != deref(other.__inst)
    
    def __str__(self):
        return "{}".format(self.name)

    def __hash__(self):
        return hash(self.id)

cdef class HandlerToken:
    """Token for a event handler registration

    When an event handler is registered in the engine for waiting an event (or expression),
    a token is returned.  This token uniquely identifies this registering act, and
    enables dismissal and inspection.

    Notes
    -----
        Tokens are not made to be created by the user, so creating one will yield an
        invalid token that points to no registered handler.  It can be used only as a dummy
        handler token.

        Valid handler tokens are obtained after invoking 'wait' and 'wait_once' methods.
    """
    def __cinit__(self):
        self.__inst = new _HandlerToken()

    def __dealloc__(self):
        del self.__inst

    @staticmethod
    cdef __wrap__(_HandlerToken handler_token):
        # Wraps a copy of the given cDynAA event type.
        # Notice that the given event_type itself is NOT wrapped, but a copy of it.
        # Such approach is slightly less efficient, but safer in terms of memory management
        # in the interface between pydynaa and cDynaa.
        cdef HandlerToken py_result = HandlerToken.__new__(HandlerToken)
        py_result.__inst = new _HandlerToken(move(handler_token))
        return py_result

    def __richcmp__(self, HandlerToken other, op):
        """Provides comparison operators for event types.

        A handler token  is equal to other event if refers to the same handler-expression registration.

        Returns
        -------
        op_res : bool
            If ``==`` is used, returns true if event types are equal.
            If ``!=`` is used, returns the inverse of ``==``.

        Raises
        ------
        Exception
            If a non-supported operation is used.

        Notes
        -----
            The comparison of two handler tokens is done by using an internal uid that is not
            accessible directly to users.

            Provides the C++ ``operator==()`` and ``operator!=()`` operations.
            There is no sense in other comparison operations, such as >, <=, etc..
        """
        if op not in (2, 3):
            op_str = {0: '<', 2: '==', 4: '>', 1: '<=', 3: '!=', 5: '>='}[op]
            raise Exception("comparions operator %s not implemented" % op_str)
        if other is None:
            return _default_richcmp_for_type_error(op)
        if op==2:
            return deref(self.__inst) == deref(other.__inst)
        if op==3:
            return deref(self.__inst) != deref(other.__inst)

    def __hash__(self):
        cdef size_t _ahash = deref(self.__inst)(deref(self.__inst))
        return hash(_ahash)


cdef class EventReport:
    """Decribes whether and how an atomic event expression was triggered within a handler report.

    Event reports are used by the :class:`~pydynaa.core.HandlerReport` to describe
    the events that triggered its event expression.

    The properties ``trigger_time``, ``trigger_ee``, ``source``, ``type`` and ``id``
    are only meaningful if the atomic event expression was triggered.

    """
    # NOTE EventReport does not need to wrap a C++ object
    # Its C properties are declared within core.pxd

    def __init__(self, *args, **kwargs):
        # EventReport has no public constructors
        if not kwargs.get("__createUnsafeObject__") is True:
            raise Exception("Cannot call this constructor")

    def __cinit__(self):
        self._atom_ee = None
        self._trigger_ee = None
        self._trigger_time = -1.

    @property
    def atom_ee(self):
        """:py:class:`~pydynaa.core.EventExpression`:
        The atomic event expression represented by this report."""
        return self._atom_ee

    @property
    def trigger_ee(self):
        """:py:class:`~pydynaa.core.EventExpression` or None:
        If the atomic event expression was triggered, this expression describes the triggering event.
        Otherwise None."""
        return self._trigger_ee

    @property
    def trigger_time(self):
        """float: the time the atomic event expression was triggered, otherwise -1."""
        return self._trigger_time

    @property
    def istriggered(self):
        """bool: whether the atomic event expression was triggered by an event."""
        return self._trigger_ee is not None

    @property
    def source(self):
        """:py:class:`~pydynaa.core.Entity` or None:
        The source entity of the triggering event."""
        return self._trigger_ee.atomic_source

    @property
    def type(self):
        """:py:class:`~pydynaa.core.EventType` or None:
        The event type of the triggering event."""
        return self._trigger_ee.atomic_type

    @property
    def id(self):
        """int or None:
        The event ID of the triggering event."""
        return self._trigger_ee.atomic_id

    def __hash__(self):
        return hash((hash(self.atom_ee), hash(self.trigger_ee)))

    def __str__(self):
        return "EventReport(atom_ee={}, triggered={})".format(self.atom_ee, self.istriggered)


cdef class HandlerReport:
    """Handler reports describe the conditions of event handler activation.

    An event expression -- which can be also a simple atomic expression -- is triggered when
    its conditions are satisfied for the first time.  Handlers listening for this event
    expression are then notified (their callbacks are called) with a report of the conditions
    that led to it.

    A handler report contains the expression it relates to, a token to inspect its handle,
    and quick basic information about the activation, such as its priority and if it was
    triggered once.

    Notes
    -----
        Handler reports are produced when the event expression is triggered and reflects their
        state at this timepoint. Actualizations of the system after the handler report is
        produced do not update the report.

    """
    def __init__(self, *args, **kwargs):
        # HandlerReport has no public constructors
        if not kwargs.get("__createUnsafeObject__") is True:
            raise Exception("Cannot call this constructor")

    def __cinit__(self):
        # Below lists are computed when first asked for
        self._state_report = None
        self._triggered_events = None

    def __dealloc__(self):
        # Deallocates the cDynAA object wrapped in here
        self.__inst.reset()

    cdef shared_ptr[_HandlerReport] inst(self):
        return self.__inst

    @staticmethod
    cdef __wrap__(_HandlerReport handler_report):
        # Wraps the simulation event with a python object
        cdef HandlerReport py_result = HandlerReport.__new__(HandlerReport)
        py_result.__inst = make_shared[_HandlerReport](move(handler_report))
        return py_result

    @staticmethod
    cdef __wrap_ptr__(shared_ptr[_HandlerReport] event_report_ptr):
        # Wraps the pointer to a simulation event with a python object
        cdef HandlerReport py_result = HandlerReport.__new__(HandlerReport)
        py_result.__inst = event_report_ptr
        return py_result

    @property
    def event_expression(self):
        """ The event expression related to this report """
        cdef _EventExpression event_expression = deref(self.__inst).getEventExpression()
        return EventExpression.__wrap__(move(event_expression))

    @property
    def handler_token(self):
        """ The handler token associated with this report """
        return HandlerToken.__wrap__(deref(self.__inst).getHandlerToken())

    @property
    def handler_priority(self):
        """ The priority associated with the handler of this report """
        return self.__inst.get().getHandlerPriority()

    @property
    def once(self):
        """ True if the handler of this report is invoked once, false otherwise """
        return self.__inst.get().once()

    @property
    def state_report(self):
        """list of :class:`~pydynaa.core.EventReport`:
        list with the trigger state report over each atom in the reported event expression."""
        if self._state_report is not None:
            return self._state_report
        cdef vector[_HandlerReport.EventReport] __states = move(deref(self.__inst).getEventStateReport())
        cdef EventReport event_report
        self._state_report = []
        for idx in range(__states.size()):
            event_report = EventReport.__new__(EventReport)
            event_report._atom_ee = EventExpression.__wrap__(move(__states.at(idx).atom_ee))
            if __states.at(idx).value:
                event_report._trigger_time = move(__states.at(idx).trigger_time)
                event_report._trigger_ee = EventExpression.__wrap__(move(__states.at(idx).last_trigger))
            self._state_report.append(event_report)
        return self._state_report

    @property
    def triggered_events(self):
        """The events that caused this expression to trigger.

        The event reports are sorted by the order in which they occured.
        If this is an atomic expression then the list will contain
        maximally one event.

        Returns
        -------
        list of :class:`~pydynaa.core.EventReport`
            Triggering events sorted by occurance, or an empty list
            if the expression has not yet been triggered.

        """
        if self._triggered_events is not None:
            return self._triggered_events
        cdef EventReport evrep
        cdef list trig_events = [evrep for evrep in self.state_report if evrep.istriggered]
        self._triggered_events = sorted(trig_events, key=lambda x: x.trigger_time)
        return self._triggered_events

cdef class Entity:
    """Anything within the simulation that is capable of generating or responding to events.

    The Entity class should be inherited from rather than instantiated directly.

    If an Entity leaves scope in python it will be garbage collected (GC), i.e. freed from memory,
    similarly to any other python object. This will happen even if the entity is the source of
    events that still exist on the simulation timeline, as these only hold a weak reference
    to it. Events on the simulation timeline that were created by an Entity that has been
    freed from memory will no longer be triggered.

    Notes
    -----
        The Entity class uses the ``__cinit__`` method to construct a new _CyEntity C++
        object on the heap. Because ``__cinit__`` is always called once, Entity can be
        safely subclassed -- which is the intended usage. However, this python class
        should not be used to wrap existing _CyEntity objects using ``__new__``, as the
        creation of a new C++ object cannot be avoided.
    """
    def __cinit__(self):
        # This method is always called when the object is created i.e. also
        # when subclassing, or when constructing using __new__().
        cdef PyObject* pyobj = <PyObject*> self
        self.__inst = make_shared[_CyEntity](pyobj)

    def __dealloc__(self):
        self.__inst.reset()

    def remove(self):
        """Removes the entity from the simulation.

        """
        self.__inst.reset()

    cdef shared_ptr[_CyEntity] inst(Entity self):
        return self.__inst

    @property
    def uid(self):
        """int: returns a unique identifier for this entity in the simulation.

        Each entity has a unique identifier in the simulation.
        """
        cdef long int _r = self.__inst.get().getUID()
        return _r

    cpdef _schedule_now(self, EventType event_type):
        """Schedules an event of given type at the current simulation time (instantaneous)

        Use this function to schedule immediately an event of given event type.

        Parameters
        ----------
        event_type : :py:class:`~pydynaa.core.EventType`
            The type of the event to be scheduled.

        Returns
        -------
        :py:class:`EventExpression`
            An atomic expression representing the scheduled event.

        Raises
        ------
        TypeError
            If ``event_type`` is not an :py:class:`~pydynaa.core.EventType`.

        """
        if not isinstance(event_type, EventType):
            raise TypeError("arg event_type wrong type")
        py_result = EventExpression.__wrap__(move(self.__inst.get().scheduleNow(deref(event_type.__inst))))
        return py_result

    cpdef _schedule_after(self, double interval, EventType event_type):
        """Schedules an event of given event type to be triggered after a given interval -- counting from the actual time.

        Use this function to schedule an event after a certain delay counting from the current simulation time.
        The event triggered will be of the given event type.

        Parameters
        ----------
        interval : float
            The interval after which the event will be triggered.
        event_type : :py:class:`~pydynaa.core.EventType`
            The type of the event to be scheduled.

        Returns
        -------
        :py:class:`~pydynaa.core.EventExpression`
            An atomic expression representing the scheduled event.

        Raises
        ------
        TypeError
            If a ``event_type`` is passed that is not an :py:class:`~pydynaa.core.EventType`.
            Or if ``interval`` is not of float type.
        Exception
            If ``interval`` is negative

        """
        if not isinstance(interval, float):
            raise TypeError('arg interval wrong type')
        if not isinstance(event_type, EventType):
            raise TypeError('arg event_type wrong type')
        cdef _SimTime _current_time = SimulationEngine().get_current_time()
        cdef _SimTime _interval = interval
        cdef _SimTime _delta = c_abs(_current_time - interval)
        cdef _SimTime _rounding_warning_limit = 1e12
        if _delta >= _rounding_warning_limit:
            import warnings
            warnings.warn("Operating scheduling intervals with large difference to simulation time " +
                "may lead to rounding problems.")
            warnings.warn("Using scheduling after with {} as interval when current simulation time is {}".format(
                interval, _current_time))      
        py_result = EventExpression.__wrap__(move(self.__inst.get().scheduleAfter(<_SimTime>interval, deref(event_type.__inst))))
        return py_result

    cpdef _schedule_at(self, double instant, EventType event_type):
        """Schedules an event of a given event type to be triggered at a given future time

        Use this function to schedule an event at a certain time instant in the future (ahead in the simulation time).
        The event triggered will be of the given event type.

        Parameters
        ----------
        instant: float
            The instant at which the event will be triggered.
        event_type : :py:class:`~pydynaa.core.EventType`
            The type of the event to be scheduled.

        Returns
        -------
        :py:class:`EventExpression`
            An atomic expression representing the scheduled event.

        Raises
        ------
        TypeError
            If ``event_type`` not an :py:class:`~pydynaa.core.EventType`.
            Or if ``instant`` is not of double type.
        Exception
            Or if ``instant`` is negative.
            Or if ``instant`` is past in the simulation time.
        """
        if not isinstance(instant, float):
            raise TypeError('arg instant wrong type')
        if not isinstance(event_type, EventType):
            raise TypeError('arg event_type wrong type')
        py_result = EventExpression.__wrap__(move(self.__inst.get().scheduleAt(<_SimTime>instant, deref(event_type.__inst))))
        return py_result

    cpdef _unschedule(self, EventType event_type=None, long event_id=ANY_EVENT_ID):
        """Removes future events scheduled by this entity.

        Parameters
        ----------
        event_type: :py:class:`~pydynaa.core.EventType` or None, optional
            Type of events to unschedule.
        event_id : long, optional
            ID of the event(s) to unschedule.

        Returns
        -------
        int
            The number of events that were unscheduled.
        """
        if event_type is None:
            event_type = ANY_EVENT_TYPE
        cdef int _count = self.__inst.get().unschedule(deref(event_type.__inst), event_id)
        return _count

    cpdef _wait_once(self, handler, Entity entity=None, EventType event_type=None, 
        long event_id=ANY_EVENT_ID, EventExpression expression=None):
        """Waits once for a certain event to be triggered.

        Waits exactly once until a matching event is triggered.
        Waiting for events requires declaring a matching (filter) criteria 
        composed of a source, an event type, an event, or and event id; 
        or a combination of those.

        Parameters
        ----------
        handler : :py:class:`~pydynaa.core.EventHandler`
            The handler to be invoked when a triggered event or expression 
            matches the given filter.
        entity : :py:class:`~pydynaa.core.Entity` or None, optional
            Only events from this entity will match the filter.
        event_type: :py:class:`~pydynaa.core.EventType` or None, optional
            Only events of the given event_type will match the filter.
        event_id : long, optional
            Only the specific event (with given event id) will match the filter.
        expression : :py:class:`~pydynaa.core.EventExpression` or None, optional
            Event expression to wait on. If specified (not None) it overrides 
            the other event related parameters (``entity``, ``event_type``, 
            ``event``, ``event_id``).

        Raises
        ------
        TypeError
            If passed parameters are not of the expected type.

        Notes
        -----
            If a source is provided, only events coming from this source (entity) will match the filter and trigger the given event handler.
            If an event type is provided, only events of this given event type with match the filter and triffer the given event handler.
            If and event_id is provided, only the event with the given event id matches the filter and triggers the given event handler.
            Combinations of those are possible, for example, if a source and an event type is provided, only events from this source and of
            the given event type will match the filter and trigger the event handler.

            Only the first event matching the filter will cause an invocation of 
            the event handler. The following matches will not invoke the handler
            anymore.
        """
        cdef EventHandler eventHandler
        cdef _HandlerToken hToken
        if entity is None:
            entity = ANY_EVENT_SOURCE
        elif not isinstance(entity, Entity):
            raise TypeError('arg entity wrong type')
        if event_type is None:
            event_type = ANY_EVENT_TYPE
        elif not isinstance(event_type, EventType):
            raise TypeError('arg event_type wrong type')
        if event_id is None:
            event_id = ANY_EVENT_ID
        elif not isinstance(event_id, int):
            raise TypeError('arg event_id wrong type')
        if isinstance(handler, EventHandler):
            eventHandler = handler
            if expression is not None:
                hToken = self.__inst.get().waitOnce(eventHandler.__inst, <_EventExpression> deref(expression.__inst))
            elif entity is not ANY_EVENT_SOURCE:
                hToken = self.__inst.get().waitOnce(eventHandler.__inst, <_EventSource> entity.__inst,
                                           deref(event_type.__inst), (<_EventID>event_id))
            else:
                hToken = self.__inst.get().waitOnce(eventHandler.__inst, deref(event_type.__inst), (<_EventID>event_id))
            return HandlerToken.__wrap__(hToken)
        else:
            raise TypeError('arg handler wrong type')

    cpdef _wait(self, handler, Entity entity=None, EventType event_type=None, 
                long event_id=ANY_EVENT_ID, bool once=False, EventExpression expression=None):
        """Waits continuously for certain events to be triggered.

        Waits until a matching event is triggered. This wait remains until it is explicitly dismissed.
        Waiting for events requires declaring a matching (filter) criteria composed of a source, an event type, an event, or and event id; or a combination of
        those.

        Parameters
        ----------
        handler : :py:class:`~pydynaa.core.EventHandler`
            The event or expression handler to be invoked when a triggered event matches the given filter.
        entity : :py:class:`~pydynaa.core.Entity` or None, optional
            Only events from this entity will match the filter.
        event_type: :py:class:`~pydynaa.core.EventType` or None, optional
            Only events of the given event_type will match the filter.
        event_id : long, optional
            Only the specific event (with given event id) will match the filter.
        once : bool, optional
            Whether to only wait once.
        expression : :py:class:`~pydynaa.core.EventExpression` or None, optional
            Event expression to wait on. If specified (not None) it overrides the other event related
            parameters (``entity``, ``event_type``, ``event``, ``event_id``).

        Returns
        ------
        handler_token: :py:class:'~pydynaa.core.HandlerToken'
            A token for accessing and querying the registered record related to this handler-expression combination.

        Raises
        ------
        TypeError
            If passed parameters are not of the expected type.

        Notes
        -----
            If a source is provided, only events coming from this source (entity) will match the filter and trigger the given event handler.
            If an event type is provided, only events of this given event type with match the filter and triffer the given event handler.
            If an once is True, the filter expect until the specific given event is triggered and then the handler is dismissed -- as if
            _waitOnce is called.
            If an event expression is provided, all other parameters (beside the event handler) are ignored.
            If and event_id is provided, only the event with the given event id matches the filter and triggers the given event handler.
            Combinations of those are possible, for example, if a source and an event type is provided, only events from this source and of the given event type
            will match the filter and trigger the event handler.

            All future events matching the filter will cause an invocation of the event handler.  To stop this behaviour, the event handler has to be dismissed.
        """
        if once :
            return self._wait_once(handler, entity=entity, event_type=event_type, 
                                   event_id=event_id, expression=expression)
        cdef EventHandler eventHandler
        cdef _HandlerToken hToken
        if entity is None:
            entity = ANY_EVENT_SOURCE
        elif not isinstance(entity, Entity):
            raise TypeError('arg entity wrong type')
        if event_type is None:
            event_type = ANY_EVENT_TYPE
        elif not isinstance(event_type, EventType):
            raise TypeError('arg event_type wrong type')
        if event_id is None:
            event_id = ANY_EVENT_ID
        if not isinstance(event_id, int):
            raise TypeError('arg event_id wrong type')
        if isinstance(handler, EventHandler):
            eventHandler = handler
            if expression is not None:
                hToken = self.__inst.get().wait(eventHandler.__inst, <_EventExpression> deref(expression.__inst))
            elif entity is not ANY_EVENT_SOURCE:
                hToken = self.__inst.get().wait(eventHandler.__inst, <_EventSource> entity.__inst,
                                           deref(event_type.__inst), (<_EventID>event_id))
            else:
                hToken = self.__inst.get().wait(eventHandler.__inst, deref(event_type.__inst), (<_EventID>event_id))
            return HandlerToken.__wrap__(hToken)
        else:
            raise TypeError('arg handler wrong type')

    cpdef _dismiss(self, HandlerToken handlerToken):
        """Dismiss an event handler based on its registration token.

        Parameters
        ----------
        handlerToken : :py:class:`~pydynaa.core.HandlerToken`
            The token received during the registration of a event handler (and event expression).

        Notes
        -----
            handlerToken(s) are received when 'wait' calls are invoked.  They uniquely identify the
            registration of a event handler (and its callback) and the event (expression) they are
            liked to.
            Invoking a dismiss cancels the given registration and the event handler is not invoked
            anymore as a result of triggering the corresponding event (expression).

        Raises
        ------
        TypeError
            If passed parameters are not of the expected type.
        """
        cdef EventHandler eventHandler
        if handlerToken is None:
            raise TypeError('handler token is invalid (None), cannot dismiss')
        self.__inst.get().dismiss(deref(handlerToken.__inst))

    def __richcmp__(self, other, op):
        # Wraps Entity::operator== and Entity::operator!= methods
        if op not in (2, 3):
            op_str = {0: '<', 2: '==', 4: '>', 1: '<=', 3: '!=', 5: '>='}[op]
            raise Exception("comparions operator %s not implemented" % op_str)
        if not isinstance(other, Entity):
            return False
        cdef Entity other_casted = other
        cdef Entity self_casted = self
        if op==2:
            return deref(self_casted.__inst.get()) == deref(other_casted.__inst.get())
        if op==3:
            return deref(self_casted.__inst.get()) != deref(other_casted.__inst.get())

    def __str__(self):
        cdef libcpp_string _r = _ENTITY_PREFIX_Entity
        return "{}_{}".format(_r.decode(), self.uid)

    def __hash__(self):
        return hash(self.uid)


cdef class EventHandler:
    """Handles an event for an Entity using a callback function.

    An EventHandler must be passed to the :py:class:`~pydynaa.core.Entity`
    *wait()* and *wait_once()* methods for an event.

    Parameters
    ----------
    callback_function : function
        Callback function.
    identifier : str or "", optional
        Identifier for event handler.
    safe_guards: list of :py:class:`~pydynaa.core.Entity`, optional
        A list of Entities to be used as safe guards of this event handler.
    priority : int
        Priority of this event handler.

    Attributes
    ----------
    default_priority : int
        Default priority level for event handlers.
    highest_priority : int
        The highest priority level for event handlers.
    lowest_priority : int
        The lowest priority level for event handlers.

    Notes
    -----
        One can pass python entities (derived from Entity) as safe guards to a callback.
        The existence of the safe guards will be checked before invoking the callback, that is
        the callback will not be invoked if the any of the safe guard entities do not exist anymore.

        When creating a callback_function using a _closure_ beware of late-binding_.
        For example, consider the case of creating callback functions in
        a loop with the help of *lambda* functions:

        .. code-block:: python

            for i in range(5):
                handlers.append(EventHandler(lambda event: print(i)))

        The effect of late binding is that each callback will print _i=5_. One
        way to counter-act late binding a default argument can be used to bind
        immediately:

        .. code-block:: python

            for i in range(5):
                handlers.append(EventHandler(lambda event, i=i: print(i)))

    .. _late-binding: http://docs.python-guide.org/en/latest/writing/gotchas/#late-binding-closures

        The EventHandler class uses the ``__cinit__`` method to construct a new C++ _EventHandler
        C++ object on the heap. Because ``__cinit__`` is always called once, EventHandler can be
        safely subclassed. However, this python class should no be used to wrap
        existing C++ _EventHandler objects using ``__new__``, as the creation of a new C++ object
        cannot be avoided.

    Examples
    --------

    .. code-block:: python

        def callbackA(report):
            print("Expression".format(report.event_expression))

        def callbackB(time):
            print("Event occured at {}".format(time))

        class CB(Entity):
            def callbackAsMethod(report):
                print("This is a method used as callback. Use CB as safe guard")


        handlerA = EventHandler(callbackA)
        handlerB = EventHandler(lambda event: callbackB(6))
        entity = CB()
        # sets entity as a safe guard.  callback is only invoked if entity still exists.
        handlerC = EventHandler(lambda event: entity.callbackAsMethod(event), [entity])
    """
    default_priority = _DEFAULT_PRIORITY_LEVEL_EventHandler
    highest_priority = _HIGHEST_PRIORITY_LEVEL_EventHandler
    lowest_priority = _LOWEST_PRIORITY_LEVEL_EventHandler

    @staticmethod
    cdef __add_to_safe_guards__(vector[weak_ptr[_Entity]] vec, Entity entity):
        # Utility function to create and add safe guards
        _add_to_ptr_vector(vec, entity.inst())

    def __cinit__(self, callback_function, str identifier="", list safe_guards=[],
                  int priority=_DEFAULT_PRIORITY_LEVEL_EventHandler):
        # Initializes an EventHandler by creating a new _EventHandler and wrapping it in a python object
        # Note that cinit constructor is always called.
        cdef _PyObjWrapper f = _PyObjWrapper(callback_function)
        cdef libcpp_string _id_cppstr = identifier.encode()
        cdef vector[weak_ptr[_Entity]] _safe_guards
        for entity in safe_guards:
            EventHandler.__add_to_safe_guards__(_safe_guards, entity)
        if len(identifier) > 0:
            self.__inst = make_shared[_EventHandler](f, _id_cppstr, _safe_guards, priority)
        else:
            self.__inst = make_shared[_EventHandler](f, _safe_guards, priority)

    def __dealloc__(self):
        self.__inst.reset()

    cdef shared_ptr[_EventHandler] inst(self):
        return self.__inst

    def handle(self, HandlerReport handler_report):
        """Handle an event.

        Parameters
        ----------
        ev : :py:class:`~pydynaa.core.HandlerReport`
            Event report used when handling.

        """
        self.__inst.get().handle(deref(handler_report.inst().get()))

    @property
    def identifier(self):
        """str: identifier of event handler."""
        cdef libcpp_string _r = self.__inst.get().getIdentifier()
        return _r.decode()

    @property
    def priority(self):
        """int: priority level of this event handler."""
        cdef int _r = self.__inst.get().getPriorityLevel()
        return _r

    def __str__(self):
        return "{}".format(self.identifier)


# Dummy entity used by EventExpression to deprecate its wait() method
cdef Entity _dummy_entity = Entity()


cdef class EventExpression:
    """Defines an expression over event conditions.

    Event expressions allows the user to *wait* on combinations of events,
    such as an event (fromSourceA, any type, any id) & an event (fromSourceB, typeC, any id).
    Meaning that the expression will only be valid when both event conditions are
    met at least once, that is, events matching these conditions were issued at least once.

    Very complex event expressions can be produced combining them in AND (&) and OR (|) operations.

    Parameters
    ----------
    source : :py:class:`~pydynaa.core.Entity` or None, optional
        The Entity that should be source of the event expression condition.
        If :py:class:`~pydynaa.core.ANY_EVENT_SOURCE` or ``None`` then
        any source is matched.
    event_type : :py:class:`~pydynaa.core.EventType` or None, optional
        The EventType that should be matched in the event expression condition.
        If :py:class:`~pydynaa.core.ANY_EVENT_TYPE` or ``None`` then
        any event type is matched.
    event_id: int, optional
        The EventID to be matched in the event expression condition.
        If :py:class:`~pydynaa.core.ANY_EVENT_ID` or ``-1`` then
        any event is matched.
    descriptor: string, optional
        A string describing this expression.  For user defined purposes only.

    Notes
    -----
        The basic event expression -- also called event condition or atomic event expression --
        is a a single condition (source, type, id) that has to be matched. In each part of the condition,
        wildcards
        :py:class:`~pydynaa.core.ANY_EVENT_SOURCE`,
        :py:class:`~pydynaa.core.ANY_EVENT_TYPE`, or
        :py:class:`~pydynaa.core.ANY_EVENT_ID`
        can be used respectively.

    Examples
    --------

    Wait for either a rain or snow event type to occur (from any entities).
    Specifying a callback function to ``wait()`` is optional.
    Calling ``wait()`` will reset the expression ``value`` to ``False``.
    The expression will become ``True`` once the event conditions are (again) matched.

    .. code-block:: python

        weather_service = Entity()
        gaia = Entity()
        evtype_rain = EventType("Rain", "Rainy weather")
        evtype_snow = EventType("Snow", "Snowy weather")

        # Define a composite event expression:
        evexpr_rain = EventExpression(source=gaia, event_type=evtype_rain)
        evexpr_snow = EventExpression(source=gaia, event_type=evtype_snow)
        evexpr_wetweather = evexpr_rain | evexpr_snow

        #TODO: Code below is not valid anymore. Re-write
        def warning_callback(event_report):
            condition = triggered_expr.triggered_events[-1].type.description
            print("Be careful: it's wet outside ({}).".format(condition))

        # Wait on event expression:
        weather_service._wait(warning_callback, expression=evexpr_wetweather)

        # Schedule snow and run simulation:
        gaia._schedule_now(evtype_snow)
        SimulationEngine().run()
    """
    ATOMIC = _EventExpressionType.Atomic
    AND = _EventExpressionType.AND
    OR = _EventExpressionType.OR

    def __cinit__(self, Entity source=None, EventType event_type=None, int event_id=ANY_EVENT_ID, str descriptor=None):
        if source is None:
            source = ANY_EVENT_SOURCE
        if event_type is None:
            event_type = ANY_EVENT_TYPE
        if event_id is None:
            event_id = ANY_EVENT_ID

        if source is ANY_EVENT_SOURCE:
            # Translates ANY_EVENT_SOURCE here to its c equivalent _ANY_SOURCE_Event for internal use
            if descriptor is None:
                self.__inst = new _EventExpression(<_EventSource> _ANY_SOURCE_Event, deref(event_type.__inst), (<_EventID>event_id))
            else:
                self.__inst = new _EventExpression(descriptor.encode(), <_EventSource> _ANY_SOURCE_Event, deref(event_type.__inst), (<_EventID>event_id))
        else:
            if descriptor is None:
                self.__inst = new _EventExpression(<_EventSource> source.__inst, deref(event_type.__inst), (<_EventID>event_id))
            else:
                self.__inst = new _EventExpression(descriptor.encode(), <_EventSource> source.__inst, deref(event_type.__inst), (<_EventID>event_id))
    
    def __dealloc__(self):
        # Deallocates the cDynAA object wrapped in here
        del self.__inst

    @staticmethod
    cdef __wrap__(_EventExpression other):
        # Wraps a copy of the given cDynAA event expression.
        # Notice that the given event expression itself is NOT wrapped, but a copy of it.
        # Such approach is slightly less efficient, but safer in terms of memory management
        # in the interface between pydynaa and cDynaa.
        cdef EventExpression py_result = EventExpression.__new__(EventExpression)
        py_result.__inst = new _EventExpression(move(other))
        return py_result

    @property
    def atomic_id(self):
        """int or None:
        The event ID of the atomic expression or None if not an atomic event expression."""
        cdef long int c_evid
        if self.type == self.ATOMIC:
            return self.__inst.atomicEventID()
        else:
            return None

    @property
    def atomic_type(self):
        """:py:class:`~pydynaa.core.EventType` or None:
        The event type of the atomic expression or None if not an atomic event expression."""
        if self.type == self.ATOMIC:
            return EventType.__wrap__(self.__inst.atomicEventType())
        else:
            return None

    @property
    def atomic_source(self):
        """:py:class:`~pydynaa.core.Entity`:
        The source entity of the atomic expression.  In case of atomic 
        expressions, value can also be the special Entity 
        `pydynaa.core.ANY_EVENT_SOURCE` indicating any source.
        if this expression is not an atomic event expression, returns None.
        """
        cdef shared_ptr[_Entity] c_source
        cdef object py_source
        if self.type == self.ATOMIC:
            try:
                c_source = self.__inst.atomicEventSource().lock()
            except RuntimeError:
                raise LookupError("Source Entity has been removed from Simulator")
            if c_source == _ANY_SOURCE_Event:
                py_source = ANY_EVENT_SOURCE
            else:
                # Assume Entity created py pydynaa ...
                py_source = <object> (<_CyEntity*> c_source.get()).pyobj()
                if py_source is None:
                    raise LookupError("Source Entity has been removed from Simulator")
            return py_source
        else:
            return None

    @property
    def type(self):
        """{self.ATOMIC, self.AND, self.OR}: type of event expression."""
        return self.__inst.type()

    @property
    def descriptor(self):
        """Descriptor string given to this expression"""
        return self.__inst.descriptor().decode()

    @property
    def first_term(self):
        """:py:class:`~pydynaa.core.EventExpression`: first term in this expression."""
        return EventExpression.__wrap__(self.__inst.firstTerm())

    @property
    def second_term(self):
        """:py:class:`~pydynaa.core.EventExpression`: second term in this expression."""
        return EventExpression.__wrap__(self.__inst.secondTerm())

    @property
    def atoms(self):
        """list: Retrieves the atomic operations of this event expression"""
        cdef vector[_EventExpression ] _atoms = move(self.__inst.atoms())
        result_atoms = []
        cdef _EventExpression _atom
        for idx in range(_atoms.size()):
            #Stepwise moves are necessary to hack move behavior into cython generated code!
            _atom = move(_atoms.at(idx))
            result_atoms.append(EventExpression.__wrap__(move(_atom)))
        return result_atoms

    def __and__(EventExpression self, other):
        """AND operator for event expressions.

        Parameters
        ----------
        other : :py:class:`~pydynaa.core.EventExpression` 
            Expressions can be created AND operating other expressions 

        Raises
        ------
        TypeError
            If passed parameters are not of the expected type.
        """
        if other is None:
            return self.copy()
        elif self is None:
            return other.copy()
        other_as_ee = <EventExpression?> other
        return EventExpression.__wrap__(move(_logicalAnd(deref(self.__inst), deref(other_as_ee.__inst))))

    def __or__(EventExpression self, other):
        """OR operator for event expressions.

        Parameters
        ----------
        other : :py:class:`~pydynaa.core.EventExpression`
            Expressions can be created OR operating other expressions

        Raises
        ------
        TypeError
            If passed parameters are not of the expected type.
        """
        if other is None:
            return self.copy()
        elif self is None:
            return other.copy()
        other_as_ee = <EventExpression?> other
        return EventExpression.__wrap__(move(_logicalOr(deref(self.__inst) , deref(other_as_ee.__inst)))) 

    def __richcmp__(EventExpression self, EventExpression other, op):
        """Provides comparison operators for event expression.

        An event expression is equal to other event expression
        if it has the same type, and defines the same logical expression.

        The value of the terms are not checked.

        Returns
        -------
        op_res : bool
            If ``==`` is used, returns true if expressions define the same logical combination.
            If ``!=`` is used, returns the inverse of ``==``.

        Raises
        ------
        Exception
            If a non-supported operation is used.

        Notes
        -----
            Provides the C++ ``operator==()`` and ``operator!=()`` operations.
            There is no sense in other comparison operations, such as >, <=, etc..
        """
        if op not in (2, 3):
            op_str = {0: '<', 2: '==', 4: '>', 1: '<=', 3: '!=', 5: '>='}[op]
            raise Exception("comparions operator %s not implemented" % op_str)
        if other is None:
            return _default_richcmp_for_type_error(op)
        if op==2:
            return deref(self.__inst) == deref(other.__inst)
        if op==3:
            return deref(self.__inst) != deref(other.__inst)

    def copy(self):
        """Copy this event expression.

        This gives a unique copy of this event expression that is untriggered.

        Returns
        -------
        :py:class:`~pydynaa.core.EventExpression`
            A copy of the event expression.
        """
        return copy.copy(self)

    def __copy__(self):
        return EventExpression.__wrap__(deref(self.__inst))

    def __hash__(self):
        """
        Provides a hash function for an event expression
        """
        return _hashit(deref(self.__inst))



cdef class SimulationEngine:
    """The engine of the simulation.

    Used to run, stop and reset discrete event simulations.

    There is only one single (singleton) simulation engine per python session.

    This simulation engine instance is the same for any python SimulationEngine
    instance that is created. This behavior is ensured during initialization.

    Notes
    -----
        Only one instance can be active per Python interpreter.
    """
    def __init__(self):
        self.__inst = _PointerHolder[_SimulationEngine](& _getInstance_SimulationEngine())

    cdef _PointerHolder[_SimulationEngine] inst(self):
        return self.__inst

    def run(self, end_time=None):
        """Run the simulation.

        Parameters
        ----------
        end_time : float or None, optional
            Time up to which to run the simulation, not including this instant itself.
            In other words, events scheduled at end_time will not be issued.
            If ``end_time`` is ``None``, simulation will run until there are no more events in the timeline.

        Notes
        -----
            It can be the case that there will always be a new event in the timeline.
            In this case, not passing an end_time to the run may cause a never ending simulation.

        Examples
        --------

        .. code-block:: python

            sim_engine = SimulationEngine()
            sim_engine.run(10.)  # runs simulation up to simulation time 10
            # State of the simulation is now at time 10

            # Resumes the simulation and stops only when there are no further events
            # in the timeline:
            sim_engine.run()

        """
        #sig_on()
        self._run(end_time)
        #sig_off()

    cdef void _run(self, object end_time) except *:
        # Wrapping run() with a pure cdef cleanly handles exceptions, and
        # avoids SystemError exceptions wrapping the original exception
        cdef _SimulationEngine* simengine = self.__inst.get()
        if end_time is None:
            simengine.run()
        elif isinstance(end_time, (float, int)):
            simengine.run(end_time)
        else:
            raise TypeError("end_time parameter is not a real number.")

    def run_event(self, num_events=1):
        """Run the simulation for the specified number of events, by default 1 event.

        Parameters
        ----------
        num_events : int or None, optional
            Number of individual events that will be simulated. This number can be larger
            than the number of events scheduled for the next instance. The next num_events will
            be simulated regardless of whether they belong to the same instant or not. The order
            of events within the same instant is arbitrary, therefore the precedence of one event
            over another in the same instant is arbitrary.
            The value of events must be a positive integer. (events > 0)

        Examples
        --------

        .. code-block:: python

            sim_engine = SimulationEngine()
            sim_engine.run_event(4)  # runs the next 4 events
            sim_engine.run_event()  # runs one more event
            # The SimulationEngine has simulated the first 5 events

        """
        #sig_on()
        if (num_events > 0):
            self._runEvent(num_events)
        #sig_off()

    cdef void _runEvent(self, object num_events) except *:
        # Wrapping run() with a pure cdef cleanly handles exceptions, and
        # avoids SystemError exceptions wrapping the original exception
        cdef _SimulationEngine* simengine = self.__inst.get()
        if isinstance(num_events, (int)):
            simengine.runEvent(num_events)
        else:
            raise TypeError("num_events parameter is not an integer.")

    def run_instant(self, num_instants=1):
        """Run the simulation for the specified number of instants, by default 1 instant.

        Parameters
        ----------
        num_instants : int or None, optional
            Number of instants that will be simulated.
            The value of events must be a positive integer. (events > 0)

        Notes
        -----
            If an instant has several events (e.g. 7 events) and we simulate some of its events
            (i.e. run_event(3)), there will be several events remaining in the current instant. In
            that case, the first instant will run until the end of the current instant (e.g. the
            remaining 4 events) and update the simulation time to the beginning of the next instant.
            If the callback from an event schedules a new event at the same time using scheduleNow(),
            this event will be considered as part of the same instant, potentially having infinite loops.

        Examples
        --------

        .. code-block:: python

            sim_engine = SimulationEngine()
            sim_engine.run_instant(4)  # runs the next 4 instants
            sim_engine.run_instant()  # runs one more instant
            # The SimulationEngine has simulated the first 5 instants
        """
        #sig_on()
        if (num_instants > 0):
            self._runInstant(num_instants)
        #sig_off()

    cdef void _runInstant(self, object num_instants) except *:
        # Wrapping run() with a pure cdef cleanly handles exceptions, and
        # avoids SystemError exceptions wrapping the original exception
        cdef _SimulationEngine* simengine = self.__inst.get()
        if isinstance(num_instants, (int)):
            simengine.runInstant(num_instants)
        else:
            raise TypeError("num_instants parameter is not an integer.")

    def stop(self):
        """Permanently stop the simulation.

        Simulation must be ``reset`` in order to run again from the beginning.

        Notes
        -----
            Stops the simulation means the actual time of the simulation is frozen and further (future)
            events are not thrown.  No further activation of any entity in the simulation.
        """
        self.__inst.get().stop()

    def reset(self):
        """Reset the simulation by clearing all scheduled events and setting time to 0.

        Notes
        -----
            After `stop()`, if the simulation is not reset, invoking `run()` will resume the
            simulation from the moment it stopped.

            After reset, the simulation time is sent back to time zero,
            all events are eliminated from the timeline, and existing entities are disposed.
            It brings the simulator to an initial state without any simulation entities in it.
        """
        self.__inst.get().reset()

    # Not fully functional yet.  Need testing and evetually debugging.
    # def pause(self):
    #     """Pause the simulation keeping all the scheduled events and the current simulationTime.

    #     Notes
    #     -----
    #         The simulation can be resumed via run(...), run_event(...) or run_instant(...)
    #     """
    #     self.__inst.get().pause()

    cdef double get_current_time(self):
        # Fast C access to the current time. Can only be called by Cython.
        cdef double _r = self.__inst.get().getCurrentTime()
        return _r

    cdef int get_state(self):
        # Fast C access to the state. Can only be called by Cython.
        cdef double _r = self.__inst.get().getState()
        return (<int>nearbyint(_r))

    @property
    def current_time(self):
        """float: return the current time in the simulation."""
        return self.get_current_time()

    @property
    def state(self):
        """float: return the current state of the simulation.
        ``0`` = *idle*, ``1`` = *running*, ``2`` = *stopped*."""
        # TODO wrap with a SimState class?
        return self.get_state()

    #Deprecated
    #TODO: has to be redesigned
    def get_expression_handlers(self, EventExpression expression):
        """Retrieve list of registered expression handlers for an event expression.

        Parameters
        ----------
        expression : :py:class:`~pydynaa.core.EventExpression`
            Event expression to inspect for registered expression handlers.

        Returns
        -------
        list of (:py:class:`~pydynaa.core.HandlerToken`):
            List of tokens associated with handlers of this expression.  Further access to the handlers, and
            their info use these tokens.
        """
        _warn_deprecated("There are no expression handlers anymore. "
                         " get_expression_handlers return now HandlerTokens to access",
                         " handlers associated to the expression",
                         key="SimEngine.get_expression_handlers")
        cdef list result = []
        return result


cdef class SimState:
    Idle = 0
    Running = 1
    Stopped = 2

cdef class Diagnostics:
    """The diagnostics module of the simulation.

    This module can be used to get insights into the simulation. It allows
    the user to see previous events and handlers, future events, and statistics
    such as the number of events and many more

    Notes
    -----
    There can only be one instance of the diagnostics module per Python interpreter
    """
    descriptions = { 'num_current_instants': "Number of instants scheduled",
                        'average_delta_time_between_instants': "Average time interval between scheduled instants",
                        'variance_delta_time_between_instants': "Variance in time interval between scheduled instants",
                        'num_events_scheduled': "Number of scheduled events",
                        'average_num_events_scheduled': "Average number of events scheduled per instant",
                        'variance_num_events_scheduled': "Variance in number of events scheduled per instant",
                        'num_registered_handlers': "Number of scheduled handlers",

                        'total_events_triggered': "Number of events triggered in all the runs",
                        'average_events_per_instant': "Average number of events triggered per instant",
                        'variance_events_per_instant': "Variance in number of events triggered per instant",
                        'total_handlers_triggered': "Number of events triggered in the latest run",
                        'total_events_triggered': "Number of handlers triggered in all the runs",
                        'average_handlers_per_event': "Average number of handlers triggered per event",
                        'average_handlers_per_instant': "Average number of handlers triggered per instant",
                        'variance_handlers_per_instant': "Variance in number of handlers triggered per instant",
                        'current_run_handlers_triggered': "Number of handlers triggered in the latest run",
                        'user_expressions_triggered': "Number of user expressions triggered in the latest run",
                        'average_complexity_triggered_expressions': "Average complexity of the user expressions triggered",
                        'variance_complexity_triggered_expressions': "Variance in complexity of the user expressions triggered",
                        'user_expressions_registered': "Number of user expressions registered in the latest run",
                        'average_complexity_registered_expressions': "Average complexity of the user expressions registered",
                        'variance_complexity_registered_expressions': "Variance in complexity of the user expressions registered",
                        'atomic_expressions_triggered': "Number of atomic expressions triggered in the latest run",
                        'atomic_expressions_registered': "Number of atomic expressions registered in the latest run",
                        'number_of_callback_functions': "Number of callback functions executed",
                        'number_of_callback_functions_current_run': "Number of callback functions executed in the latest run"
                        }
    def __init__(self):
        self.__inst = _PointerHolder[_Diagnostics](&_getInstance_Diagnostics())

    cdef _PointerHolder[_Diagnostics] inst(self):
        return self.__inst

    cdef _cEvTimelineToPyEvTimeline(self, _DiagnosticsEventTimeline cTimeline):
        pyTimeline= []
        for i in cTimeline:
            pyTimeline.append((i.first, EventExpression.__wrap__(i.second)))
        return pyTimeline

    cdef _cHanTimelineToPyHanTimeline(self, _DiagnosticsHandlerTimeline cTimeline):
        pyTimeline= []
        for i in cTimeline:
            pyTimeline.append((i.first, HandlerReport.__wrap_ptr__(i.second)))
        return pyTimeline

    def get_past_events_last_run(self):
        """
        Returns a timeline with the events triggered in the last run, this
        is generally the current active run.

        Parameters
        ----------
        It doesn't take any parameters

        Returns
        -------
        list of (pair of ( long and :py:class:`~pydynaa.core.EventExpression`))
            A timeline formed by an ordered list of pairs. Each pair includes the
            simulation time (as an integer) and the EventExpression matching the event
            triggered at that simulation time.
            The list is ordered with the first element corresponding to the first event
            that was triggered and the last element is the last event, the most recent.
            If several events are triggered simultaneously, each will have a different 
            entry in the list and the order of those relative to each other is undefined
            and not meaningful.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._getPastEventsLastRun()

    cdef _getPastEventsLastRun(self):
        return Diagnostics._cEvTimelineToPyEvTimeline(self, self.__inst.get().getPastEventsLastRun())
    
    def get_past_events(self, run):
        """
        Returns a timeline of triggered events for a specific run or all the
        previous runs.

        Parameters
        ----------
        OPTIONAL run: integer
            This integer specifies the index of the run for which we will generate
            the timeline. The first run takes the index 0.
            If an index is not specified or is a negative number, all the runs
            (including the current one) are returned.
            If the index is larger than the index of the last run (e.g. we have done 3
            runs with indexes [0,1,2], and we ask for index 9), it defaults to
            getPastEventsLastRun().

        Returns
        -------
        list of (pair of ( long and :py:class:`~pydynaa.core.EventExpression`))
            A timeline formed by an ordered list of pairs. Each pair includes the
            simulation time (as an integer) and the EventExpression matching the event
            triggered at that simulation time.
            The list is ordered with the first element corresponding to the first event
            that was triggered and the last element is the last event, the most recent.
            If several events are triggered simultaneously, each will have a different 
            entry in the list and the order of those relative to each other is undefined
            and not meaningful.

        Notes
        -----
        The index of the run is not returned in any case. The user should keep track
        of which timeline corresponds to which run.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._getPastEvents(run)

    cdef _getPastEvents(self, int run):
        if run is None:
            return Diagnostics._cEvTimelineToPyEvTimeline(self, self.__inst.get().getPastEvents())
        else:
            return Diagnostics._cEvTimelineToPyEvTimeline(self, self.__inst.get().getPastEvents(run))

    def get_past_statistics(self):
        """
        Returns a dictionary with different statistical values related to the past
        of the simulation.

        Parameters
        ----------
        This function doesn't take any parameters.

        Returns
        -------
        dictionary with the following keywords:
            "total_events_triggered": Total number of events triggered in all the runs.
            "current_run_events_triggered": Number of events triggered in the current/last run.
            "average_events_per_instant": Average number of events triggered per instant.
            "variance_events_per_instant": Variance in the number of events triggered per 
                                            instant.
            "total_handlers_triggered": Total number of handlers triggered in all the runs.
            "current_run_handlers_triggered": Number of handlers triggered in the 
                                            current/last run.
            "average_handlers_per_event": Average number of handlers triggered per event in
                                            the current/last run.
            "average_handlers_per_instant": Average number of handlers triggered per instant.
            "variance_handlers_per_instant": Variance in the number of handlers triggered per
                                            instant.
            "user_expressions_triggered": Number of user event expressions triggered in the 
                                            current/last run.
            "user_expressions_registered": Number of user event expressions registered in the 
                                            current/last run.
            "atomic_expressions_triggered": Number of atomic event expressions triggered in 
                                            the current/last run.
            "atomic_expressions_registered": Number of atomic event expressions registered in 
                                            the current/last run.
            "average_complexity_registered_expressions": Average complexity (number of atomic 
                                            expressions) in the expressions registered in the
                                            current/last run.
            "variance_complexity_registered_expressions": Variance in the complexity (number
                                            of atomic expressions) in the expressions registered
                                            in the current/last run.
            "average_complexity_triggered_expressions": Average complexity (number of atomic 
                                            expressions) in the expressions triggered in the 
                                            current/last run.
            "variance_complexity_triggered_expressions": Variance in the complexity (number of 
                                            atomic expressions) in the expressions triggered in 
                                            the current/last run.
            "number_of_callback_functions": Number of callback functions executed.
            "number_of_callback_functions_current_run": Number of callback functions executed in
                                            the current run.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._getPastStatistics()
    
    cdef _getPastStatistics(self):
        cdef map[libcpp_string, double] info = self.__inst.get().getPastStatistics()
        cdef map[libcpp_string, double].iterator it = info.begin()
        cdef dict infodict = {}
        while(it != info.end()):
            infodict[deref(it).first.decode()] = deref(it).second
            incr(it)
        return infodict

    def get_past_handlers(self, time):
        """
        Returns a vector of HandlerTokens corresponding to the handlers triggered
        at the specified time.

        Parameters
        ----------
        time: long
        Long representing the simulation time for which the user requests the vector
        of triggered handler tokens.

        Returns
        -------
        list of (:py:class:`~pydynaa.core.HandlerToken`))
            A list of the HandlerTokens corresponding to the triggered handlers for the
            requested time.
        """
        if SimulationEngine().state == SimState.Running:
            raise Exception("You should pause the simulation before running this\n")
        return self._getPastHandlers(time)

    cdef _getPastHandlers(self, _SimTime time):
        cdef libcpp_vector[ _HandlerToken ] cVector = self.__inst.get().getPastHandlers(time)
        pyVector= []
        for i in cVector:
            pyVector.append(HandlerToken.__wrap__(i))
        return pyVector

    def get_past_handler_reports(self, HandlerToken token):
        """
        Returns a timeline of all the handler reports related to the given HandlerToken

        Parameters
        ----------
        token: :py:class:`~pydynaa.core.HandlerToken`
        HandlerToken for which the query is made

        Returns
        -------
        list of (pair of (:py:class:`~pydynaa.core.HandlerToken`))
            A list of the HandlerTokens corresponding to the triggered handlers for the
            requested time.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        cdef _HandlerToken new_token = deref(token.__inst)
        return self._getPastHandlerReports(new_token)

    cdef _getPastHandlerReports(self, _HandlerToken token):
        return Diagnostics._cHanTimelineToPyHanTimeline(self, self.__inst.get().getPastHandlerReports(token))

    def get_future_events(self):
        """
        Returns a timeline of all the currently scheduled events.

        Parameters
        ----------
        This function doesn't take parameters

        Returns
        -------
        list of (pair of ( long and :py:class:`~pydynaa.core.EventExpression`))
            A timeline formed by an ordered list of pairs. Each pair includes the
            simulation time (as an integer) for which the event is scheduled and
            an atomic EventExpression matching the event.
            The list is ordered with the first element corresponding to the nearest event
            (i.e. smallest time).
            If several events are scheduled for the same instant, each will have a different
            entry in the list and the order of those relative to each other is undefined
            and not meaningful.
        
        Notes
        -----
        This function only returns a snapshot of the currently scheduled events. These events could
        schedule new events or unschedule existing ones, so the events in the timeline might not
        reflect the actal behavior of the simulation.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._getFutureEvents()

    cdef _getFutureEvents(self):
        return Diagnostics._cEvTimelineToPyEvTimeline(self, self.__inst.get().getFutureEvents())

    def get_future_statistics(self):
        """
        Returns a dictionary with different statistical values related to the future of
        the simulation.

        Parameters
        ----------
        This function doesn't take any parameters.

        Returns
        -------
        dictionary with the following keywords:
        "num_current_instants": Number of instants for which there are scheduled events.
        "average_delta_time_between_instants": Average interval between instants scheduled
                                                in the future.
        "variance_delta_time_between_instants": Variance in the time between instants
                                                scheduled in the future.
        "num_events_scheduled": Number of events scheduled.
        "average_num_events_scheduled": Average number of events scheduled per instant.
        "variance_num_events_scheduled": Variance in the number of events scheduled per instant.
        "num_registered_handlers": Number of registered handlers.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._getFutureStatistics()
        
    
    cdef _getFutureStatistics(self):
        cdef map[libcpp_string, double] info = self.__inst.get().getFutureStatistics()
        cdef map[libcpp_string, double].iterator it = info.begin()
        cdef dict infodict = {}
        while(it != info.end()):
            infodict[deref(it).first.decode()] = deref(it).second
            incr(it)
        return infodict

    def get_time_of_most_recent_event(self):
        """
        Returns the time of the last event that was triggered.

        Parameters
        ----------
        This function doesn't take parameters

        Returns
        -------
        integer
            Time of the last event triggered.
        
        Notes
        -----
        This value is equivalent to the current simulation time.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._getTimeOfMostRecentEvent()

    cdef _SimTime _getTimeOfMostRecentEvent(self):
        return self.__inst.get().getTimeOfMostRecentEvent()

    def get_time_of_next_event(self):
        """
        Returns the time of the next event that will be triggered.

        Parameters
        ----------
        This function doesn't take parameters

        Returns
        -------
        integer
            Time of the next event in the timeline
        
        Notes
        -----
        This value is always equal (i.e. in case of simultaneous events) or larger than the time of the last event.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._getTimeOfNextEvent()
    
    cdef _SimTime _getTimeOfNextEvent(self):
        return self.__inst.get().getTimeOfNextEvent()

    def get_next_events(self, n):
        """
        Returns a timeline including the next `n` scheduled events or all the
        events scheduled for the next instant.

        Parameters
        ----------
        OPTIONAL `n`: integer
            This integer specifies number of events that should be returned. If it
            is not specified, it returns all the events scheduled for the next simulation
            instant.

        Returns
        -------
        list of (pair of ( long and :py:class:`~pydynaa.core.EventExpression`))
            A timeline formed by an ordered list of pairs. Each pair includes the
            simulation time (as an integer) for which the event is scheduled and
            an atomic EventExpression matching the event.
            The list is ordered with the first element corresponding to the nearest event
            (i.e. smallest time).
            If several events are scheduled for the same instant, each will have a different 
            entry in the list and the order of those relative to each other is undefined
            and not meaningful.
        
        Notes
        -----
        If `n` is smaller than 1, an empty timeline is returned.
        If there are less than `n` events, it returns all the scheduled events.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._getNextEvents(n)

    cdef _getNextEvents(self, int n):
        if n is None:
            return Diagnostics._cEvTimelineToPyEvTimeline(self, self.__inst.get().getEventsAtNextInstant())
        else:
            return Diagnostics._cEvTimelineToPyEvTimeline(self, self.__inst.get().getNextEvents(n))
    
    def activate_diagnostics_storage(self):
        """
        Activates the diagnostics module to start gathering information.

        Parameters
        ----------
        This function doesn't take parameters

        Returns
        -------
        This function doesn't return any values
        
        Notes
        -----
        The diagnostics module can only return information for events triggered after the
        activation of the diagnostics module. Gathering information can have some negative
        impact in the performace of the simulation.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._activateDiagnosticsStorage()
    
    cdef void _activateDiagnosticsStorage(self):
        self.__inst.get().activateDiagnosticsStorage()
        return

    def deactivate_diagnostics_storage(self):
        """
        Deactivates the diagnostics module.

        Parameters
        ----------
        This function doesn't take parameters

        Returns
        -------
        This function doesn't return any values
        
        Notes
        -----
        Deactivating the diagnostics module efectively removes it's effects on the
        simulation and it's performance.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._deactivateDiagnosticsStorage()
    
    cdef void _deactivateDiagnosticsStorage(self):
        self.__inst.get().deactivateDiagnosticsStorage()
        return

    def is_diagnostics_storage_active(self):
        """
        Returns true if diagnostics is active or false otherwise

        Parameters
        ----------
        This function doesn't take parameters

        Returns
        -------
        bool:
            Represents if the current state is Active (true) or Inactive (false)
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._isDiagnosticsStorageActive()

    cdef _isDiagnosticsStorageActive(self):
        return self.__inst.get().isDiagnosticsStorageActive()

    def clear_memory(self):
        """
        Deletes the memory of the diagnostics module, returning it to the state before
        the simulation was started.

        Parameters
        ----------
        This function doesn't take parameters

        Returns
        -------
        This function doesn't return any values
        
        Notes
        -----
        It is unclear if this function is useful for the user at any point.
        TODO: determine if users need this at all. If not, remove to avoid accidents.
        """
        if SimulationEngine().state == SimState.Running :
            raise Exception("You should pause the simulation before running this\n")
        return self._reset()
    
    cdef void _reset(self):
        self.__inst.get().reset()
        return

    @property
    def num_events(self):
        """int: number of events currently on the simulation timeline."""
        info = self._getFutureStatistics()
        return info["num_events_scheduled"]

    @property
    def num_instants(self):
        """int: number of time instants currently on the simulation timeline."""
        info = self._getFutureStatistics()
        return info["num_current_instants"]

    @property
    def num_handlers(self):
        """int: number of event handlers currently registered in the simulation."""
        info = self._getFutureStatistics()
        return info["num_registered_handlers"]

def _cleanup_on_module_exit():
    # Function called to clean up on module exit
    SimulationEngine().reset()


import atexit
atexit.register(_cleanup_on_module_exit)
