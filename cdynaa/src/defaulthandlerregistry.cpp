/*
file: defaultscheduler.cpp
author: Julio Oliveira and Coen van Leeuwen, TNO

Member functions of the class defaulthandlerregistry
*/

#include "../include/defaulthandlerregistry.h"
#include "../include/coreexceptions.h"
#include "../include/entity.h"
#include "../include/simengine.h"
#include <algorithm>
#include <memory>

using namespace dynaa;
using namespace std;

const long DefaultHandlerRegistry::defaultIDMask = 0xFFFFFFFFFFFFFFFF;

// Functions native of the struct EventMask

bool DefaultHandlerRegistry::EventMask::operator==(const EventMask &other) const {
    return (sourceID == other.sourceID && typeID == other.typeID && eventID == other.eventID);
}

bool DefaultHandlerRegistry::EventMask::matches(const EventMask &mask,
        const EventMask &masked) {
    return matches(mask.sourceID, masked.sourceID) &&
           matches(mask.typeID, masked.typeID) &&
           matches(mask.eventID, masked.eventID);
}

bool DefaultHandlerRegistry::EventMask::matches(const long mask, const long masked) {
    return (mask == DefaultHandlerRegistry::defaultIDMask) ? true : mask == masked;
}

std::size_t DefaultHandlerRegistry::EventMask::operator()(const EventMask &k) const {
    return (((k.sourceID) ^ ((k.typeID) << 13)) >> 7) ^ ((k.eventID) << 3);
}


// Normal methods: implementation

void DefaultHandlerRegistry::reset() {
    handlerMap.clear();
    exprHandlerMap.clear();
}

// Methods for comparing handler priority


// *************************************************
// Utility functions
// *************************************************

/**
 * Utility function to remove dismissed handlers from a set of handlers.
 *
 * @see DefaultHandlerRegistry::appendHandlersTo
 */
void removeSingleShotHandlers(HandlerSet &eventHandlers) {
    for (HandlerSet::iterator iter = eventHandlers.begin(); iter != eventHandlers.end();) {
        //if ((*iter)->isOnce()) {
        if ((*iter).once) {
            eventHandlers.erase(iter++);
        } else {
            ++iter;
        }
    }
}

// TODO this method should be 'const', however, it currently also takes care of cleaning up
// single-shot handlers. That responsibility should be moved to another (non-query) method
HandlerSet DefaultHandlerRegistry::getEventHandlersOf(const Event &event) {
    HandlerSet handlers;
    try {

        const EventSource source = event.source();
        const EventType &type = event.type();
        const EventID id = event.id();

        appendHandlersTo(&handlers, Event::ANY_SOURCE, Event::ANY_TYPE, Event::ANY_ID);
        appendHandlersTo(&handlers, source, Event::ANY_TYPE, Event::ANY_ID);
        appendHandlersTo(&handlers, Event::ANY_SOURCE, type, Event::ANY_ID);
        appendHandlersTo(&handlers, Event::ANY_SOURCE, Event::ANY_TYPE, id);
        appendHandlersTo(&handlers, Event::ANY_SOURCE, type, id);
        appendHandlersTo(&handlers, source, Event::ANY_TYPE, id);
        appendHandlersTo(&handlers, source, type, Event::ANY_ID);
        appendHandlersTo(&handlers, source, type, id);

    } catch (InvalidEventSourceException & e) { // It it occurs, ignore this event because source does not exist
        handlers.clear();
    }
    return handlers;

}

ExpressionHandlerSet DefaultHandlerRegistry::getExpressionHandlersOf(const shared_ptr<EventExpression>& evExpr) const {
    ExpressionHandlerSet handlers;
    auto iter = exprHandlerMap.find(evExpr);
    if (iter != exprHandlerMap.end()) {
        handlers = iter->second;  // this does a copy
    }
    return handlers;
}

