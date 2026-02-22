#include "../include/cythonutils.h"
#include "../include/eventexpression.h"

/* This file contains several helpers to facilitate cython wrapping. */

namespace dynaa {

EventExpression logicalAnd(const EventExpression& first, const EventExpression& second) {
    return (first && second);
}


EventExpression logicalOr(const EventExpression& first, const EventExpression& second) {
    return (first || second);
}

unsigned long hashit(const EventExpression& ee) {
    EventExpressionHash eeHash;
    return eeHash(ee);
};

}
