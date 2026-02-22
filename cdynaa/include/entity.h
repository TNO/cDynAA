// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_ENTITY_
#define INCLUDED_ENTITY_
/* Definition of the Entity class
 */

#include "dynaatypes.h"
#include "eventtype.h"
#include "event.h"
#include "eventhandler.h"
#include "scheduleruserinterface.h"
#include "simengine.h"
#include "eventexpression.h"
#include <string>
#include <iostream>
#include <memory>

namespace dynaa {
/**
 * Core class for a simulation entity
 *
 * This class designs the basic behavior of an entity in the Dynaa simulation
 * system.
 *
 */
class Entity : public std::enable_shared_from_this<Entity> {

private:
    /**
      * Seed for identifier sequences
      */
    static long m_idseed;

    /**
      * Entity unique identifier
      */
    const long m_uid;

public:
    /**
      * Prefix for entity print outs
      */
    static const std::string ENTITY_PREFIX;

    /**
       * Default constructor for Entity.
       */
    Entity() : m_uid(Entity::m_idseed++),
        hRegistry(SimulationEngine::getInstance().getHandlerRegistry()),
        scheduler(SimulationEngine::getInstance().getScheduler()) {};

    /**
     * Retrieves a unique ID for this Entity.
     *
     * @return an unique ID for this entity.
     */
    long getUID() const;

    /**
    * Comparison operator (equality)
    *
    * @return true if entities have the same unique id
    */
    bool operator==(const Entity &other) const;

    /**
    * Comparison operator (inequality)
    *
    * @return true if entities are unequal (see operator==), false otherwise
    */
    bool operator!=(const Entity &other) const;

    /**
     * Comparison operator (less than)
     *
     * @return true if this entity has the id smaller than other
     */
    bool operator<(const Entity &other) const;

    friend std::ostream &operator<<(std::ostream &strm, const Entity &entity);

protected:
    HandlerRegistrationInterface& hRegistry;
    SchedulerUserInterface& scheduler;

    /**
     * Dismisses event handler and event expression associated with the token.
     *
     * @param token
     *            the token access for the record that has to be dismissed.
     */
    void dismiss(const HandlerToken& token);

    //DESIGN_DISCUSSION: A dismiss API for the event handler can exist.  But what should be the semantics?
    //  This topic is under discussion. Meanwhile, the interface to dismiss based on the event handler (and
    //  a event expression) is  disabled.
    //  /**
    //   * Dismisses the given event handler from waiting on any event.
    //   *
    //   * @param eventHandler
    //   *            the eventHandler to be dismissed from waiting.
    //   */
    //  void dismiss(const std::shared_ptr<EventHandler> &eventHandler) ;

    //  /**
    //   * Dismisses the given event expression handler from waiting on the
    //   * indicated event expression.
    //   *
    //   * @param eventHandler
    //   *            the eventHandler to be dismissed from waiting
    //   * @param eventExpression
    //   *            event expression to which the handler will not be waiting
    //   */
    //  void dismiss(const std::shared_ptr<EventHandler>& eventHandler,
    //               const std::shared_ptr<EventExpression>& eventExpression);


    /**
     * Creates an event with the indicated type and with this entity as source,
     * and schedules it after the designated interval.
     *
     * @param interval
     *            time after which (in relation to current simulation time) the
     *            event will be scheduled.
     * @param type
     *            the type of the event to be generated.
     * @return an atomic expression representing the scheduled event.
     * @throws InvalidTimepointException
     *             if interval is negative.
     * @see EventExpression
     */
    EventExpression scheduleAfter(SimTime interval, const EventType& type);

    /**
     * Creates an event with the indicated type and with this entity as source,
     * and schedules it at the designated time point.
     *
     * Indicated time point must be after or equal to the current simulation
     * time.
     *
     * @param timepoint
     *            time at which the event will be scheduled. Must be larger or
     *            equal to current time.
     * @param type
     *            the type of the event to be generated.
     * @return an atomic expression representing the scheduled event.
     * @throws InvalidTimepointException
     *             if time point is before the simulation current time
     * @see EventExpression
     */
    EventExpression scheduleAt(SimTime instant, const EventType &type);