void DefaultHandlerRegistry::appendHandlersTo(HandlerSet *const handlers, const EventSource source,
        const EventType &type, const long id) {
    const EventMask key = generateMask(source, type, id);
    const auto iter = handlerMap.find(key);
    if (iter != handlerMap.end()) {
        HandlerSet& matchingHandlers = iter->second;
        handlers->insert(matchingHandlers.begin(), matchingHandlers.end());
        removeSingleShotHandlers(matchingHandlers);
        if (matchingHandlers.empty()) {
            handlerMap.erase(key);
        }
    }
}

void DefaultHandlerRegistry::registerHandler(shared_ptr<EventHandler> eventHandler,
        const EventSource entity,
        const EventType &eventType,
        const EventID id,
        const bool once) {
    const EventMask key = generateMask(entity, eventType, id);
    auto iter = handlerMap.find(key);
    RegisteredHandler<EventHandler> regHandler = RegisteredHandler<EventHandler>(eventHandler, once);
    if (iter != handlerMap.end()) {
        auto result = iter->second.insert(regHandler);
        if (!(result.second)) {
            // RegisteredHandler already existed, so just update its 'once' value
            (*result.first).once = once;
        }
    } else {
        HandlerSet handlers = HandlerSet({regHandler});
        handlerMap[key] = handlers;
    }
}

void DefaultHandlerRegistry::unregisterHandler(shared_ptr<EventHandler> eventHandler,
        const EventSource entity,
        const EventType &eventType,
        const EventID id) {
    // NOTE this does expensive O(N) search to compare against all masks!
    const EventMask mask = DefaultHandlerRegistry::generateMask(entity, eventType, id);
    RegisteredHandler<EventHandler> regHandler = RegisteredHandler<EventHandler>(eventHandler);
    for (auto iter = handlerMap.begin(); iter != handlerMap.end(); ) {
        const EventMask &handlerKey = iter->first;
        HandlerSet &handlers = iter->second;
        if (DefaultHandlerRegistry::EventMask::matches(mask, handlerKey)) {
            //handlers.erase(eventHandler);
            handlers.erase(regHandler);
            if (handlers.empty()) {
                handlerMap.erase(iter++);
                continue;
            }
        }
        ++iter;
    }
}

void DefaultHandlerRegistry::triggerAtomicExpression(
            weak_ptr<EventExpression> parentExpr,
            weak_ptr<EventExpression> atomicExpr, Event event) {
    if (parentExpr.expired() || atomicExpr.expired()) {
        return;
    }
    shared_ptr<EventExpression> spParentExpr = parentExpr.lock();
    shared_ptr<EventExpression> spAtomicExpr = atomicExpr.lock();
    if (spParentExpr->value()) {
        // If parent expression already triggered exit.
        // This can happen if parent has children with the same event mask,
        // second child will still be handled even though parent already unregistered
        //cout << "WARNING: a parent expression was already triggered" << endl;
        return;
    }
    // If atomic expression not already triggered, then trigger with this event
    if (!spAtomicExpr->value()) {
        spAtomicExpr->m_trigger_event = unique_ptr<Event>(new Event(event));
        spAtomicExpr->m_trigger_timestamp = SimulationEngine::getInstance().getCurrentTime();
    }
    if (!spParentExpr->value()) {
        return;
    }
    // If expression handler is missing, warn and exit
    auto iter = exprHandlerMap.find(spParentExpr);
    if (iter == exprHandlerMap.end()) {
        // TODO this occurs sometimes (but causes no issues), how?? Is it because simengine
        // already has callback set and we clear exprHandlerMap below?
        cout << "WARNING: a expression handler missing" << endl;
        return;
    }
    // Copy the handler set in case users changes it using callbacks (e.g. new wait)
    ExpressionHandlerSet exprHandlerSet = iter->second;  // copy
    // Handle all registered expression handlers
    bool repeat = false;
    for (auto regExprHandler: exprHandlerSet) {
        if (regExprHandler.once) {
            // Unregister handler
            iter->second.erase(regExprHandler);
        } else if (!repeat) {
            repeat = true;
        }
        // TODO count expression handler calls
        regExprHandler.handler->handle(spParentExpr);
        // Check if user modified the event expression during handling of last callback
        // If so, stop processing callbacks right away
        if (!spParentExpr->value()) {
            //cout << "WARNING: event expression modified during callbacks" << endl;
            return;
        }
    }
    // Check whether we should wait with this expression again
    if (repeat && !iter->second.empty()) {
        // Get priority from first element of set (should all be the same?)
        spParentExpr->reprime();
    } else {
        // Unregister atomic expression event handlers
        unregisterAtomicHandlers(spParentExpr);
        // Remove event expression
        exprHandlerMap.erase(spParentExpr);
    }
}

