// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_EVENT_
#define INCLUDED_EVENT_
/* Definition of the Event class */

#include "eventtype.h"
#include "coreexceptions.h"
#include <iostream>
#include <memory>
#include <utility>

namespace dynaa {

// Forward declaration
class Entity;

typedef long EventID;

typedef std::weak_ptr<Entity> EventSource;

/**.
 * Core class for a simulation event
 *
 * An event is an observable occurrence within a simulation system.
 *
 * Events have a source, a type, and an ID. Source is an Entity that generates
 * the event occurrence. The source of an event is always an Entity object. The type
 * of an Event distinguishes between occurrences of different nature. Two
 * different types of occurrences. The type of an event is always an EventType.
 * The Event identifier uniquely identifies the occurrence related to the event.
 *
 * Together, the source, type, and identifier uniquely denotes an event in the
 * system.
 */
class Event {

    friend class Entity;

public:
    static const EventType ANY_TYPE;
    static const EventType INVALID_EVENT_TYPE;

    static std::shared_ptr<Entity> ANY_SOURCE;

    static const EventID ANY_ID;

    // CHECK: Used in event mask, but should be possible to extract directly from ANY_SOURCE.
    static const long ANY_SOURCE_ID;

private:
    /**
     * Used to generate a sequence of unique identifiers for this event.
     */
    static EventID m_idSequenceNumber;
    /**
     * A unique id (pseudo) for this event.
     */
    EventID m_id;
    /**
     * The source of this event
     */
    EventSource m_source;
    /**
     * The type of occurrence indicated by this event.
     */
    EventType m_type = INVALID_EVENT_TYPE;

    /**
    * Constructor
    *
    * Builds a new Event with the given EventType and source.
    *
    * This constructor is private, because events cannot be
    * created directly by the user.  By design, only Entity instances should be
    * able to create events.
    *
    * @param source
    *            the source of this event
    * @param type
    *            the type of this event
    */
    Event(EventSource source, EventType type) :
        m_id(Event::m_idSequenceNumber++), m_source(source), m_type(std::move(type)) {}

public:
    /**
     * Default constructors
     */
    Event() : m_id(Event::m_idSequenceNumber++) {};
    Event(const Event&)=default; // Copy constructor
    Event(Event&&)=default;      // Move constructor
    Event& operator=(const Event&)=default; //Copy assignment
    Event& operator=(Event&&)=default;      //Move assignment

    /**
     * Retrieves the id of this event
     *
     * @return the id of this event
     */
    EventID id() const;

    /**
     * Retrieves the source of this event
     *
     * @return the source of this event or InvalidEventSourceException in case the source is not accessible.
     */
    std::shared_ptr<Entity> source() const;

    /**
     * Retrieves the type of this event
     *
     * @return reference to object representing the type of this event.
     */
    const EventType& type() const;

    /**
    * Comparison operator (equality)
    *
    * @return true if events have the same id, source, and type, false otherwise
    */
    bool operator==(const Event &other) const;
    /**
    * Comparison operator (inequality)
    *
    * @return true if events are unequal (see operator==), false otherwise
    */
    bool operator!=(const Event &other) const;
    /**
    * Comparison operator (less than)
    *
    * @return true if this event has a lower id, or (if equal) its source has a lower id,
    * or (if equal) its event type has a lower id; in that order, otherwise false.
    * if its entity h
    */
    bool operator<(const Event &other) const;

    //FIX: Think I do not need this friendship anymore.  Getting the id is public, right?
    friend std::ostream& operator<<(std::ostream &strm, const Event &ev);

    /**
     * Small cheat to make testing for the diagnostics module more convenient
     * TODO: Is there a better solution for this?
     */
    static Event eventFactory(EventSource source, const EventType& type);

};

/**
* Overloads the << operator
*/
std::ostream& operator<<(std::ostream &strm, const Event &ev);
}

/**
 * DISCUSSIONS AND DECISIONS
 *
 * Design of Event ---------------
 *
 * 1) Event is immutable.
 *
 * The reason is that once an Event is produced it uniquely identifies an
 * occurrence in the simulation, which should not be modified in its type,
 * identification, or source. Modifications would mean changing one of those,
 * and thus changing the occurrence. Such thing should be represented by another
 * Event then.
 *
 * REMARK : In a strict way of speaking, Event is not purely immutable. This is
 * true because the source may change externally. However, no method in Event
 * uses the Entity interface. It is thus guaranteed that Event will always refer
 * to the same Entity.
 *
 * 2) The event id is not in the hand of the user.
 *
 * The reason is to avoid manipulation of events. The identifier is more of an
 * internal thing, which in certain sense should not even be visible to the
 * user. It simply disambiguates two events of the same type and triggered by
 * the same source. We keep the identifier of an event readable for purposes of
 * logging. Within the program, operations with events should avoid using id
 * directly. For example, for checking equality use "equals" function, instead
 * of comparing the id. Moreover, event identifiers are NOT the same from one
 * execution of the simulation to another !!!
 *
 * 3) Events are simple and ready to be passed by value
 *
 * Imagine the question:  Who is the owner of an event and takes care of its
 * lifetime in memory?  This is a hell to answer in C++ and in the context of
 * the DynAA simulator.  Probably, no good answer.
 * For that reason, events have value semantics.  That is, they have default copy
 * constructors, default move constructors, default copy assignment, etc...
 * in the Event class that allows to pass event by value in return values of function
 * or in input parameters.
 * It makes also life easier when storing events in std containers, which is most
 * the case for DynAA.
 *
 * 4) Sources of events are entities, which are stored in the event as a referring smart pointer (std::weak_ptr)
 *
 * A safe way to deal with lifetime issues of event sources (entities) is by storing entities
 * as smart pointers in the event, instead of a raw reference.  Smart pointers have methods
 * to validate the referred object before actioning on them.  An EventSource type is then defined
 * as a std::weak_ptr<Entity>.  The user, receives that std::weak_ptr when invoking the function get_source().
 * It may then get a temporary ownership on the event source by invoking the method lock() of the weak pointer.
 * See documentation of smart pointers (std::shared_ptr and std::weak_ptr) for more details.
 *
 * 5) Types of events are stored in the event
 *
 * EventType objects are very light weight and can be directly copied, moved, etc.. (value semantics)
 * Therefore no penalties in performance when copy/moving events AND no boilerplate code
 * for handling smartpointers.
 *
 * 6) Users do not have access to pure events anymore.  Events are core only objects.
 *    Instead, users receive always an atomic, fully qualified event expression representing exactly
 *    the unique event.  This expression can be used for reporting, dismissing, etc.
 */

#endif
