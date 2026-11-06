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
# - The cDynAA classes Event and EventType are immutable and copy constructed in many cases.
#   The new design makes copies of these object types whenever possible. This way there
#   is no need to care about lifetime management of the C++ object.
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

#cimport cpython as cp
import sys
import copy
import warnings
from cython.operator cimport dereference as deref
from cython.operator cimport postincrement as incr
#from cpython.exc cimport PyErr_CheckSignals, PyErr_SetString
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

from pydynaa.PointerHolder cimport PointerHolder as _PointerHolder
from pydynaa.PyObjWrapper cimport PyObjWrapper as _PyObjWrapper

# Import C++ class and function definitions:
from pydynaa.dynaatypes cimport SimTime as _SimTime
from pydynaa.Event cimport Event as _Event
from pydynaa.Event cimport EventID as _EventID
from pydynaa.Event cimport ANY_TYPE as _ANY_TYPE_Event
from pydynaa.Event cimport ANY_SOURCE as _ANY_SOURCE_Event
from pydynaa.Event cimport ANY_ID as _ANY_ID_Event
from pydynaa.Event cimport EventSource as _EventSource
from pydynaa.CyEntity cimport CyEntity as _CyEntity
from pydynaa.CyEntity cimport add_to_ptr_vector
from pydynaa.Entity cimport Entity as _Entity
from pydynaa.Entity cimport ENTITY_PREFIX as _ENTITY_PREFIX_Entity
from pydynaa.EventType cimport EventType as _EventType
from pydynaa.EventHandler cimport EventHandler as _EventHandler
from pydynaa.EventHandler cimport DEFAULT_PRIORITY_LEVEL as _DEFAULT_PRIORITY_LEVEL_EventHandler
from pydynaa.EventHandler cimport HIGHEST_PRIORITY_LEVEL as _HIGHEST_PRIORITY_LEVEL_EventHandler
from pydynaa.EventHandler cimport LOWEST_PRIORITY_LEVEL as _LOWEST_PRIORITY_LEVEL_EventHandler
from pydynaa.ExpressionHandler cimport ExpressionHandler as _ExpressionHandler
from pydynaa.EventExpression cimport EventExpression as _EventExpression
from pydynaa.EventExpression cimport EventExpressionType as _EventExpressionType
from pydynaa.HandlerRegistry cimport RegisteredHandler as _RegisteredHandler
from pydynaa.HandlerRegistry cimport ExpressionHandlerSet as _ExpressionHandlerSet
from pydynaa.SimulationEngine cimport SimulationEngine as _SimulationEngine
from pydynaa.SimulationEngine cimport getInstance as _getInstance_SimulationEngine
# Import Cython class definitions:
from pydynaa.core cimport Event
from pydynaa.core cimport Entity
from pydynaa.core cimport EventType
from pydynaa.core cimport EventHandler
from pydynaa.core cimport EventExpression
from pydynaa.core cimport SimulationEngine

# Record deprecation warnings so that we only give them once
_deprecation_warnings = {}


#cdef extern from "<cmath>" namespace "std" nogil:
#    bool isnan(double arg) nogil except+


# TODO move to a separate util module?
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


cdef public void _call_obj(obj, _Event ev) except * with gil:
    """Invokes a python callable object (functor) 

    Invokes the callable python object 'obj' with the given event.
    This function is used to call python functions as event handlers (callbacks).
    
    Parameters
    ----------
    obj : a python callable object 
        A function that receives a DynAA event as parameter is expected.
    ev : a DynAA event (_Event)
        The event that triggered this event handler execution
    
    Notes
    -----
        Event is an immutable type in cDynAA and passed here as a wrapped copy.
    
    """
    sig_check()
    try:
        obj(Event.__wrap__(ev))
    except Exception as e:
        _getInstance_SimulationEngine().stop()
        raise e


cdef public void _call_obj_expr(obj, shared_ptr[_EventExpression] evexpr) except * with gil:
    """Invokes a python callable object (functor) 

    Invokes the callable python object 'obj' with the given event expression.
    This function is used to call python functions as expression handlers (callbacks).
    
    Parameters
    ----------
    obj : a python callable object 
        A function that receives a DynAA event expression as parameter is expected.
    evexpr : a shared pointer DynAA event expression (shared_ptr<_EventExpression>)
        The event expression that triggered this event handler execution
    
    """
    cdef EventExpression py_evexpr
    sig_check()
    try:
        py_evexpr = EventExpression.__new__(EventExpression)
        py_evexpr.__inst = evexpr
        obj(py_evexpr)
    except Exception as e:
        _getInstance_SimulationEngine().stop()
        raise e


