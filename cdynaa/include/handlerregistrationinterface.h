// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_HANDLERREGISTRATIONINTERFACE_
#define INCLUDED_HANDLERREGISTRATIONINTERFACE_

#include "eventhandler.h"
#include "eventtype.h"
#include "handlertoken.h"
#include <memory>

namespace dynaa {

/**
 * HandlerRegistryInterface
 *
 *
 */
class HandlerRegistrationInterface {

public:
    /**
     * Unregister an event handler from observing events from a certain entity,
     * and of a certain type, and with a certain id.
     *
     * @param eventHandler
     *            the event handler to be unregistered
     * @param entity
     *            the source of events (can be {@link #ANY_ENTITY})
     * @param eventType
     *            the type of events (can be {@link #ANY_TYPE})
     * @param id
     *            the id of the event (can be {@link #ANY_ID})
     */
    virtual void unregisterHandler(const std::shared_ptr<EventHandler> &eventHandler,
                                   const EventSource &entity,
                                   const EventType &eventType,
                                   long id) = 0;

    /**
     * Unregister an event handler from observing events.
     *
     * The handler is referenced by its registration token that uniquely
     * identifies this registration record..
     *
     * @param token
     *            the event handler token produced during the registration of the event handler.
     */
    virtual void unregisterHandler(const HandlerToken& token) = 0;

    /**
     * Register an expression handler as an observer for event expressions
     *
     * If the expression handler has a higher priority than existing expression handlers
     * waiting on this expression, the higher priority is adoped by the atomic expression
     * handlers of the event expression. The handlers themselves will still be called
     * in the order of their priority.
     *
     * If a registered event expression is triggered, all of the handlers registered to it have their
     * callbacks executed in order of priority, and are passed the triggered event expression.
     * Single shot handlers are removed from the set, and if any continuous handlers (once=false) remain,
     * the event expression state is reset and allowed to trigger again.
     *
     * @param expressionHandler
     *            the expression handler to be registered, in order to have it run
     *            any for an event expression
     * @param evExptr
     *            the event expression to listen to
     * @param once
     *            whether to only wait once for the expression to trigger.
     *
     */
    virtual HandlerToken registerHandler(const std::shared_ptr<EventHandler>& eventHandler,
                                         const EventExpression& evExpr, bool once) = 0;

    /**
     * Unregister an event handler from observing events from a certain entity,
     * and of a certain type, and with a certain id.
     *
     * @param eventHandler
     *            the expression handler to be unregistered
     * @param evExptr
     *            the event expression to unregister the handler for
     *
     */
    virtual void unregisterHandler(const std::shared_ptr<EventHandler>& eventHandler,
                                   const EventExpression& evExpr) = 0;

    /**
     * Unregister an event handler from observing any event (expression) at all.
     * All previous registrations of the given event handler are unregistered.
     *
     * @param eventHandler
     *            the expression handler to be unregistered
     *
     */
    virtual void unregisterHandler(const std::shared_ptr<EventHandler>& eventHandler) = 0;

    /**
     * Cleans the registry from invalid handlers
     */
    virtual void clean() = 0;

    /**
     * Constructors, virtual destructor and assignment operator.
     */
    HandlerRegistrationInterface() = default;
    HandlerRegistrationInterface(const HandlerRegistrationInterface&) = default;
    HandlerRegistrationInterface(HandlerRegistrationInterface&&) = default;
    virtual ~HandlerRegistrationInterface() = default;
    HandlerRegistrationInterface& operator=(const HandlerRegistrationInterface&) = default;
    HandlerRegistrationInterface& operator=(HandlerRegistrationInterface&&) noexcept = default;
};
}

#endif
