/*
file: defaultscheduler.cpp

Member functions of the class defaulthandlerregistry
*/

#include "../include/defaulthandlerregistry.h"
#include "../include/coreexceptions.h"
#include "../include/entity.h"
#include "../include/simengine.h"
#include "../include/diagnostics.h"
#include <algorithm>
#include <memory>

using namespace dynaa;
using namespace std;

static const long INVALID_RECORDS_THRESHOLD = 100;


void DefaultHandlerRegistry::reset() {
    handlerRecordDB.clear();
    maskToRecordDB.clear();
}

std::set<HandlerToken> DefaultHandlerRegistry::getEventHandlersOf(const Event &event) {
    HandlerTokenSet handlers;
    try {

        const long source_id = event.source()->getUID();
        const long type_id = event.type().id();
        const long id = event.id();
        const long defaultIDMask = EventMask::defaultIDMask;

        // Append handlers with the 8 different types of masks we can produce from the event.
        appendHandlersTo(&handlers, {defaultIDMask, defaultIDMask, defaultIDMask});
        appendHandlersTo(&handlers, {source_id, defaultIDMask, defaultIDMask});
        appendHandlersTo(&handlers, {defaultIDMask, type_id, defaultIDMask});
        appendHandlersTo(&handlers, {defaultIDMask, defaultIDMask, id});
        appendHandlersTo(&handlers, {defaultIDMask, type_id, id});
        appendHandlersTo(&handlers, {source_id, defaultIDMask, id});
        appendHandlersTo(&handlers, {source_id, type_id, defaultIDMask});
        appendHandlersTo(&handlers, {source_id, type_id, id});

    } catch (InvalidEventSourceException & e) { // If it occurs, ignore this event because source does not exist
        handlers.clear();
    }
    return handlers;

}

bool is_valid_record_tag(const RecordTag& tag) {
    const auto record = tag.record.lock();
    return record ? record->is_valid() : false;
}

void DefaultHandlerRegistry::appendHandlersTo(HandlerTokenSet *const handlers, const EventMask& key) {
    const auto iter = maskToRecordDB.find(key);
    if (iter != maskToRecordDB.end()) {
        std::set<RecordTag>& matchingHandlers = iter->second;
        for (auto it = matchingHandlers.begin(), last = matchingHandlers.end(); it != last;) {
            if (is_valid_record_tag(*it)) {
                (*handlers).insert(*it);
                ++it;
            } else {
                it = matchingHandlers.erase(it);
            }
        }
    }
}

void DefaultHandlerRegistry::unregisterHandler(const shared_ptr<EventHandler>& eventHandler,
        const EventSource &entity,
        const EventType &eventType,
        const EventID id) {
    EventExpression evExpr(entity, eventType, id);
    unregisterHandler(eventHandler, evExpr);
}

void DefaultHandlerRegistry::unregisterHandler(const HandlerToken& token) {
    auto record_it = handlerRecordDB.find(token);
    if (record_it != handlerRecordDB.end()) {
        record_it->second->invalidate();
    }
}

HandlerToken DefaultHandlerRegistry::registerHandler(const shared_ptr<EventHandler>& eventHandler,
        const EventExpression& evExpr, bool once) {
    auto record = make_shared<HandlerRecord>(evExpr, eventHandler);
    record->set_once(once);
    if (!record) {
        throw invalid_argument("trying to create an empty record");
    }
    if (!record->getEventHandler()) {
        throw invalid_argument("expression handler argument cannot be a null pointer.");
    }

    const HandlerToken newRecordID = record->getUID();

    // Add record to registry
    handlerRecordDB[newRecordID] = record;

    // Add record references into mask to handler map
    for (auto keyMask : record->getEventMasks()) {
        maskToRecordDB[keyMask].insert(RecordTag(newRecordID, record));
    }
    Diagnostics::getInstance().incrementRegisteredAtomExprCounter(evExpr.numberOfAtomicExpressions());
    return newRecordID;
}

void DefaultHandlerRegistry::unregisterHandler(const shared_ptr<EventHandler>& eventHandler,
        const EventExpression& evExpr) {
    for (auto entry: handlerRecordDB) {
        auto const &record = entry.second;
        if (record) {
            if ((record->getEventHandler() == eventHandler) && (record->getEventExpression() == evExpr)) {
                record->invalidate();
            }
        }
    }
}

void DefaultHandlerRegistry::unregisterHandler(const shared_ptr<EventHandler>& eventHandler) {
    for (auto entry: handlerRecordDB) {
        auto const &record = entry.second;
        if (record) {
            if (record->getEventHandler() == eventHandler) {
                record->invalidate();
            }
        }
    }
}

bool is_valid_db_record(const std::pair<HandlerToken, std::shared_ptr<HandlerRecord>>& pair) {
    auto &record = pair.second;
    return record ? record->is_valid() : false;
}

long DefaultHandlerRegistry::getNumberOfHandlers() const {
    return std::count_if(handlerRecordDB.begin(),
                         handlerRecordDB.end(),
                         is_valid_db_record);
}

bool DefaultHandlerRegistry::needs_cleaning() const {
    return m_invalid_records > INVALID_RECORDS_THRESHOLD;
}

void DefaultHandlerRegistry::clean() {
    // TODO: create a erase_if for maps and sets
    for(auto it = handlerRecordDB.begin(); it != handlerRecordDB.end(); ) {
        if (!is_valid_db_record(*it)) {
            it = handlerRecordDB.erase(it);
        } else {
            ++it;
        }
    }
    for(auto it = maskToRecordDB.begin(); it != maskToRecordDB.end(); ) {
        for (auto tag_it = it->second.begin(); tag_it != it->second.end(); ) {
            if (!is_valid_record_tag(*tag_it)) {
                tag_it = it->second.erase(tag_it);
            } else {
                ++tag_it;
            }
        }
        if (it->second.empty()) {
            it = maskToRecordDB.erase(it);
        } else {
            ++it;
        }
    }
    m_invalid_records = 0;
}

void DefaultHandlerRegistry::activateHandler(const HandlerToken& token, const Event& event) {
    auto idx = handlerRecordDB.find(token);
    if (idx == handlerRecordDB.end()) return;  // token does not exist, ignore (fails silently)

    auto record = idx->second;
    if (!record) throw runtime_error("Trying to activate handler with invalid token.");

    auto event_handler = record->getEventHandler();
    if (!event_handler) throw runtime_error("Trying to activate non-existing handler.");

    //event_handler->handle(event);
    HandlerReport event_report = record->createEventReport();
    event_handler->handle(event_report);
    Diagnostics::getInstance().incrementCallbacksCounter();
    Diagnostics::getInstance().addDiagnosticsRecord(token, event_report);

    // Actualizing record "after" firing handle
    //CHECK: should that be before invokation?
    if (record->is_once()) {
        record->invalidate();
    } else {
        record->reset();
    }
}

bool DefaultHandlerRegistry::updateHandler(const HandlerToken& token, const Event& event, const SimTime updateTime) {
    auto idx = handlerRecordDB.find(token);
    if (idx == handlerRecordDB.end()) return false;  // token does not exist, ignore (fails silently)

    auto record = idx->second;
    if (!record) throw runtime_error("Trying to update record with invalid token.");

    record->update(event, updateTime);
    if (record->triggered()) {
        Diagnostics::getInstance().storeTriggeredUserExprComplexity(record->getEventExpression().atoms().size());
        return true;
    }
    return false;
}