def __static_any_type_Event():
    """Wraps static Event::ANY_TYPE parameter.

    Returns
    -------
    py_result : :py:class:`~pydynaa.core.EventType`
        A python EventType wrapping a cDynAA Event::ANY_TYPE event type.

    """
    return EventType.__wrap__(_ANY_TYPE_Event) 


def __static_any_source_Event():
    """Wraps static Event::ANY_SOURCE parameter.

    Returns
    -------
    py_result : :py:class:`~pydynaa.core.Entity`
        A python :py:class:`~pydynaa.core.Entity` wrapping a cDynAA entity of type ANY_SOURCE.
    
    """
    return None


def __static_any_id_Event():
    """Wraps static Event::ANY_ID parameter.

    Returns
    -------
    py_result : int
        A long int representing an Event::ANY_ID event type.

    """
    cdef long int _r = _ANY_ID_Event
    return _r


cdef class Event:
    """An instance of time in the simulation with significance to a simulation entity.

    Notes
    -----
        Events are created by the simulation core and never directly by the user.  
        For this reason, there is no initialization of an event. Events from the 
        core can only be _wrapped_ for being accessed from python.

    """

    any_type = __static_any_type_Event()
    any_source = __static_any_source_Event()
    any_id = __static_any_id_Event()

    def __init__(self, *args, **kwargs):
        # Event has no public constructors
        if not kwargs.get("__createUnsafeObject__") is True:
            raise Exception("Cannot call this constructor")

    @staticmethod
    cdef __wrap__(const _Event& event):
        # Wraps the simulation event with a python object
        cdef Event py_result = Event.__new__(Event)
        py_result.__inst = make_shared[_Event](event)
        return py_result

    @staticmethod
    cdef __wrap_ptr__(shared_ptr[_Event] event_ptr):
        # Wraps the pointer to a simulation event with a python object
        cdef Event py_result = Event.__new__(Event)
        py_result.__inst = event_ptr
        return py_result

    cdef shared_ptr[_Event] inst(self):
        return self.__inst

    @property
    def id(self):
        """int: a unique identifier for the event during a simulation run.
    
        Each event is uniquely identified by a long integer number in the simulation.
        This property can be used to retrieve this id.

        """
        cdef long int _r = self.__inst.get().id()
        return _r

    @property
    def source(self):
        """:py:class:`~pydynaa.core.Entity`: the source entity that issued this event.

        Each event is issued by one particular source.  Sources of events are always entities.
        This property can be used to retrieve the source of this event.

        Raises
        ------
        LookupError
            If the source entity no longer exists i.e. has been garbage collected.
        
        """
        cdef shared_ptr[_Entity] entity
        cdef object source
        try:
            entity = self.__inst.get().source()
        except RuntimeError:
            raise LookupError("Source Entity has been removed from Simulator")
        source = <object> (<_CyEntity*>(entity.get())).pyobj()
        if source is None:
            raise LookupError("Source Entity has been removed from Simulator")
        return source
    
    @property
    def type(self):
        """:py:class:~pydynaa.core.EventType: the type of this event.

        This property can be used to retrieve the type of this event.

        Notes
        -----
            This method returns a reference to an existing ``EventType`` instance (hopefully),
            and therefore memory should not be freed when the returned object is deleted.
            The call to ``__new__`` bypasses ``__init__`` so that ``__is_memory_allocated``
            is ``False``.

        """
        return EventType.__wrap_ptr__(self.__inst.get().typeAsPtr())

    def unschedule(self):
        """Unschedule this event from the simulation.

        This event is removed from the simulation timeline, so that it is not 
        issued anymore.

        Notes
        -----
            Unscheduling events that do are lon longer in the timeline, or that were 
            already issued in the past yields no effect -- no error (so no exception 
            or error returned).

        """
        _getInstance_SimulationEngine().unschedule(deref(self.__inst.get()))

    def __richcmp__(self, other, op):
        """Provides comparison operators for events.

        An event is equal to other event if it has the same type, id, and source.

        An event is less than another event if it has lower id, or (if equal) its source has a lower id,
        or (if equal) its event type has a lower id; in that order.

        Returns
        -------
        bool
            Truth value of comparison.

        """ 
        if not isinstance(other, Event):
            return False
        cdef Event other_casted = other
        cdef Event self_casted = self
        if op == 0:
            return deref(self_casted.__inst.get()) < deref(other_casted.__inst.get())
        elif op == 1:
            return (deref(self_casted.__inst.get()) < deref(other_casted.__inst.get()) or
                    deref(self_casted.__inst.get()) == deref(other_casted.__inst.get()))
        elif op == 2:
            return deref(self_casted.__inst.get()) == deref(other_casted.__inst.get())
        elif op == 3:
            return deref(self_casted.__inst.get()) != deref(other_casted.__inst.get())
        elif op == 4:
            return deref(other_casted.__inst.get()) < deref(self_casted.__inst.get())
        elif op == 5:
            return (deref(other_casted.__inst.get()) < deref(self_casted.__inst.get()) or
                    deref(self_casted.__inst.get()) == deref(other_casted.__inst.get()))

    def __str__(self):
        return "ev_{}".format(self.id)

    def __hash__(self):
        return hash((self.id, self.type.id, self.source.uid))


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
    def __init__(self, str name, str description):
        if not isinstance(name, str):
            raise TypeError('arg name wrong type')
        if not isinstance(description, str):
            raise TypeError('arg description wrong type')
        cdef libcpp_string _name = name.encode()
        cdef libcpp_string _description = description.encode()
        self.__inst = make_shared[_EventType](_name, _description)

    def __dealloc__(self):
        # Deallocates the cDynAA object wrapped in here
        self.__inst.reset()

    @staticmethod
    cdef __wrap__(const _EventType& event_type):
        # Wraps a copy of the given cDynAA event type.
        # Notice that the given event_type itself is NOT wrapped, but a copy of it. 
        # Such approach is slightly less efficient, but safer in terms of memory management 
        # in the interface between pydynaa and cDynaa.
        cdef EventType py_result = EventType.__new__(EventType)
        py_result.__inst = make_shared[_EventType](event_type)
        return py_result

    @staticmethod
    cdef __wrap_ptr__(shared_ptr[_EventType] event_type_ptr):
        # Wraps the shared pointer to a cDynAA event type
        cdef EventType py_result = EventType.__new__(EventType)
        py_result.__inst = event_type_ptr
        return py_result

    cdef shared_ptr[_EventType] inst(self):
        return self.__inst

    @property
    def name(self):
        """str: the name of this event type."""
        cdef libcpp_string _r = self.__inst.get().name()
        return _r.decode()

    @property
    def description(self):
        """str: a short description of this event type."""
        cdef libcpp_string _r = self.__inst.get().description()
        return _r.decode()
   
    @property
    def id(self):
        """int: a unique identifier for the event type."""
        cdef long int _r = self.__inst.get().id()
        return _r

    def __richcmp__(self, other, op):
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
        if not isinstance(other, EventType):
            return False
        cdef EventType other_casted = other
        cdef EventType self_casted = self
        if op==2:
            return deref(self_casted.__inst.get()) == deref(other_casted.__inst.get())
        if op==3:
            return deref(self_casted.__inst.get()) != deref(other_casted.__inst.get())
    
    def __str__(self):
        return "{}".format(self.name)

    def __hash__(self):
        return hash(self.id)


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
        :py:class:`Event`
            The scheduled event.

        Raises
        ------
        TypeError
            If ``event_type`` is not an :py:class:`~pydynaa.core.EventType`.

        """
        if not isinstance(event_type, EventType):
            raise TypeError("arg event_type wrong type")
        cdef shared_ptr[_Event] _event_ptr = self.__inst.get().scheduleNow(event_type.inst())
        py_result = Event.__wrap_ptr__(_event_ptr)
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
        :py:class:`~pydynaa.core.Event`
            The scheduled event.

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
        cdef _EventType* _evtype = event_type.inst().get()
        cdef shared_ptr[_Event] _event_ptr = self.__inst.get().scheduleAfter(<_SimTime>interval, event_type.inst())       
        py_result = Event.__wrap_ptr__(_event_ptr)
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
        :py:class:`Event`
            The scheduled event.

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
        cdef _EventType* _evtype = event_type.inst().get()
        cdef shared_ptr[_Event] _event_ptr = self.__inst.get().scheduleAt(<_SimTime>instant, event_type.inst())
        py_result = Event.__wrap_ptr__(_event_ptr)
        return py_result

    cpdef _wait_once(self, handler, Entity entity=None, EventType event_type=None, Event event=None,
                     long event_id=Event.any_id, EventExpression expression=None):
        """Waits once for a certain event to be triggered.
        
        Waits exactly once until a matching event is triggered.  
        Waiting for events requires declaring a matching (filter) criteria composed of a source, an event type, an event, or and event id; or a combination of those.
        
        Parameters
        ----------
        handler : :py:class:`~pydynaa.core.EventHandler` or :py:class:`~pydynaa.core.ExpressionHandler`
            The handler to be invoked when a triggered event or expression matches the given filter.
        entity : :py:class:`~pydynaa.core.Entity` or None, optional
            Only events from this entity will match the filter.
        event_type: :py:class:`~pydynaa.core.EventType` or None, optional
            Only events of the given event_type will match the filter.
        event : :py:class:`~pydynaa.core.Event` or None, optional
            Only the specific event will match the filter.
        event_id : long, optional
            Only the specific event (with given event id) will match the filter.
        expression : :py:class:`~pydynaa.core.ExpressionHandler` or None, optional
            Event expression to wait on. If specified (not None) it overrides the other event related
            parameters (``entity``, ``event_type``, ``event``, ``event_id``).

        Raises
        ------
        TypeError
            If passed parameters are not of the expected type.

        Notes
        -----
            If a source is provided, only events coming from this source (entity) will match the filter and trigger the given event handler.        
            If an event type is provided, only events of this given event type with match the filter and triffer the given event handler.
            If an event is provided, the filter expect until the specific given event is triggered.
            If and event_id is provided, only the event with the given event id matches the filter and triggers the given event handler.
            Combinations of those are possible, for example, if a source and an event type is provided, only events from this source and of the given event type will match the filter and trigger the event handler.

            Only the first event matching the filter will cause an invocation of th event handler.  The following matches will not invoke the handler anymore.

        """
        cdef EventHandler eventHandler
        cdef ExpressionHandler exprHandler
        if entity is None:
            entity = Event.any_source
        elif not isinstance(entity, Entity):
                raise TypeError('arg entity wrong type')
        if event_type is None:
            event_type = Event.any_type
        elif not isinstance(event_type, EventType):
                raise TypeError('arg event_type wrong type')
        if event is not None:
            if not isinstance(event, Event):
                raise TypeError('arg event wrong type')
            event_id = event.id  # override
        if event_id is None:
            event_id = Event.any_id
        elif not isinstance(event_id, int):
            raise TypeError('arg event_id wrong type')
        if isinstance(handler, EventHandler):
            eventHandler = handler
            if event is not None:
                self.__inst.get().waitOnce(eventHandler.__inst, deref(event.inst().get()))
            elif entity is not None:
                self.__inst.get().waitOnce(eventHandler.__inst, <_EventSource> entity.__inst,
                                           deref(event_type.inst().get()), (<_EventID>event_id))
            else:
                self.__inst.get().waitOnce(eventHandler.__inst, deref(event_type.inst().get()), (<_EventID>event_id))
        elif isinstance(handler, ExpressionHandler):
            exprHandler = handler
            if expression is None:
                expression = EventExpression(entity, event_type, event_id)
            self.__inst.get().waitOnce(exprHandler.__inst, expression.__inst)
        else:
            raise TypeError('arg handler wrong type')

    cpdef _wait(self, handler, Entity entity=None, EventType event_type=None, Event event=None,
                long event_id=Event.any_id, bool once=False, EventExpression expression=None):
        """Waits continuously for certain events to be triggered.
        
        Waits until a matching event is triggered. This wait remains until it is explicitly dismissed. 
        Waiting for events requires declaring a matching (filter) criteria composed of a source, an event type, an event, or and event id; or a combination of those.
        
        Parameters
        ----------
        handler : :py:class:`~pydynaa.core.EventHandler` or :py:class:`~pydynaa.core.ExpressionHandler`
            The event or expression handler to be invoked when a triggered event matches the given filter.
        entity : :py:class:`~pydynaa.core.Entity` or None, optional
            Only events from this entity will match the filter.
        event_type: :py:class:`~pydynaa.core.EventType` or None, optional
            Only events of the given event_type will match the filter.
        event : :py:class:`~pydynaa.core.Event` or None, optional
            Only the specific event will match the filter.
        event_id : long, optional
            Only the specific event (with given event id) will match the filter.
            Event expression to wait on. If specified (not None) it overrides the other event related
            parameters (``entity``, ``event_type``, ``event``, ``event_id``).
        once : bool, optional
            Whether to only wait once.
        expression : :py:class:`~pydynaa.core.ExpressionHandler` or None, optional
            Event expression to wait on. If specified (not None) it overrides the other event related
            parameters (``entity``, ``event_type``, ``event``, ``event_id``).

        Raises
        ------
        TypeError
            If passed parameters are not of the expected type.

        Notes
        -----
            If a source is provided, only events coming from this source (entity) will match the filter and trigger the given event handler.        
            If an event type is provided, only events of this given event type with match the filter and triffer the given event handler.
            If an event is provided, the filter expect until the specific given event is triggered and then the handler is dismissed -- as if _waitOnce is called.
            If an event is provided, all other parameters (beside the event handler) are ignored.
            If and event_id is provided, only the event with the given event id matches the filter and triggers the given event handler.
            Combinations of those are possible, for example, if a source and an event type is provided, only events from this source and of the given event type will match the filter and trigger the event handler.

            All future events matching the filter will cause an invocation of th event handler.  To stop this behaviour, the event handler has to be dismissed.

        """
        if once or event is not None:
            self._wait_once(handler, entity=entity, event_type=event_type, event=event,
                            event_id=event_id, expression=expression)
            return
        cdef EventHandler eventHandler
        cdef ExpressionHandler exprHandler
        if entity is None:
            entity = Event.any_source
        elif not isinstance(entity, Entity):
                raise TypeError('arg entity wrong type')
        if event_type is None:
            event_type = Event.any_type
        elif not isinstance(event_type, EventType):
                raise TypeError('arg event_type wrong type')
        if event_id is None:
            event_id = Event.any_id
        if not isinstance(event_id, int):
            raise TypeError('arg event_id wrong type')
        if isinstance(handler, EventHandler):
            eventHandler = handler
            if entity is not None:
                self.__inst.get().wait(eventHandler.__inst, <_EventSource> entity.__inst,
                                           deref(event_type.inst().get()), (<_EventID>event_id))
            else:
                self.__inst.get().wait(eventHandler.__inst, deref(event_type.inst().get()), (<_EventID>event_id))
        elif isinstance(handler, ExpressionHandler):
            exprHandler = handler
            if expression is None:
                expression = EventExpression(entity, event_type, event_id)
            self.__inst.get().wait(exprHandler.__inst, expression.__inst)
        else:
            raise TypeError('arg handler wrong type')

    cpdef _dismiss(self, handler, Entity entity=None, EventType event_type=None,
                   Event event=None, long event_id=Event.any_id, EventExpression expression=None):
        """Dismiss an event handler waiting for certain events to be triggered.

        Parameters
        ----------
        handler : :py:class:`~pydynaa.core.EventHandler` or :py:class:`~pydynaa.core.ExpressionHandler`
            The event or expression handler to be dismissed.
        entity : :py:class:`~pydynaa.core.Entity` or None, optional
            Only events from this entity will match the filter.
        event_type: :py:class:`~pydynaa.core.EventType` or None, optional
            Only events of the given event_type will match the filter.
        event : :py:class:`~pydynaa.core.Event` or None, optional
            Only the specific event will match the filter.
        event_id : int, optional
            Only the specific event (with given event id) will match the filter.
        expression : :py:class:`~pydynaa.core.ExpressionHandler` or None, optional
            Event expression to wait on. If specified (not None) it overrides the other event related
            parameters (``entity``, ``event_type``, ``event``, ``event_id``).

        Notes
        -----
            If an event is passed as parameter, its information overrides all other parameters.
            Other parameters are ignored.

        Raises
        ------
        TypeError
            If passed parameters are not of the expected type.

        """
        cdef EventHandler eventHandler
        cdef ExpressionHandler exprHandler
        if entity is None:
            entity = Event.any_source
        elif not isinstance(entity, Entity):
                raise TypeError('arg entity wrong type')
        if event_type is None:
            event_type = Event.any_type
        elif not isinstance(event_type, EventType):
                raise TypeError('arg event_type wrong type')
        if event_id is None:
            event_id = Event.any_id
        if not isinstance(event_id, int):
            raise TypeError('arg event_id wrong type')
        if event is not None and not isinstance(event, Event):
            raise TypeError('arg event wrong type')
        if isinstance(handler, EventHandler):
            eventHandler = handler
            if event is not None:
                self.__inst.get().dismiss(eventHandler.__inst, deref(event.inst().get()))
            elif entity is not None:
                self.__inst.get().dismiss(eventHandler.__inst, <_EventSource> entity.__inst,
                                           deref(event_type.inst().get()), (<_EventID>event_id))
            else:
                self.__inst.get().dismiss(eventHandler.__inst, deref(event_type.inst().get()), (<_EventID>event_id))
        elif isinstance(handler, ExpressionHandler):
            exprHandler = handler
            if expression is not None:
                self.__inst.get().dismiss(exprHandler.__inst, expression.__inst)
            else:
                self.__inst.get().dismiss(exprHandler.__inst)
        else:
            raise TypeError('arg handler wrong type')

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
   
    An EventHandler must be specified to the :py:class:`~pydynaa.core.Entity`
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

        def callbackA(event):
            print("Source UID".format(event.source.uid))
        
        def callbackB(time):
            print("Event occured at {}".format(time))

        class CB(Entity):
            def callbackAsMethod(event):
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
        add_to_ptr_vector(vec, entity.inst())

    #def __cinit__(self, callback_function, str identifier="", list safe_guards=[], int priority=5, *args, **kwargs):
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

    def handle(self, Event ev):
        """Handle an event.
        
        Parameters
        ----------
        ev : :py:class:`~pydynaa.core.Event`
            Event to handle.

        """
        self.__inst.get().handle(deref(ev.inst().get()))

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


cdef class ExpressionHandler:
    """Handles an event expression for an Entity using a callback function.
   
    An ExpressionHandler must be specified to the :py:class:`~pydynaa.core.Entity`
    *wait()* and *wait_once()* methods.

    Parameters
    ----------
    callback_function : function(EventExpression) or None
        Callback function taking event expression as argument, or None.
    identifier : str, optional
        Identifier for expression handler.
    safe_guards: list of :py:class:`~pydynaa.core.Entity`, optional
        A list of Entities to be used as safe guards of this expression handler.
    priority : int
        Priority of this expression handler. 
  
    Attributes
    ----------
    default_priority : int
        Default priority level for expression handlers.
    highest_priority : int
        The highest priority level for expression handlers.
    lowest_priority : int
        The lowest priority level for expression handlers.

    Notes
    -----
        See also the notes for :class:`~pydynaa.core.EventHandler`.

        The ExpressionHandler class uses the ``__cinit__`` method to construct a new C++ _ExpressionHandler
        C++ object on the heap. Because ``__cinit__`` is always called once, ExpressionHandler can be
        safely subclassed. However, this python class should no be used to wrap
        existing C++ _ExpressionHandler objects using ``__new__``, as the creation of a new C++ object
        cannot be avoided.

    """
    default_priority = _DEFAULT_PRIORITY_LEVEL_EventHandler
    highest_priority = _HIGHEST_PRIORITY_LEVEL_EventHandler
    lowest_priority = _LOWEST_PRIORITY_LEVEL_EventHandler

    def __cinit__(self, callback_function=None, identifier="", list safe_guards=[],
                  int priority=_DEFAULT_PRIORITY_LEVEL_EventHandler):
        # Note that cinit constructor is always called.
        cdef _PyObjWrapper f
        cdef libcpp_string id_cppstr = identifier.encode()
        cdef vector[weak_ptr[_Entity]] _safe_guards
        if callback_function:
            f = _PyObjWrapper(callback_function)
            for entity in safe_guards:
                EventHandler.__add_to_safe_guards__(_safe_guards, entity)
            self.__inst = make_shared[_ExpressionHandler](f, id_cppstr, _safe_guards, priority)
        else:
            self.__inst = make_shared[_ExpressionHandler](id_cppstr, _safe_guards, priority)

    def __dealloc__(self):
        self.__inst.reset()

    cdef shared_ptr[_ExpressionHandler] inst(self):
        return self.__inst

    def handle(self, eventExpr):
        """Handle an event or event expression.
        
        Parameters
        ----------
        eventExpr : :py:class:`~pydynaa.core.EventExpression` or :py:class:`~pydynaa.core.Event`
            Event expression or event to handle with callback function.

        """
        cdef EventExpression _evExpr
        cdef Event _event
        if isinstance(eventExpr, EventExpression):
            _evExpr = eventExpr
            self.__inst.get().handle(_evExpr.__inst)
        elif isinstance(eventExpr, Event):
            _event = eventExpr
            self.__inst.get().handle(deref(_event.inst().get()))
        else:
            raise TypeError("{} is not a valid EventExpression or Event".format())

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
        If :py:class:`~pydynaa.core.Event.any_source` or ``None`` then
        any source is matched.
    event_type : :py:class:`~pydynaa.core.EventType` or None, optional
        The EventType that should be matched in the event expression condition.  
        If :py:class:`~pydynaa.core.Event.any_type` or ``None`` then
        any event type is matched.
    event_id: int, optional
        The EventID to be matched in the event expression condition.  
        If :py:class:`~pydynaa.core.Event.any_id` or ``-1`` then
        any event is matched.

    Notes
    -----   
        The basic event expression -- also called event condition or atomic event expression --
        is a a single condition (source, type, id) that has to be matched. In each part of the condition, 
        wildcards
        :py:class:`~pydynaa.core.Event.any_source`,
        :py:class:`~pydynaa.core.Event.any_type`, or
        :py:class:`~pydynaa.core.Event.any_id` 
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

        def warning_callback(evexpr):
            condition = triggered_expr.triggered_events[-1].type.description
            print("Be careful: it's wet outside ({}).".format(condition))
        
        warning_handler = ExpressionHandler(warning_callback)

        # Wait on event expression using an expression handler:
        weather_service._wait(warning_handler, expression=evexpr_wetweather)

        # Schedule snow and run simulation:
        gaia._schedule_now(evtype_snow)
        SimulationEngine().run()
        
    Event expressions can also be used directly at truth values i.e. in boolean expressions
    there is no calling their ``value`` attribute. For example:

    .. code-block:: python

        if not evexpr_wetweather:
            print("It's okay to venture outside.")

    """
    ATOMIC = _EventExpressionType.Atomic
    AND = _EventExpressionType.AND
    OR = _EventExpressionType.OR

    def __init__(self, Entity source=None, EventType event_type=None, int event_id=Event.any_id):
        if source is None:
            # NOTE that this currently sets source to None anyway
            source = Event.any_source
        if event_type is None:
            event_type = Event.any_type 
        if event_id is None:
            event_id = Event.any_id
        if source is None:
            self.__inst = make_shared[_EventExpression](<_EventSource> _ANY_SOURCE_Event, deref(event_type.inst().get()), (<_EventID>event_id))
        else:    
            self.__inst = make_shared[_EventExpression](<_EventSource> source.__inst, deref(event_type.inst().get()), (<_EventID>event_id))
    
    cdef shared_ptr[_EventExpression] inst(self):
        return self.__inst

    @property
    def value(self):
        """bool: the current logical value of this expression in the simulation."""
        return self.__inst.get().value()

    def _triggered_event(self):
        # Get triggering event
        cdef _Event * ev_ptr = self.__inst.get().triggeredEvent().get()
        if ev_ptr == NULL:
            return None
        else:
            return Event.__wrap__(deref(ev_ptr))

    @property
    def atomic_id(self):
        """int or None:
        The event ID of the atomic expression or None if not an atomic event expression."""
        cdef long int c_evid
        if self.type == self.ATOMIC:
            return self.__inst.get().atomicEventID()
        else:
            return None

    @property
    def atomic_type(self):
        """:py:class:`~pydynaa.core.EventType` or None:
        The event type of the atomic expression or None if not an atomic event expression."""
        if self.type == self.ATOMIC:
            return EventType.__wrap__(self.__inst.get().atomicEventType())
        else:
            return None

    @property
    def atomic_source(self):
        """:py:class:`~pydynaa.core.Entity` or None:
        The source entity of the atomic expression or None if not an atomic event expression."""
        cdef shared_ptr[_Entity] c_source
        cdef object py_source
        if self.type == self.ATOMIC:
            try:
                c_source = self.__inst.get().atomicEventSource().lock()
            except RuntimeError:
                raise LookupError("Source Entity has been removed from Simulator")
            if c_source == _ANY_SOURCE_Event:
                py_source = None
            else:
                # Assume Entity created py pydynaa ... 
                py_source = <object> (<_CyEntity*> c_source.get()).pyobj()
                if py_source is None:
                    raise LookupError("Source Entity has been removed from Simulator")
            return py_source
        else:
            return None

    @property
    def triggered_events(self):
        """The events that caused this expression to trigger.

        The events are sorted by the order in which they occured.
        If this is an atomic expression then the list will contain
        maximally one event.
        
        Returns
        -------
        list of :class:`~pydynaa.core.Event`
            Triggering events sorted by occurance, or an empty list
            if the expression has not yet been triggered.

        """
        cdef _Event * ev_ptr
        if self.type == self.ATOMIC:
            ev_ptr = self.__inst.get().triggeredEvent().get()
            if ev_ptr == NULL:
                return []
            else:
                return [Event.__wrap__(deref(ev_ptr))]
        cdef list events = []
        events.extend(self.first_term.triggered_events)
        events.extend(self.second_term.triggered_events)
        events.sort()
        return events

    @property
    def triggered_time(self):
        """The time at which this event expression triggered, if applicable.

        Returns
        -------
        float or None

        """
        cdef double _triggered_time = self.__inst.get().triggeredTime()
        if isnan(_triggered_time):
            return None
        else:
            return _triggered_time

    @property
    def type(self):
        """{self.ATOMIC, self.AND, self.OR}: type of event expression."""
        return self.__inst.get().type()

    @property
    def first_term(self):
        """:py:class:`~pydynaa.core.EventExpression`: first term in this expression."""
        cdef EventExpression py_result = EventExpression.__new__(EventExpression)
        py_result.__inst = self.__inst.get().firstTermAsPtr()
        return py_result

    @property
    def second_term(self):
        """:py:class:`~pydynaa.core.EventExpression`: second term in this expression."""
        cdef EventExpression py_result = EventExpression.__new__(EventExpression)
        py_result.__inst = self.__inst.get().secondTermAsPtr()
        return py_result

    def reprime(self):
        """Reset the event expression by setting all its atomic expressions
        to not triggered.

        """
        self.__inst.get().reprime()

    def __and__(EventExpression self, EventExpression other):
        if other is None:
            return self.copy()
        elif self is None:
            return other.copy()
        cdef EventExpression py_result = EventExpression.__new__(EventExpression)
        py_result.__inst = self.__inst.get().logicalAnd(other.__inst) 
        return py_result

    def __or__(EventExpression self, EventExpression other):
        if other is None:
            return self.copy()
        elif self is None:
            return other.copy()
        cdef EventExpression py_result = EventExpression.__new__(EventExpression)
        py_result.__inst = self.__inst.get().logicalOr(other.__inst)
        return py_result 

    def __nonzero__(EventExpression self):
        # Used for truth value testing and built-in operations bool()
        return self.__inst.get().value()

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
        if op==2:
            return deref(self.__inst.get()) == deref(other.__inst.get())
        if op==3:
            return deref(self.__inst.get()) != deref(other.__inst.get())

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
        cdef EventExpression py_result = EventExpression.__new__(EventExpression)
        py_result.__inst = make_shared[_EventExpression](deref(self.__inst.get()))
        return py_result

    # Deprecated

    @property
    def triggered_event(self):
        # DEPRECATED
        _warn_deprecated("triggered_event() method is deprecated. Use triggered_events() method instead.",
                         key="EventExpression.triggered_event")
        return self._triggered_event()

    def wait(EventExpression self, callback=None, int priority=_DEFAULT_PRIORITY_LEVEL_EventHandler):
        # DEPRECATED
        _warn_deprecated("EventExpression.wait() is deprecated. Use Entity.wait(...) or Entity.wait_once() "
                         " with parameters of type ExpressionHandler and EventExpression instead.",
                         key="EventExpression.wait")
        def expr_callback(evExpr):
            if callback:
                callback()

        # Dismiss any existing handlers registered for this expression
        for exprhandler, __ in SimulationEngine().get_expression_handlers(self):
            _dummy_entity._dismiss(exprhandler, expression=self)
        # Wait with a new expression handler
        cdef ExpressionHandler py_exprhandler = ExpressionHandler(expr_callback, priority=priority)
        _dummy_entity.__inst.get().waitOnce(py_exprhandler.__inst, self.__inst)


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
        self.__inst = _PointerHolder[_SimulationEngine](&_getInstance_SimulationEngine())

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

    @property
    def num_events(self):
        """int: number of events currently on the simulation timeline."""
        cdef unordered_map[libcpp_string, long] info = self.__inst.get().getDiagnosticInfo()
        cdef long _r = info["num_current_events"]
        #cdef int _r = self.__inst.get().getNumberOfEvents()
        return _r

    @property
    def num_instants(self):
        """int: number of time instants currently on the simulation timeline."""
        cdef unordered_map[libcpp_string, long] info = self.__inst.get().getDiagnosticInfo()
        cdef long _r = info["num_current_instants"]
        #cdef int _r = self.__inst.get().getNumberOfInstants()
        return _r

    @property
    def num_handlers(self):
        """int: number of event handlers currently registered in the simulation."""
        cdef unordered_map[libcpp_string, long] info = self.__inst.get().getDiagnosticInfo()
        cdef long _r = info["num_reg_handlers"]
        #cdef int _r = self.__inst.get().getNumberOfHandlers()
        return _r

    def get_diagnostic_info(self):
        """dict: diagnostic info."""
        cdef unordered_map[libcpp_string, long] info = self.__inst.get().getDiagnosticInfo()
        cdef unordered_map[libcpp_string, long].iterator it = info.begin()
        cdef dict infodict = {}
        while(it != info.end()):
            infodict[deref(it).first.decode()] = deref(it).second
            incr(it)
        return infodict

    def get_expression_handlers(self, EventExpression expression):
        """Retrieve list of registered expression handlers for an event expression.

        Parameters
        ----------
        expression : :py:class:`~pydynaa.core.EventExpression`
            Event expression to inspect for registered expression handlers.

        Returns
        -------
        list of (:py:class:`~pydynaa.core.ExpressionHandler`, bool):
            List of tuples of expression handler and a boolean indicating if expression
            is single shot (`once`).
        
        """
        cdef list result = []
        cdef ExpressionHandler py_exprhandler
        cdef shared_ptr[_ExpressionHandler] sp_exprhandler
        cdef _ExpressionHandlerSet handler_set = (
            self.__inst.get().getHandlerInspector().getExpressionHandlersOf(expression.__inst))
        cdef set[_RegisteredHandler[_ExpressionHandler]].iterator iter = handler_set.begin()
        while iter != handler_set.end():
            py_exprhandler = ExpressionHandler.__new__(ExpressionHandler)
            sp_exprhandler = deref(iter).handler
            py_exprhandler.__inst = sp_exprhandler
            result.append((py_exprhandler, deref(iter).once))
            incr(iter)
        return result


cdef class SimState:
    Idle = 0
    Running = 1
    Stopped = 2 


def _cleanup_on_module_exit():
    # Function called to clean up on module exit
    SimulationEngine().reset()


import atexit
atexit.register(_cleanup_on_module_exit)
