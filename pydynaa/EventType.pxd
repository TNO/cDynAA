# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
from libcpp.string cimport string as libcpp_string
from libcpp cimport bool


cdef extern from "eventtype.h" namespace "dynaa":
    
    cdef cppclass EventType:
       
        EventType() nogil except +
        EventType(EventType) nogil except +
        EventType(libcpp_string name, libcpp_string description) nogil except +
        
        libcpp_string name() nogil except +
        libcpp_string description() nogil except +
        long id() nogil except +
        
        bool operator== (EventType & other) nogil except +
        bool operator!= (EventType & other) nogil except +
