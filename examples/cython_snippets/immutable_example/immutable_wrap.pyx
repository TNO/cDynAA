from libcpp.string  cimport string as libcpp_string
from libcpp.memory cimport shared_ptr
from cython.operator cimport dereference as deref


cdef extern from "softimmutable.h" namespace "imtest":
    cdef cppclass SoftImmutable:
        SoftImmutable() nogil except +
        SoftImmutable(SoftImmutable) nogil except +
        long int id() nogil except +
        libcpp_string name() nogil except +


cdef extern from "hardimmutable.h" namespace "imtest":
    cdef cppclass HardImmutable:
        HardImmutable() nogil except +
        HardImmutable(HardImmutable) nogil except +
        long int id() nogil except +
        libcpp_string name() nogil except +


cdef extern from "user.h" namespace "imtest":

    cdef cppclass User:
        
        User() nogil except +
        User(User) nogil except +
        
        SoftImmutable getSoftImmutable(libcpp_string name) nogil except +
        HardImmutable & getHardImmutable(libcpp_string & name) nogil except +
        shared_ptr[HardImmutable] getHardImmutableShared(libcpp_string & name) nogil except +


cdef class PySoftImmutable:

    cdef shared_ptr[SoftImmutable] inst

    def __dealloc__(self):
         self.inst.reset()
    
    def __init__(self, *args, **kwargs):
        if not kwargs.get("__createUnsafeObject__") is True:
            raise Exception("Cannot call this constructor")
    
    def id(self):
        cdef long int _r = self.inst.get().id()
        py_result = <long int>_r
        return py_result

    def name(self):
        cdef libcpp_string _r = self.inst.get().name()
        py_result = <libcpp_string>_r
        return py_result


cdef class PyHardImmutable:

    cdef shared_ptr[HardImmutable] inst

    @staticmethod
    cdef __create__(HardImmutable immutable):
        cdef shared_ptr[HardImmutable] copy = shared_ptr[HardImmutable](new HardImmutable(immutable))
        cdef PyHardImmutable py_result = PyHardImmutable.__new__(PyHardImmutable)
        py_result.inst = copy
        return py_result

    def __dealloc__(self):
         self.inst.reset()
    
    def __init__(self, *args, **kwargs):
        if not kwargs.get("__createUnsafeObject__") is True:
            raise Exception("Cannot call this constructor")
    
    def id(self):
        cdef long int _r = self.inst.get().id()
        py_result = <long int>_r
        return py_result

    def name(self):
        cdef libcpp_string _r = self.inst.get().name()
        py_result = <libcpp_string>_r
        return py_result


cdef class PyUser:

    cdef shared_ptr[User] inst

    def __dealloc__(self):
         self.inst.reset()
    
    def __init__(self, *args, **kwargs):
        pass

    def getSoftImmutable(self, bytes name):
        # create new Immutable using copy constructor
        cdef SoftImmutable * _r = new SoftImmutable(self.inst.get().getSoftImmutable((<libcpp_string>name)))
        cdef PySoftImmutable py_result = PySoftImmutable.__new__(PySoftImmutable)
        py_result.inst = shared_ptr[SoftImmutable](_r)
        return py_result

    def getHardImmutable(self, bytes name):
        # create new Immutable using copy constructor
        return PyHardImmutable.__create__(self.inst.get().getHardImmutable((<libcpp_string>name)))
        

        # Old way. Used is getHardImmutable returns a reference (or pointer)
        # cdef HardImmutable * _r = &self.inst.get().getHardImmutable((<libcpp_string>name))
        # cdef PyHardImmutable py_result = PyHardImmutable.__new__(PyHardImmutable)
        # py_result.inst = shared_ptr[HardImmutable](_r)
        # return py_result

    def getHardImmutableShared(self, bytes name):
        # create new Immutable using copy constructor
        cdef shared_ptr[HardImmutable] _r = self.inst.get().getHardImmutableShared((<libcpp_string>name))
        cdef PyHardImmutable py_result = PyHardImmutable.__new__(PyHardImmutable)
        py_result.inst = _r
        return py_result
