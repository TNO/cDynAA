// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_DIAGNOSTICSEVENTSTORAGEINTERFACE_
#define INCLUDED_DIAGNOSTICSEVENTSTORAGEINTERFACE_

#include "dynaatypes.h"
#include "handlertoken.h"
#include "event.h"
#include "eventexpression.h"
#include <vector>


namespace dynaa {

long typedef EventRecordLabel;

struct DiagnosticsEventRecord {
    // Event timestamp
    SimTime timestamp;
    // Event itself
    Event event;
    // Record label
    EventRecordLabel label;
};

std::vector<DiagnosticsEventRecord> typedef DiagnosticsEventRecordsList;

class DiagnosticsEventStorageInterface {
public:
    /**
     * Sets a value for the label of future records
     */
    virtual void setLabel(const EventRecordLabel new_label) = 0;

    /**
     * Returns the value for the label currently used
     */
    virtual EventRecordLabel getLabel() const = 0;
    /**
     * Clears the memory
     */
    virtual void clear() = 0;

    /**
     * Adds a new record
     */
    virtual void addDiagnosticsRecord(const SimTime timestamp, const Event event) = 0;

    /**
     * Retrieves all the records including information of the run
     */
    virtual DiagnosticsEventRecordsList getRecords() = 0;

    DiagnosticsEventStorageInterface() = default;
    DiagnosticsEventStorageInterface(const DiagnosticsEventStorageInterface&) = default;
    DiagnosticsEventStorageInterface(DiagnosticsEventStorageInterface&&) = default;
    virtual ~DiagnosticsEventStorageInterface() = default;
    DiagnosticsEventStorageInterface& operator=(const DiagnosticsEventStorageInterface&) = default;
    DiagnosticsEventStorageInterface& operator=(DiagnosticsEventStorageInterface&&) noexcept = default;

};
}

#endif //INCLUDED_DIAGNOSTICSEVENTSTORAGEINTERFACE_