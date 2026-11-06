import numpy as np
cimport numpy as np


cdef extern from "py_obj_wrapper.h":
    cdef cppclass PyObjWrapper:
        PyObjWrapper()
        PyObjWrapper(object) # define a constructor that takes a Python object
             # note - doesn't match c++ signature - that's fine!


cdef extern from "accepts_std_function.h":
    # NOTE to gil or nogil ?
    double call_some_std_func(PyObjWrapper) except +
            # here I lie about the signature
            # because C++ does an automatic conversion to function pointer
            # for classes that define operator(), but Cython doesn't know that


def example1(a, b):
    print(a,b)


def example2(a, b, c=3):
    print("{} world!".format(b.decode()))
    print("{} + {} = {}".format(a, c, a + c))


def example3(a, b):
    """Test that numpy works in a cython callback!"""
    row_dim = a
    col_dim = len(b)
    ndim = 2 if row_dim > 1 and col_dim > 1 else 1
    cdef np.ndarray[double, ndim=2] foo
    foo = np.zeros([row_dim, col_dim])
    foo = np.hstack((foo, np.eye(row_dim)))
    print(foo)


def test_call():
    cdef PyObjWrapper f1 = PyObjWrapper(example1)
    cdef PyObjWrapper f2 = PyObjWrapper(example2)
    cdef PyObjWrapper f3 = PyObjWrapper(example3)

    call_some_std_func(f1)
    call_some_std_func(f2)
    call_some_std_func(f3)


def call_func(pure_py_func):
    """Call function from python"""
    cdef PyObjWrapper f = PyObjWrapper(pure_py_func)
    foo = call_some_std_func(f)
