# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)


cdef extern from "<py_obj_wrapper.h>" namespace "dynaa":
    
    # note this doesn't match a c++ signature, which is fine
    cdef cppclass PyObjWrapper:

        PyObjWrapper() with gil
        PyObjWrapper(object)  with gil  # define a constructor that takes a Python object
