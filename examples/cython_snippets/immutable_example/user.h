#ifndef _USER_
#define _USER_

#include "softimmutable.h"
#include "hardimmutable.h"
#include <string>
#include <memory>

namespace imtest {

class User {
private:

public:
    // Default constructor
    User() {};

    SoftImmutable getSoftImmutable(std::string name) {
        // Because SoftImmutable has a default constructor and assignment operator,
        // Cython is happy to deal with it on the stack
        // In this case Python wrapper will call "new"
        return SoftImmutable(name);
    };
    
    HardImmutable getHardImmutable(const std::string name) {
        // Because HardImmutable has no default constructor and a deleted assignment operator,
        // Cython is not happy to deal with it on the stack
        // In this case Python wrapper will call wrap pointer into a shared_ptr
        // HardImmutable * hi = new HardImmutable(name);
        // return *hi;
        return HardImmutable(name);
    };

    std::shared_ptr<HardImmutable> getHardImmutableShared(const std::string name) {
        // Because HardImmutable has no default constructor and a deleted assignment operator,
        // Cython is not happy to deal with it on the stack
        // In this case Python wrapper will simply adopt this shared_ptr
        return std::make_shared<HardImmutable>(HardImmutable(name));
    };

};

}

#endif
