// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_EVENTHANDLER_
#define INCLUDED_EVENTHANDLER_
/* File eventhandler.h
 */

#include "event.h"
#include "handlerreport.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <set>

namespace dynaa {

/**
 * EventHandler
 *
 */
class EventHandler {

private:
    /**
    * A generator for unique identifiers of event handlers
    */
    static long m_handlerSequenceNumber;

    /**
     * An identifier for print outs of this event handler.
     */
    const std::string identifier;

protected:

    /**
     * An identifier for this event handler
     */
    const long id;

    /**
     * The callback function associated with this event handler.
     */
    const std::function<void(HandlerReport)> m_eventCallback;

    /**
     * The priority level for this event handler. Range 1 - 10 : 1 is more urgent.
     */
    const int priority;

    /**
     * A list of weak pointers to be used as safe_guards for calling the handle.
     */
    const std::vector<std::weak_ptr<Entity>> safe_guards;

    static bool safeCheck(std::weak_ptr<Entity> safe_guard);

public:
    static std::shared_ptr<EventHandler> create(std::function<void(HandlerReport)> f, int priority);
    static std::shared_ptr<EventHandler> create(std::function<void(HandlerReport)> f, std::string identifier, int priority);
    static std::shared_ptr<EventHandler> create(std::function<void(HandlerReport)> f,
            std::string identifier = DEFAULT_HANDLER_IDENTIFIER,
            std::vector<std::weak_ptr<Entity>> safe_guards = {}, int priority = DEFAULT_PRIORITY_LEVEL);
    static std::shared_ptr<EventHandler> create(std::function<void(HandlerReport)>,
            std::vector<std::weak_ptr<Entity>> safe_guards,
            int priority = DEFAULT_PRIORITY_LEVEL);

    static const int DEFAULT_PRIORITY_LEVEL;
    static const int HIGHEST_PRIORITY_LEVEL;
    static const int LOWEST_PRIORITY_LEVEL;
    static const string DEFAULT_HANDLER_IDENTIFIER;

    /**
     * Constructor
     */
    EventHandler(const std::function<void(HandlerReport)> &f, const std::string &identifier = DEFAULT_HANDLER_IDENTIFIER,
                 const std::vector<std::weak_ptr<Entity>> &safe_guards = {}, int priority = DEFAULT_PRIORITY_LEVEL) :
        identifier(identifier),
        id(EventHandler::m_handlerSequenceNumber++),
        m_eventCallback(f),
        priority(priority),
        safe_guards(safe_guards) {
        if ((priority < HIGHEST_PRIORITY_LEVEL) || (priority > LOWEST_PRIORITY_LEVEL)) {
            throw InvalidEventHandlerPriority();
        }
    }

    /**
     * Constructor
     */
    EventHandler(const std::function<void(HandlerReport)> &f, const std::string &the_identifier, int the_priority) :
        EventHandler(f, the_identifier, {}, the_priority) {}

    /**
     * Constructor
     */
    EventHandler(const std::function<void(HandlerReport)> &f, int the_priority) :
        EventHandler(f,DEFAULT_HANDLER_IDENTIFIER, the_priority) {}

    /**
     * Constructor
     */
    EventHandler(const std::function<void(HandlerReport)> &f, const std::vector<std::weak_ptr<Entity>> &the_safe_guards,
                 int the_priority = DEFAULT_PRIORITY_LEVEL) :
        EventHandler(f, DEFAULT_HANDLER_IDENTIFIER, the_safe_guards, the_priority) {}

    /**
     * Default Constructor
     *
     * Has no callback function.
     */
    EventHandler() :
        //m_eventCallback(nullptr),
        identifier(DEFAULT_HANDLER_IDENTIFIER),
        id(EventHandler::m_handlerSequenceNumber++),
        priority(DEFAULT_PRIORITY_LEVEL),
        safe_guards({}) {}

    /**
     * Copy constructor
     */
    EventHandler(const EventHandler& eventHandler) :
        identifier(eventHandler.identifier),
        id(eventHandler.id),
        m_eventCallback(eventHandler.m_eventCallback),
        priority(eventHandler.priority),
        safe_guards(eventHandler.safe_guards) {}

    /**
     * Destructor
     */
    ~EventHandler() {}

    /**
    * Handles the event .
    *
    * @param handler_report
    *            a report for the triggering of the event (expression) that
    *            triggered this handle call.
    */
    void handle(HandlerReport handler_report);

    /**
     * Get the identifier of the event handler in a way that makes human
     * interpretation possible
     *
     * @return A identifier for human interpretation
     */
    const std::string getIdentifier();

    /**
     * Retrieves the priority level of this event handler
     */
    int getPriorityLevel() const;

    /*
    * Used for comparison of event handlers in maps and sets
    */
    bool operator<(const EventHandler &other) const;
};

}

#endif
