# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
"""
C++ class declaration for EventExpression
"""
from libcpp.string cimport string as libcpp_string
from libcpp cimport bool
from libcpp.memory cimport shared_ptr
from libcpp.memory cimport unique_ptr
from libcpp.vector cimport vector

from pydynaa.Event cimport Event as _Event
from pydynaa.Event cimport EventID as _EventID
from pydynaa.Event cimport EventSource as _EventSource
from pydynaa.EventType cimport EventType as _EventType
from pydynaa.Entity cimport Entity as _Entity
from pydynaa.PyObjWrapper cimport PyObjWrapper as _PyObjWrapper


cdef extern from "eventexpression.h" namespace "dynaa": 

    ctypedef enum EventExpressionType :
        Atomic, AND, OR

    cdef cppclass EventExpression:

        EventExpression() nogil except +
        EventExpression(_EventSource, _EventType, _EventID) nogil except +
        EventExpression(libcpp_string, _EventSource, _EventType, _EventID) nogil except +
        EventExpression(EventExpression) nogil except +
        EventExpressionType type() nogil except +
        EventExpression & firstTerm() nogil except +
        EventExpression & secondTerm() nogil except +
        bool operator== (EventExpression & second) nogil except +
        bool operator!= (EventExpression & second) nogil except +
        vector[EventExpression] atoms() nogil except +

        _EventType & atomicEventType() nogil except +
        _EventSource atomicEventSource() nogil except +
        _EventID atomicEventID() nogil except +
        libcpp_string descriptor() nogil except +