void DefaultHandlerRegistry::registerAtomicHandlers(
                        const shared_ptr<EventExpression>& parentExpr,
                        const shared_ptr<EventExpression>& evExpr, int priority) {
    if (evExpr->type() == Atomic) {
        EventSource evsource = evExpr->atomicEventSource();
        const EventType& evtype = evExpr->atomicEventType();
        EventID evID = evExpr->atomicEventID();
        if (auto source = evsource.lock()) {
            function<void(Event)> f = bind(&DefaultHandlerRegistry::triggerAtomicExpression, this,
                                           parentExpr, evExpr, std::placeholders::_1);
            shared_ptr<EventHandler> atomicHandler = make_shared<EventHandler>(f, priority);
            // Set handlers waiting continuously until dismissed, in case expression is reset
            // after some of its atomic expressions have triggered
            registerHandler(atomicHandler, source, evtype, evID, false);
            // store atomicHandlers somewhere so they can be dismissed prematurely
            if (!atomicHandlerMap.count(parentExpr)) {
                atomicHandlerMap[parentExpr] = vector<pair<weak_ptr<EventHandler>, const EventMask>>();
            }
            // Store atomic handler along with its exact mask for fast removal
            atomicHandlerMap[parentExpr].push_back(make_pair(atomicHandler, generateMask(source, evtype, evID)));
        } else{
            // TODO something more descriptive exception here?
            throw InvalidEventSourceException();
        }
    } else {
        registerAtomicHandlers(parentExpr, evExpr->firstTermAsPtr(), priority);
        registerAtomicHandlers(parentExpr, evExpr->secondTermAsPtr(), priority);
    }
}

void DefaultHandlerRegistry::unregisterAtomicHandlers(const shared_ptr<EventExpression>& evExpr) {
    // Unregister the internally registered atomic handlers
    // Uses their exact mask to avoid a slow O(N) search
    auto iter = atomicHandlerMap.find(evExpr);
    if (iter == atomicHandlerMap.end()) {
        return;
    }
    for (auto veciter: iter->second) {
        // Use exact mask to directly find correct handlers set
        const EventMask mask = veciter.second;
        auto iter3 = handlerMap.find(mask);
        if (iter3 == handlerMap.end()) {
            return;
        }
        HandlerSet &handlers = iter3->second;
        if (auto spEventHandler = veciter.first.lock()) {
            // Lookup 
            RegisteredHandler<EventHandler> regHandler = RegisteredHandler<EventHandler>(spEventHandler);
            handlers.erase(regHandler);
            if (handlers.empty()) {
                handlerMap.erase(iter3);
            }
            // Previously we called an expensive search:
            //unregisterHandler(spEventHandler, Event::ANY_SOURCE, Event::ANY_TYPE, Event::ANY_ID);
        }
    }
    atomicHandlerMap.erase(iter);
}

