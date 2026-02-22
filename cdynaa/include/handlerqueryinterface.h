// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_HANDLERQUERYINTERFACE_
#define INCLUDED_HANDLERQUERYINTERFACE_

#include "event.h"
#include "eventhandler.h"
#include "eventexpression.h"
#include <set>
#include <map>
#include <unordered_set>
#include <memory>

namespace dynaa {

/**
 * HandlerQueryInterface
 */
class HandlerQueryInterface {

public:
    /**
     * Get a set of all the event handlers of the given event. This would
     * include all handlers that implicitly should be called when they have
     * generic (i.e. when they listen to all events of a specific type)
     *
     * The returned set is ordered according to the handlers' priority levels, so that iterating the set
     * of HandlerTokens occurs from highest priority to lowest priority.
     *
     * @param event
     *            an Event of which all handlers are requested
     * @return a collection of EventHandlers
     */
    // TODO this method should be 'const', however, it currently also takes care of cleaning up
    // single-shot handlers. That responsibility should be moved to another (non-query) method
    virtual std::set<HandlerToken> getEventHandlersOf(const Event &event) = 0;

    /*
     * Retrieves the number of event handlers currently registered.
     *
     * @return an int specifying the number of event handlers registered.
     */
    virtual long int getNumberOfHandlers() const = 0;

    /**
     * Constructors, virtual destructor and assignment operator.
     */
    HandlerQueryInterface() = default;
    HandlerQueryInterface(const HandlerQueryInterface&) = default;
    HandlerQueryInterface(HandlerQueryInterface&&) = default;
    virtual ~HandlerQueryInterface() = default;
    HandlerQueryInterface& operator=(const HandlerQueryInterface&) = default;
    HandlerQueryInterface& operator=(HandlerQueryInterface&&) noexcept = default;

};
}

#endif
