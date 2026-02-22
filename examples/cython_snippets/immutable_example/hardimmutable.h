#ifndef _HARDIMMUTABLE_TEST_
#define _HARDIMMUTABLE_TEST_

#include <string>

namespace imtest {

// Forward declaration
class User;

class HardImmutable {

    friend User;
    
private:

    // Used to generate a sequence of unique identifiers for this immutable.
    static long m_idSequenceNumber;

    // Hard immutable means m_id is declared const, which means
    // the implicit assignment operator is deleted
    const long m_id;
    const std::string m_name;
    
    // Constructor
    HardImmutable(const std::string name) : m_id(HardImmutable::m_idSequenceNumber++), m_name(name) {};

public:

    // no default constructor

    // copy constructor
    HardImmutable(const HardImmutable & immutable) : m_id(immutable.m_id), m_name(immutable.m_name) {};

    long id() const {
        return m_id;
    };

    std::string name() const {
        return m_name;
    };
};

}

#endif
