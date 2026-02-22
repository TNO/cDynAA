# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
"""
C++ class declaration for DefaultHandlerRegistry.
"""
from libcpp.memory cimport shared_ptr
from libcpp.set cimport set
from libcpp cimport bool

from pydynaa.Event cimport Event as _Event
from pydynaa.EventExpression cimport EventExpression as _EventExpression
from pydynaa.EventHandler cimport EventHandler as _EventHandler
from pydynaa.HandlerToken cimport HandlerToken as _HandlerToken

cdef extern from "handlerqueryinterface.h" namespace "dynaa":

    # cdef cppclass RegisteredHandler[T]:
    #     RegisteredHandler() nogil except +
    #     RegisteredHandler(shared_ptr[T] the_handler, bool once=true) nogil except +
    #     shared_ptr[T] handler
    #     bool once
    #     #bool operator< (const RegisteredHandler[T]& other) const

    # ctypedef set[RegisteredHandler[_EventHandler]] HandlerSet
    ctypedef set[_HandlerToken] HandlerTokenSet

    cdef cppclass HandlerQueryInterface:
        #HandlerQueryInterface() nogil except +
        #HandlerQueryInterface(HandlerQueryInterface) nogil except +
        #HandlerSet getEventHandlersOf(const _Event & event) nogil except +
        #HandlerTokenSet getExpressionHandlersOf(shared_ptr[_EventExpression]& evExpr) nogil except +
        long int getNumberOfHandlers() nogil except +