void DefaultHandlerRegistry::registerHandler(shared_ptr<ExpressionHandler> expressionHandler,
                                             shared_ptr<EventExpression> evExpr, bool once, bool reprime) {
    if (!expressionHandler) {
        throw invalid_argument("expression handler argument cannot be a null pointer.");
    }
    if (!evExpr) {
        throw invalid_argument("event expression argument cannot be a null pointer.");
    }
    // Add expression handler to event expression key
    auto iter = exprHandlerMap.find(evExpr);
    RegisteredHandler<ExpressionHandler> regExprHandler(expressionHandler, once);
    if (iter != exprHandlerMap.end()) {
        auto iter2 = iter->second.find(regExprHandler);
        if (iter2 != iter->second.end()) {
            // Update once value of existing expression handler
            iter2->once = once;
        } else {
            iter->second.insert(regExprHandler);
        }
    } else {
        exprHandlerMap[evExpr] = {regExprHandler};
    }
    // Reset expression handler if requested (default is to reset)
    if (reprime) {
        evExpr->reprime();
    }
    // If the new expressionHandler has a higher priority than currently set atomic handlers
    // then unregister them and reregister with the higher priority
    int priority = expressionHandler->getPriorityLevel();
    if (!atomicHandlerMap.count(evExpr) || atomicHandlerMap[evExpr].empty()) {
        registerAtomicHandlers(evExpr, evExpr, priority);
    } else {
        auto eventHandler = (atomicHandlerMap[evExpr].begin()->first).lock();
        if (eventHandler.get()->getPriorityLevel() > priority) {
            unregisterAtomicHandlers(evExpr);
            registerAtomicHandlers(evExpr, evExpr, priority);
        }
    }
}

void DefaultHandlerRegistry::unregisterHandler(const shared_ptr<ExpressionHandler>& expressionHandler,
                                               const shared_ptr<EventExpression>& evExpr) {
    if (!expressionHandler) {
        throw invalid_argument("expression handler argument cannot be a null pointer.");
    }
    RegisteredHandler<ExpressionHandler> regExprHandler =
        RegisteredHandler<ExpressionHandler>(expressionHandler);
    if (evExpr && evExpr.get() != nullptr) {
        // Find the given event expression
        auto iter = exprHandlerMap.find(evExpr);
        if (iter == exprHandlerMap.end()) {
            return;
        }
        ExpressionHandlerSet& exprHandlerSet = iter->second;
        exprHandlerSet.erase(regExprHandler);
        if (exprHandlerSet.empty()) {
            unregisterAtomicHandlers(evExpr);
            exprHandlerMap.erase(iter);
        }
    } else {
        // Look in all event expressions
        for (auto iter = exprHandlerMap.begin(); iter != exprHandlerMap.end();) {
            ExpressionHandlerSet& exprHandlerSet = iter->second;
            exprHandlerSet.erase(regExprHandler);
            if (exprHandlerSet.empty()) {
                unregisterAtomicHandlers(iter->first);
                iter = exprHandlerMap.erase(iter);
            } else {
                ++iter;
            }
        }
    }
}

const DefaultHandlerRegistry::EventMask DefaultHandlerRegistry::generateMask(const EventSource source_ptr,
        const EventType &type, const EventID id) {
    const long wildCardMask = DefaultHandlerRegistry::defaultIDMask;
    auto source = source_ptr.lock();
    const long sourceID = (source == Event::ANY_SOURCE) ? wildCardMask : source->getUID();
    const long typeID = (type == Event::ANY_TYPE) ? wildCardMask : type.id();
    const long eventID = (id == Event::ANY_ID) ? wildCardMask : id;
    return {sourceID, typeID, eventID};
}

long DefaultHandlerRegistry::getNumberOfHandlers() const {
    long num_handlers = 0;
    for (auto iter = handlerMap.begin(); iter != handlerMap.end(); iter++) {
        num_handlers += iter->second.size();
    }
    for (auto iter = exprHandlerMap.begin(); iter != exprHandlerMap.end(); iter++) {
        num_handlers += iter->second.size();
    }
    return num_handlers;
}
