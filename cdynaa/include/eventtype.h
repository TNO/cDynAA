// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_EVENTTYPE_
#define INCLUDED_EVENTTYPE_
/* Definition of the EventType class
 */

#include <string>
#include <memory>
#include <algorithm>
#include <iostream>

namespace dynaa {
/**
 * Core class for a simulation event type
 *
 * An event type defines a certain sort of event.
 *
 * Events types are initialized with a name and descriptor, which are mainly
 * intended to be used for documentation.  On creation of an event type it also
 * receives automatically (not in users control) a type number (long).  This number is
 * unique for each event type and can be used for efficient identification of types.
 */
class EventType {

private:
    static long m_typeSequenceNumber;

    long m_id;

    std::shared_ptr<std::string> m_name;

    std::shared_ptr<std::string> m_description;

    static const std::string DEFAULT_EVENT_TYPE_NAME;
    static const std::string DEFAULT_EVENT_TYPE_DESCRIPTION;

public:
    /**
     * Default constructor
     *
     * Try to use the named (described) constructor.  It prints out better in the log.
     *
     * We provide a default constructor because giving a name or description is not obligatory.  Also, it allows to make things like:
     *
     * >> EventType firstType("example_type","Just an example type");
     * >> EventType myEventType;  // Creates a different type !!!!
     * >> assert(firstType != myEventType);
     *
     */
    EventType() : m_id(EventType::m_typeSequenceNumber++),
        m_name(std::make_shared<std::string>(DEFAULT_EVENT_TYPE_NAME)),
        m_description(std::make_shared<std::string>(DEFAULT_EVENT_TYPE_DESCRIPTION)) {};

    // Default constructors and special methods
    EventType(EventType&&)=default;
    EventType& operator=(const EventType&)=default;
    EventType& operator=(EventType&&)=default;
    EventType(const EventType&)=default;


    /**
     * Constructor
     *
     * Builds a new EventType with the given name and description.
     *
     * IMPORTANT:
     * The name and description of an EventType are just a helper, they do not define the type per se.
     * The type is defined by its id, which is acquired when creating a new type.
     * This way, for example, two types can have the same name and description and yet be of another type.
     *
     * As expected, copy constructor, copy assignment, and move operations preserve the id, thus, they do not create a new
     * event type, but just an object representing the same type.
     *
     * @param name
     *            the name of this event type
     * @param description
     *            the description of this event type
     */
    EventType(const std::string& name, const std::string& description) :
        m_id(EventType::m_typeSequenceNumber++),
        m_name(std::make_shared<std::string >(name)),
        m_description(std::make_shared<std::string >(description)) {};

    /**
     * Returns the name given to this event type
     *
     * @return the name given to this event type
     */
    std::string name() const;

    /**
     * Returns a textual description of this event type for human interpretation.
     *
     * @return a textual description of this event type
     */
    std::string description() const;

    /**
     * Returns the unique identifier given to this event type
     *
     * @return a unique identifier given to this event type
     */
    long id() const;

    /**
     * The equality operator for this event type
     */
    bool operator==(const EventType &other) const;

    /**
     * The inequality operator for this event type
     */
    bool operator!=(const EventType &other) const;

    /**
     * the streaming operator  for this event type
     */
    friend std::ostream& operator<<(std::ostream &strm, const EventType &ev);

};

/**
 * Overloads the << operator
 */
std::ostream& operator<<(std::ostream &strm, const EventType &ev);
}

/**
 * DISCUSSIONS AND DECISIONS
 *
 * Design of EventType
 *
 * 1) EventType is immutable.  But has a value semantics.
 *
 * Reason for that is that you can create and make copies of a certain event type in a very safe and efficient way.
 * Recommendation is that EventType objects are not create by new, but they are referred to or copied.
 * Note that what really defines an event type is its id, which cannot be determined by the user.
 * A user creates an EventType for representing a given sort of event in the simulation.  Creating an event
 * of this created type is always done by passing a reference or copy of the EventType, not creating a
 * new with the same name or description.
 *
 *
 * 2) Use short names and description.
 * Name and description of an event type are basically meant for printing user information in loggers or debug
 * cycles.  Thus, keep then short, meaninful, and expressive.
 *
 * 3) EventType does not store the name and description internally.
 * We use smart pointers to refer to stored names and descriptions.  So new memory is allocated for
 * name and description only during creation of new EventTypes.  When an EventType object is copied
 * around, the copies smartly share the same stored name and description.  Note that this storage
 * (memory) will be de-allocated only when all elements of the same EventType cease to exist.
 */

#endif
