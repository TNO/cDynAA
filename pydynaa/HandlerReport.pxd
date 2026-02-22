# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
"""
C++ class declaration for EventReport, which has been renamed HandlerReport in pydynaa.
"""
from libcpp.memory cimport shared_ptr
from libcpp.memory cimport make_shared
from libcpp.vector cimport vector
from libcpp cimport bool

from pydynaa.dynaatypes cimport SimTime as _SimTime
from pydynaa.EventExpression cimport EventExpression as _EventExpression
from pydynaa.HandlerToken cimport HandlerToken as _HandlerToken

cdef extern from "handlerreport.h" namespace "dynaa":
    
    cdef cppclass HandlerReport:

        struct EventReport:
            _EventExpression atom_ee
            bool value
            _EventExpression last_trigger
            _SimTime trigger_time

        _EventExpression getEventExpression() nogil except +
        _HandlerToken getHandlerToken() nogil except +
        int getHandlerPriority() nogil except +
        vector[HandlerReport.EventReport] getEventStateReport() nogil except+
        bool once() nogil except +
