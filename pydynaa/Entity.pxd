# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
"""
C++ class declaration for Entity
"""
from libcpp.string cimport string as libcpp_string
from libcpp cimport bool

cdef extern from "<entity.h>" namespace "dynaa":

    cdef cppclass Entity:
        
        Entity() nogil except +
        Entity(Entity) nogil except +
        long getUID() nogil except +
        bool operator== (Entity& other) nogil except +
        bool operator!= (Entity& other) nogil except +


cdef extern from "<entity.h>" namespace "dynaa::Entity":

    cdef libcpp_string ENTITY_PREFIX
