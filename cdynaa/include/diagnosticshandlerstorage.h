// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_DIAGNOSTICSHANDLERSTORAGE_
#define INCLUDED_DIAGNOSTICSHANDLERSTORAGE_

#define PREALLOC_SIZE 1000

#include <iostream>
#include <string>
#include <vector>
#include <iostream>
#include "diagnosticshandlerstorageinterface.h"

namespace dynaa {

class DiagnosticsHandlerStorage: public DiagnosticsHandlerStorageInterface {
public:
    /**
     * Sets a value for the label of future records
     */
    void setLabel(const HandlerReportLabel new_label);

    /**
     * Returns the value for the label currently used
     */
    HandlerReportLabel getLabel() const;

    /**
     * Clears the memory, not recoverable
     */
    void clear();

    /**
     * Adds a new entry to the diagnostics records
     */
    void addDiagnosticsRecord(const SimTime timestamp, const HandlerReport hReport, const HandlerToken hToken);

    /**
     * Retrieves all the records including information of the run
     */
    DiagnosticsHandlerReportsList getRecords();

    DiagnosticsHandlerStorage();
    ~DiagnosticsHandlerStorage();

private:
    /**
     * Stores the records efficiently
     */
    DiagnosticsHandlerReportsList records;

    /**
     * Stores the value of the label that will be attached to new records
     */
    HandlerReportLabel label;

    // /**
    //  * Helper function when debugging, prints records as a table
    //  */
    // void printFullRecords();
};
}

#endif //INCLUDED_DIAGNOSTICSHANDLERSTORAGE_