#ifndef _SOFTIMMUTABLE_TEST_
#define _SOFTIMMUTABLE_TEST_

#include <string>

namespace imtest {

// Forward declaration
class User;

class SoftImmutable {

    friend User;
    
private:

    // Used to generate a sequence of unique identifiers for this immutable.
    static long m_idSequenceNumber;

    // Soft immutable means no variables declared const
    long m_id;
    std::string m_name;

    // Constructor
    SoftImmutable(std::string name) : m_id(SoftImmutable::m_idSequenceNumber++), m_name(name) {};

public:

    // default constructor - ideally don't need to define this, alas we do
    SoftImmutable() : m_id(0), m_name("") {};

    // copy constructor
    SoftImmutable(const SoftImmutable & immutable) : m_id(immutable.m_id), m_name(immutable.m_name) {};

    long id() const {
        return m_id;
    };

    std::string name() const {
        return m_name;
    };
};

}

#endif
