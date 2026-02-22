// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_EVENTMASK_
#define INCLUDED_EVENTMASK_

#include "event.h"

namespace dynaa {

/**
 * EventMask is an structure type used as key in the mappings between event observers and events in the registry.
 *
 * An event handler is triggered when an event in the system matches its event mask.
 *
 * Developer documentation:
 *
 * The implementation of EventMask is done to be a straightforward data struct of 3 longs.
 * The intention is to keep it extremely simple for performance. Typical operations are
 * comparison, hashing, and matches (cover).
 *
 * In this implementation the value 0xFFFFFFFFFFFF for a field is used as a wildcard symbol,
 * instead of an absolute field value.  Whereas this value is used, it will match (or cover)
 * any other absolute value.
 *
 * The struct declaration is maintained in this (separate) file to avoid circular dependencies
 * between the registry and other internal used classes (e.g. HandlerRecord)
 *
 * This is an internal type relevant only for the DefaultHandlerRegistry implementation.
 * It should not be exported nor used in the registry interface.
 *
 */
struct EventMask {
    long sourceID;
    long typeID;
    EventID eventID;

    static const long defaultIDMask = 0xFFFFFFFFFFFFFFFF;

    /**
     * Equality operator for an event mask
     *
     * An event mask is considered to be equal, when all its fields(sourceID, typeID, and eventID)
     * are equal
     */
    inline bool operator==(const EventMask& other) const {
        return (sourceID == other.sourceID && typeID == other.typeID && eventID == other.eventID);
    }

    inline bool operator<(const EventMask &other) const  {
        if (sourceID < other.sourceID)  return true;
        if (sourceID == other.sourceID && typeID < other.typeID) return true;
        return (sourceID == other.sourceID && typeID == other.typeID && eventID < other.eventID);
    }

    /**
     * Checks if the event mask matches another masked event.
     *
     * An event mask matches its masked event if each field of the mask (sourceID, typeID, eventID)
     * matches its equivalent field in the masked event.
     *
     * @param mask the mask against which the match is tested
     * @param masked the event mask to be tested against the mask
     *
     * @return true is the event mask matches the masked event
     */
    inline static bool matches(const EventMask &mask,
                               const EventMask &masked) {
        return matches(mask.sourceID, masked.sourceID) &&
               matches(mask.typeID, masked.typeID) &&
               matches(mask.eventID, masked.eventID);
    };

    /**
     * Checks if a mask matches the masked field
     *
     * A mask matches the masked field if :
     *
     *  - the mask field is default (0xFFFFFFFFFFFFFFFF); or
     *
     *  - the mask field is equal to the masked field;
     *
     * @param mask the mask field to be checked against
     * @param masked the field to check against the mask
     *
     * @return  true if fields match, false otherwise.
     */
    inline static bool matches(const long& mask, const long& masked) {
        return (mask == defaultIDMask) ? true : mask == masked;
    };

    /**
     * Function to generate the a mask from a source, type, id triplet
     *
     * @param source
     *            the Entity id that is the source of the event
     * @param type
     *            the EventType of the event
     * @param id
     *            the unique identifier of the event
     * @return a String which is a unique identifier for this event handler
     *
     * TODO: Check if instead of a source ID, we can pass the source.  Fix dependency problems.
     */
    inline static EventMask generateMask(const long sourceID, const EventType& type, const EventID id) {
        const long asourceID = (sourceID == Event::ANY_SOURCE_ID) ? defaultIDMask : sourceID;
        const long typeID = (type == Event::ANY_TYPE) ? defaultIDMask : type.id();
        const long eventID = (id == Event::ANY_ID) ? defaultIDMask : id;
        return {asourceID, typeID, eventID};
    };

    /**
     * Hash operator for this event mask. Hashing is important for using event masks as keys in the
     * registry
     */
    inline size_t operator()(const EventMask& k) const {
        return (((k.sourceID) ^ ((k.typeID) << 13)) >> 7) ^ ((k.eventID) << 3);
    };
};
};

/**
 * DISCUSSIONS AND DECISIONS
 *
 * This struct was an internal type of the DefaultHandlerRegistry before.
 * As it evolved, we extracted it to avoid circular dependencies.
 *
 * I made eventmask.h a header file only.  The idea is to have a highly
 * efficient structure and "inline" most of its usage to avoid function call overhead.
 *
 */

#endif
