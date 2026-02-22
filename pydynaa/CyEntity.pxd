# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
"""
C++ class declaration for CyEntity.
"""
from cpython.ref cimport PyObject
from libcpp.string cimport string as libcpp_string
from libcpp.memory cimport shared_ptr as libcpp_shared_ptr
from libcpp.memory cimport weak_ptr as libcpp_weak_ptr
from libcpp cimport bool
from libcpp.vector cimport vector

from pydynaa.Event cimport Event as _Event
from pydynaa.Event cimport EventID as _EventID
from pydynaa.Event cimport EventSource
from pydynaa.EventType cimport EventType as _EventType
from pydynaa.Entity cimport Entity as _Entity
from pydynaa.EventHandler cimport EventHandler as _EventHandler
from pydynaa.EventExpression cimport EventExpression as _EventExpression
from pydynaa.HandlerToken cimport HandlerToken as _HandlerToken
from pydynaa.dynaatypes cimport SimTime as _SimTime

cdef extern from "cyentity.h" namespace "dynaa":

    cdef cppclass CyEntity(_Entity):
        
        CyEntity() nogil except +
        CyEntity(CyEntity) nogil except +
        CyEntity(PyObject* pyEntity) nogil except +

        PyObject* pyobj() except + with gil

        _EventExpression scheduleNow(const _EventType & evType) nogil except +
        _EventExpression scheduleAfter(_SimTime interval, const _EventType & evType) nogil except +
        _EventExpression scheduleAt(_SimTime instant, const _EventType & evType) nogil except +


        # unschedule()
        int unschedule(_EventType & eventType, _EventID eventID) nogil except +

        # wait()
        _HandlerToken wait(libcpp_shared_ptr[_EventHandler] eventHandler, EventSource entity,
                  _EventType & eventType, _EventID eventID) nogil except +
        _HandlerToken wait(libcpp_shared_ptr[_EventHandler] eventHandler,
                  _EventType & eventType, _EventID eventID) nogil except +
        _HandlerToken wait(libcpp_shared_ptr[_EventHandler] exprHandler,
                   const _EventExpression& eventExpr) nogil except +

        # waitOnce()
        _HandlerToken waitOnce(libcpp_shared_ptr[_EventHandler] eventHandler, EventSource entity,
                      _EventType & eventType, _EventID eventID) nogil except +
        _HandlerToken waitOnce(libcpp_shared_ptr[_EventHandler] eventHandler,
                      _EventType & eventType, _EventID eventID) nogil except +
        _HandlerToken waitOnce(libcpp_shared_ptr[_EventHandler] exprHandler,
                       const _EventExpression& eventExpr) nogil except +
        
        # dismiss()
        void dismiss(_HandlerToken &handlerToken) nogil except +
        


    cdef void add_to_ptr_vector(vector[libcpp_weak_ptr[_Entity]] &ptr_vector, libcpp_shared_ptr[CyEntity]) nogil except +
