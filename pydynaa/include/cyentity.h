// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_CYTHON_ENTITY_
#define INCLUDED_CYTHON_ENTITY_
/* Definition of a modified Entity class that plays nicely with Cython.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "entity.h"
#include "eventtype.h"
#include "eventhandler.h"
#include "eventexpression.h"
#include "handlerreport.h"
#include "handlertoken.h"
#include <memory>
#include <vector>

namespace dynaa {
/**
 * Wrapper class for Entity class that is compatible with Cython
 *
 * Wraps the Entity class to give public access to protected methods for testing
 */
class CyEntity : public Entity {

private:
    PyObject* m_pyobj = NULL;

public:
    CyEntity() : Entity() {};

    CyEntity(PyObject* pyEntity) : Entity() {
        m_pyobj = pyEntity;
    };

    PyObject* pyobj() const {
        if (m_pyobj != NULL && Py_REFCNT(m_pyobj) > 0) {
            return m_pyobj;
        } else {
            return NULL;
        }
    };

    EventExpression scheduleNow(const EventType& evType) {
        return Entity::scheduleNow(evType);
    };
    EventExpression scheduleAfter(const SimTime interval, const EventType& evType) {
        return Entity::scheduleAfter(interval, evType);
    };
    EventExpression scheduleAt(const SimTime instant, const EventType& evType) {
        return Entity::scheduleAt(instant, evType);
    };

    // unschedule
    int unschedule(const EventType& eventType, const EventID eventID) {
        return Entity::unschedule(eventType, eventID);
    };

    // wait
    HandlerToken wait(const std::shared_ptr<EventHandler>& eventHandler, const EventSource& entity,
                      const EventType & eventType, const EventID eventID) const {
        return wait(eventHandler, EventExpression(entity, eventType, eventID));
    };
    HandlerToken wait(const std::shared_ptr<EventHandler>& eventHandler,
                      const EventType & eventType, const EventID eventID) const {
        return wait(eventHandler, EventExpression(Event::ANY_SOURCE, eventType, eventID));
    };
    HandlerToken wait(const std::shared_ptr<EventHandler>& eventHandler,
                      const EventExpression& ev_expression) const {
        return Entity::wait(eventHandler, ev_expression);
    };

    // waitOnce
    HandlerToken waitOnce(const std::shared_ptr<EventHandler>& eventHandler, const EventSource& entity,
                          const EventType & eventType, const EventID eventID) const {
        return waitOnce(eventHandler, EventExpression(entity, eventType, eventID));
    };
    HandlerToken waitOnce(const std::shared_ptr<EventHandler>& eventHandler,
                          const EventType& eventType, const EventID eventID) const {
        return waitOnce(eventHandler, EventExpression(Event::ANY_SOURCE, eventType, eventID));
    };
    HandlerToken waitOnce(const std::shared_ptr<EventHandler>& eventHandler, const EventExpression& ev_expression) const {
        return Entity::waitOnce(eventHandler, ev_expression);
    };

    // dismiss
    void dismiss(const HandlerToken& handlerToken) {
        Entity::dismiss(handlerToken);
    };
};


void add_to_ptr_vector(std::vector<std::weak_ptr<Entity>> &ptr_vector, std::shared_ptr<CyEntity> toAdd) {
    ptr_vector.push_back(toAdd);
}

}
#endif
