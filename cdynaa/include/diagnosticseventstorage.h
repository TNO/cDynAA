// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_DIAGNOSTICSEVENTSTORAGE_
#define INCLUDED_DIAGNOSTICSEVENTSTORAGE_

#define PREALLOC_SIZE 1000

#include <iostream>
#include <string>
#include <vector>
#include <iostream>
#include "diagnosticseventstorageinterface.h"
//#include "eventexpression.h"

namespace dynaa {

class DiagnosticsEventStorage: public DiagnosticsEventStorageInterface {
public:
    /**
     * Sets a value for the label of future records
     */
    void setLabel(const EventRecordLabel new_label);

    /**
     * Returns the value for the label currently used
     */
    EventRecordLabel getLabel() const;

    /**
     * Clears the memory, not recoverable
     */
    void clear();

    /**
     * Adds a new entry to the diagnostics records
     */
    void addDiagnosticsRecord(const SimTime timestamp, const Event event);

    /**
     * Retrieves all the records including information of the run
     */
    DiagnosticsEventRecordsList getRecords();

    DiagnosticsEventStorage();
    ~DiagnosticsEventStorage();

private:
    /**
     * Stores the records efficiently
     */
    DiagnosticsEventRecordsList records;

    /**
     * Stores the value of the label that will be attached to new records
     */
    EventRecordLabel label;

    // /**
    //  * Helper function when debugging, prints records as a table
    //  */
    // void printFullRecords();
};
}

#endif //INCLUDED_DIAGNOSTICSEVENTSTORAGE_