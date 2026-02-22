# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
from libcpp.string cimport string as libcpp_string
from libcpp cimport bool


cdef extern from "handlertoken.h" namespace "dynaa":
    
    cdef cppclass HandlerToken:
       
        HandlerToken() nogil except +
        HandlerToken(HandlerToken) nogil except +
        
        libcpp_string name() nogil except +
        libcpp_string description() nogil except +
        long id() nogil except +
        
        bool operator== (HandlerToken & other) nogil except +
        bool operator!= (HandlerToken & other) nogil except +
        size_t operator()(const HandlerToken& token) nogil except +