    /**
     * Creates an event with the indicated type and with this entity as source,
     * and schedules it at the current simulation time.
     *
     * @param type
     *            the type of the event to be generated.
     * @return an atomic expression representing the scheduled event.
     * @see EventExpression
     */
    EventExpression scheduleNow(const EventType &type);

    /**
     * Unschedules events currently scheduled by this entity.
     *
     * @param type
     *            the type of the event to be unscheduled.
     * @param id
     *            the id of the event to be unscheduled.
     * @return int
     *            the number of events that were unscheduled.
     */
    int unschedule(const EventType& type = Event::ANY_TYPE, const EventID id = Event::ANY_ID);

    /**
       * Makes the event handler an observer of any future event of the indicated
       * type and with the indicated id, independent of its source.
       *
       * @param eventHandler
       *            the observer to be triggered when a matching event occurs.
       * @param id
       *            the id of the event to be observed.
       * @param type
       *            the type of the event to be observed.
       * @throws InvalidEventException
       * @throws InvalidEventHandlerException
       */
    HandlerToken wait(const std::shared_ptr<EventHandler> &eventHandler, const EventType &type,
                      const EventID id = Event::ANY_ID) const;

    /**
       * Makes the event handler an observer of any future event of the indicated
       * type that comes from the given source, independent of its id.
       *
       * @param eventHandler
       *            the observer to be triggered when a matching event occurs.
       * @param source
       *            the source of the event to be observed.
       * @param type
       *            the type of the event to be observed.
       * @param id
       *            the id of the event to be observed.
       * @throws InvalidEventHandlerException
       *             If event handler is null.
       * @throws InvalidEventSourceException
       *             If source is null
       * @throws InvalidEventIdException
       *             If id is negative or zero.
       * @throws InvalidEventException
       *             If invalid parameter.
       */
    HandlerToken wait(const shared_ptr<EventHandler> &eventHandler, const EventSource &source = Event::ANY_SOURCE,
                      const EventType &type = Event::ANY_TYPE,
                      const long id = Event::ANY_ID) const;

    /**
       * Makes the event handler an observer of any future event of the indicated
       * id independent of its source or type.
       *
       * @param eventHandler
       *            the observer to be triggered when a matching event occurs.
       * @param id
       *            the id of the event to be observed
       *
       **/
    HandlerToken wait(const std::shared_ptr<EventHandler> &eventHandler, const EventID id) const;

    /**
       * Makes the event handler an observer of any future event from the
       * indicated source and with the indicated id, independent of its type.
       *
       * @param eventHandler
       *            the observer to be triggered when a matching event occurs.
       * @param source
       *            the source of the event to be observed.
       * @param id
       *            the id of the event to be observed.
       * @throws InvalidEventHandlerException
       *             If event handler is null.
       * @throws InvalidEventSourceException
       *             If source is null
       * @throws InvalidEventIdException
       *             If id is negative or zero.
       * @throws InvalidEventException
       *             If invalid parameter.
       *
       **/
    HandlerToken wait(const std::shared_ptr<EventHandler> &eventHandler, const EventSource &source, const EventID id) const;

    /**
       * Makes the expression handler an observer of any future triggering of
       * the event expression.
       *
       * After the expression triggers it will immediately be re-primed to trigger again.
       *
       * @param eventHandler
       *            the observer to be triggered when the event expression occurs.
       * @param eventExpression
       *            the event expression to observe.
       *
       */
    HandlerToken wait(const std::shared_ptr<EventHandler> &eventHandler, const EventExpression& evExpr) const;

