# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
from pydynaa.Entity cimport Entity as _Entity

cdef extern from "<dynaatypes.h>" namespace "dynaa":

    ctypedef double SimTime
    ctypedef long SourceUID

