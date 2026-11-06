# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)

cdef extern from "<pointer_tools.h>" namespace "dynaa":

    cdef cppclass PointerHolder [T]:

        PointerHolder()
        PointerHolder(T *)
        T * get()
        void assign(T *)
