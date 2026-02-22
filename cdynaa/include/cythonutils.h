// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef _INCLUDED_PYDYNAA_WRAP_UTILS_
#define _INCLUDED_PYDYNAA_WRAP_UTILS_
/* Set of static functions that helps on converting cDynAA objects into
   pyDynAA and back.
 */

//#include <Python.h>
#include <algorithm>
#include <memory>
#include <vector>
#include "dynaatypes.h"
#include "event.h"
#include "eventexpression.h"
#include "handlerreport.h"

/**
 * Functions imported in pydynaa
 */
namespace dynaa {

/**
 * Helper function to implement && operator in cython
 */
EventExpression logicalAnd(const EventExpression &first, const EventExpression &second);

/**
 * Helper function to implement ||  operator in cython
 */
EventExpression logicalOr(const EventExpression &first, const EventExpression &second);

/**
 * Offers an efficient way to calculate the hash of and EventExpression from
 * pydynaa
 */
unsigned long hashit(const EventExpression&);


}

#endif
