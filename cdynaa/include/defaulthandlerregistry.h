// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_DEFAULTHANDLERREGISTRY_
#define INCLUDED_DEFAULTHANDLERREGISTRY_

#include "event.h"
#include "eventtype.h"
#include "eventhandler.h"
#include "eventexpression.h"
#include "handlerregistrationinterface.h"
#include "handlerqueryinterface.h"
#include "eventmask.h"
#include "handlerrecord.h"
#include <memory>
#include <unordered_map>
#include <set>

namespace dynaa {

/**
 * Record tag is a light weight wrapper for a HandlerRecord pointer
 *
 * Weak pointers are not suitable to be used directly in sets.  This light
 * weight wrapper allows us to put pointers to HandlerRecords in a set
 * structure in the registry.
 *
 * @see MaskToRecordMap
 */
struct RecordTag : public HandlerToken  {
    const std::weak_ptr<HandlerRecord> record;

    RecordTag(const HandlerToken &token, const std::weak_ptr<HandlerRecord>& record) :
        HandlerToken(token), record(record) {}
    RecordTag(const RecordTag& tag) :
        HandlerToken(tag), record(tag.record) {};
};

typedef std::set<HandlerToken> HandlerTokenSet;

/**
 * DefaultHandlerRegistry
 */
class DefaultHandlerRegistry : public HandlerRegistrationInterface, public HandlerQueryInterface {

private:

    /**
     * A map to hold handler records
     *
     * It is an unordered_map because we make a lot of operations by key.
     */
    std::unordered_map<HandlerToken, std::shared_ptr<HandlerRecord>, HandlerToken > handlerRecordDB;

    /**
     * A map to hold the relation between records and triggering masks
     * It is an unordered_map because we make a lot of operations by key.
     */
    std::unordered_map<const EventMask, std::set<RecordTag>, EventMask> maskToRecordDB;

    /**
     * Keeps track of the number of invalid records for cleaning purposes
     */
    long m_invalid_records;

public:
    /**
     * The constructor of the DefaultHanlderRegistry sets up the handler maps
     */
    DefaultHandlerRegistry() : handlerRecordDB(), maskToRecordDB(), m_invalid_records(0) {};

    /*
     * @see
     * HandlerQueryInterface#getEventHandlersOf
     */
    HandlerTokenSet getEventHandlersOf(const Event &event) override;

    /*
     * @see
     * HandlerQueryInterface#getNumberOfHandlers
     */
    long int getNumberOfHandlers() const override;

    /*
     * @see
     * HandlerRegistryInterface#unregisterHandler
     */
    void unregisterHandler(const std::shared_ptr<EventHandler>& eventHandler,
                           const EventSource &entity,
                           const EventType &eventType,
                           const EventID id) override;

    /*
     * @see
     * HandlerRegistryInterface#unregisterHandler
     */
    void unregisterHandler(const HandlerToken &token);

    /*
     * @see
     * HandlerRegistryInterface#registerHandler
     */
    HandlerToken registerHandler(const std::shared_ptr<EventHandler>& eventHandler,
                                 const EventExpression& evExpr, bool once) override;

    /*
     * @see
     * HandlerRegistryInterface#unregisterHandler
     */
    void unregisterHandler(const std::shared_ptr<EventHandler>& eventHandler,
                           const EventExpression& evExpr) override;


    /*
     * @see
     * HandlerRegistryInterface#unregisterHandler
     */
    void unregisterHandler(const std::shared_ptr<EventHandler>& eventHandler) override;

    /**
     * Resets this registry.
     *
     * Deletes all entries in the internal registry and empties it.
     *
     */
    void reset();

    /**
     * Clean the registry data structures
     *
     * The registry cleans the invalid data structures in bulk for efficiency.
     */
    void clean() override;

    /**
     * Returns true if this registry needs cleaning
     *
     * Usually related to a condition on the number of invalid records in the registry
     */
    bool needs_cleaning() const;

    /**
     * Safely activates an event handler
     */
    void activateHandler(const HandlerToken& token, const Event& event);

    /**
     * Safely updates a handler with information about a (triggered) event
     */
    bool updateHandler(const HandlerToken& token, const Event& event, const SimTime updateTime);

private:
    /**
     * Concatenate a Set of EventHandlers to include a (new) Set of handlers to
     * be fetched from the handlerMap
     *
     * @param observers
     *            the already existing Set of EventHandlers. May include
     *            EventHandlers already, and these will stay in it.
     * @param source
     *            the triggering entity source of the handlers to add
     * @param type
     *            the triggering event type of the handlers to add
     * @param id
     *            the triggering event id of the handlers to add
     * @return a Set of EventHandlers which is a copy of the first input
     *         argument, with new handlers added
     */
    void appendHandlersTo(HandlerTokenSet * const handlers, const EventMask& mask);

};
}

/**
 * DISCUSSIONS AND DECISIONS
 *
 * Design of DefaultHandlerRegistry
 *
 * 1) The maskToRecordMap
 *
 * a. I considered first using a set of weak_ptr<HandlerRecord> as values in the maskToRecordMap.
 *    Note however that using weak_ptr in a set is a ill definition, as it is not possible to make a consistent
 *    <(less than) comparison. weak_ptr may change in time.
 * b. Also not sufficient to use directly a Pair<int, weak_ptr<HandlerRecord>> because default (lexicographic) comparisons
 *    in a pair are synthesized three-way comparisons (see https://en.cppreference.com/w/cpp/utility/pair/operator_cmp).
 *    Overriding the pair comparison system is more complex than defining own structure.
 * c. So, we boil then here to use sets of specific structs in the maskToRecordMap
 */


#endif
