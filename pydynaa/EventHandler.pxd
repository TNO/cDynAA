# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
"""
C++ class declaration for EventHandler.
"""
from libcpp.string cimport string as libcpp_string
from libcpp.memory cimport shared_ptr
from libcpp.memory cimport weak_ptr
from libcpp cimport bool
from libcpp.vector cimport vector

from pydynaa.HandlerReport cimport HandlerReport as _HandlerReport
from pydynaa.Entity cimport Entity as _Entity
from pydynaa.PyObjWrapper cimport PyObjWrapper as _PyObjWrapper

cdef extern from "eventhandler.h" namespace "dynaa":

    cdef cppclass EventHandler:

        EventHandler() nogil except +
        EventHandler(EventHandler) nogil except +
        EventHandler(_PyObjWrapper) nogil except +
        EventHandler(_PyObjWrapper, int priority) nogil except +
        EventHandler(_PyObjWrapper, libcpp_string identifier) nogil except +
        EventHandler(_PyObjWrapper, libcpp_string identifier, int priority) nogil except +
        EventHandler(_PyObjWrapper, libcpp_string identifer, vector[weak_ptr[_Entity]] safe_guards) except +
        EventHandler(_PyObjWrapper, libcpp_string identifer, vector[weak_ptr[_Entity]] safe_guards, int priority) except +
        EventHandler(_PyObjWrapper, vector[weak_ptr[_Entity]] safe_guards) except +
        EventHandler(_PyObjWrapper, vector[weak_ptr[_Entity]] safe_guards, int priority) except +

        #Note: Should not use nogil.  Eventually, python functions can be called.
        void handle(_HandlerReport handler_report) except +

        libcpp_string getIdentifier() nogil except +

        int getPriorityLevel() const


cdef extern from "<eventhandler.h>" namespace "dynaa::EventHandler":

    cdef const int DEFAULT_PRIORITY_LEVEL
    cdef const int HIGHEST_PRIORITY_LEVEL
    cdef const int LOWEST_PRIORITY_LEVEL

    # here we lie about the signature because C++ does an automatic conversion
    # to function pointer for classes that define operator(), but Cython doesn't know that
    # Note: nogil can be used as no python code is invoked in _create_
    shared_ptr[EventHandler] create(_PyObjWrapper) nogil except +
    shared_ptr[EventHandler] create(_PyObjWrapper, int priority) nogil except +
    shared_ptr[EventHandler] create(_PyObjWrapper, libcpp_string identifier) nogil except +
    shared_ptr[EventHandler] create(_PyObjWrapper, libcpp_string identifier, int priority) nogil except +
    shared_ptr[EventHandler] create(_PyObjWrapper, vector[weak_ptr[_Entity]] safe_guards) nogil except +
    shared_ptr[EventHandler] create(_PyObjWrapper, vector[weak_ptr[_Entity]] safe_guards, int priority) nogil except +
    shared_ptr[EventHandler] create(_PyObjWrapper, libcpp_string identifier, vector[weak_ptr[_Entity]] safe_guards) nogil except +
    shared_ptr[EventHandler] create(_PyObjWrapper, libcpp_string identifier, vector[weak_ptr[_Entity]] safe_guards, int priority) nogil except +