    /**
       * Makes the event handler an observer of the next future event of the
       * indicated type and with the indicated id, independent of its source.
       *
       * Only the next matching event will trigger the event handler. After that,
       * the handler will be automatically dismissed.
       *
       * @param eventHandler
       *            the observer to be triggered when a matching event occurs.
       * @param id
       *            the id of the event to be observed.
       * @param type
       *            the type of the event to be observed.
       * @throws InvalidEventException
       * @throws InvalidEventHandlerException
       */
    HandlerToken waitOnce(const std::shared_ptr<EventHandler>& eventHandler, const EventType &type,
                          const EventID id = Event::ANY_ID) const;

    /**
       * Makes the event handler an observer of next future event of the indicated
       * id independent of its source or type.
       *
       * Only the next matching event will trigger the event handler. After that,
       * the handler will be automatically dismissed.
       *
       * @param eventHandler
       *            the observer to be triggered when a matching event occurs.
       * @param id
       *            the id of the event to be observed
       *
       **/
    HandlerToken waitOnce(const std::shared_ptr<EventHandler> &eventHandler, const EventID id) const;

    /**
       * Makes the event handler an observer of the next future event from the
       * indicated source and with the indicated id, independent of its type.
       *
       * Only the next matching event will trigger the event handler. After that,
       * the handler will be automatically dismissed.
       *
       * @param eventHandler
       *            the observer to be triggered when a matching event occurs.
       * @param source
       *            the source of the event to be observed.
       * @param id
       *            the id of the event to be observed.
       * @throws InvalidEventHandlerException
       *             If event handler is null.
       * @throws InvalidEventSourceException
       *             If source is null
       * @throws InvalidEventIdException
       *             If id is negative or zero.
       * @throws InvalidEventException
       *             If invalid parameter.
       *
       **/
    HandlerToken waitOnce(const std::shared_ptr<EventHandler> &eventHandler, const EventSource &source,
                          const EventID id) const;

    /**
       * Makes the event handler an observer of the next future event of the
       * indicated type that comes from the given source, and with the given id.
       *
       * Only the next matching event will trigger the event handler. After that,
       * the handler will be automatically dismissed.
       *
       * @param eventHandler
       *            the observer to be triggered when a matching event occurs.
       * @param source
       *            the source of the event to be observed. Defaults to Event::ANY_SOURCE
       * @param type
       *            the type of the event to be observed. Defaults to Event::ANY_TYPE
       * @param id
       *            the id of the event to be observed. Defaults to Event::ANY_ID
       * @throws InvalidEventHandlerException
       *             If event handler is null.
       * @throws InvalidEventSourceException
       *             If source is null
       * @throws InvalidEventIdException
       *             If id is negative or zero.
       * @throws InvalidEventException
       *             If invalid parameter.
       *
       */
    HandlerToken waitOnce(const shared_ptr<EventHandler> &eventHandler, const EventSource &entity = Event::ANY_SOURCE,
                          const EventType &eventType = Event::ANY_TYPE,
                          const long id = Event::ANY_ID) const;

    /**
       * Makes the expression handler an observer of the next future triggering of
       * the event expression.
       *
       * Only the next trigger of the event expression will trigger the expression handler.
       * After that the expression handler will be automatically dismissed.
       *
       * @param eventHandler
       *            the observer to be triggered when the event expression occurs.
       * @param eventExpression
       *            the event expression to observe.
       *
       */
    HandlerToken waitOnce(const std::shared_ptr<EventHandler> &eventHandler,
                          const EventExpression& eventExpression) const;

private:
    /**
    * Effective method to be called by all other dismiss interface methods.
    */
    void dismissHandler(std::shared_ptr<EventHandler> eventHandler, const EventSource source, const EventType &type,
                        const EventID id) ;

    /**
     * Effective method to be called by all other dismiss interface methods.
     */
    void dismissHandler(const HandlerToken& token) ;

};

/**
* Overloads the << operator
*/
std::ostream &operator<<(std::ostream &strm, const Entity &entity);
}
#endif
