# NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
"""
C++ class declaration for cythonutils.
"""
from libcpp.memory cimport shared_ptr
from libcpp.vector cimport vector
from libcpp cimport bool

from pydynaa.dynaatypes cimport SimTime as _SimTime
from pydynaa.EventExpression cimport EventExpression as _EventExpression
from pydynaa.HandlerToken cimport HandlerToken as _HandlerToken
from pydynaa.HandlerReport cimport HandlerReport as _HandlerReport

cdef extern from "cythonutils.h" namespace "dynaa":

    _EventExpression logicalAnd(const _EventExpression &first, const _EventExpression &second)
    _EventExpression logicalOr(const _EventExpression &first, const _EventExpression &second)
    unsigned long hashit(const _EventExpression& event_expression)
