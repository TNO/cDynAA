#NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
"""
C++ class declaration for Diagnostics.
"""
from libcpp.memory cimport shared_ptr
from libcpp.map cimport map as libcpp_map
from libcpp.vector cimport vector as libcpp_vector
from libcpp.pair cimport pair as libcpp_pair
from libcpp cimport bool
from libcpp.string cimport string as libcpp_string

from pydynaa.dynaatypes cimport SimTime as _SimTime
from pydynaa.dynaatypes cimport SourceUID as _SourceUID
from pydynaa.EventExpression cimport EventExpression as _EventExpression
from pydynaa.HandlerToken cimport HandlerToken as _HandlerToken
from pydynaa.HandlerReport cimport HandlerReport as _HandlerReport

cdef extern from "diagnostics.h" namespace "dynaa":

    ctypedef libcpp_vector[ libcpp_pair [ _SimTime, _EventExpression ] ] DiagnosticsEventTimeline
    ctypedef libcpp_vector[ libcpp_pair [ _SimTime, shared_ptr[ _HandlerReport ] ] ] DiagnosticsHandlerTimeline

    cdef cppclass Diagnostics:
        Diagnostics() nogil except +
        Diagnostics(Diagnostics) nogil except +
# Functions used by User
        DiagnosticsEventTimeline getPastEvents() nogil except +
        DiagnosticsEventTimeline getPastEventsLastRun() nogil except +
        DiagnosticsEventTimeline getPastEvents(int) nogil except +
        libcpp_map[libcpp_string,double] getPastStatistics() nogil except +
        libcpp_vector[_HandlerToken] getPastHandlers(_SimTime) nogil except +
        DiagnosticsHandlerTimeline getPastHandlerReports(_HandlerToken) nogil except +
        DiagnosticsEventTimeline getFutureEvents() nogil except +
        libcpp_map[libcpp_string,double] getFutureStatistics() nogil except +
        _SimTime getTimeOfMostRecentEvent() nogil except +
        _SimTime getTimeOfNextEvent() nogil except +
        DiagnosticsEventTimeline getNextEvents(int) nogil except +
        DiagnosticsEventTimeline getEventsAtNextInstant() nogil except +
        void activateDiagnosticsStorage() nogil except +
        void deactivateDiagnosticsStorage() nogil except +
        bool isDiagnosticsStorageActive() nogil except +
        void reset() nogil except +
        Diagnostics & getInstance() nogil except +

cdef extern from "diagnostics.h" namespace "dynaa::Diagnostics":

    Diagnostics & getInstance() nogil except +
