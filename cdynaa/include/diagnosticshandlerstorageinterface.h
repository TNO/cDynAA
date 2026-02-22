// NOTE: a guard with license and author info is auto added to package distributions (see setup.py)
#ifndef INCLUDED_DIAGNOSTICSHANDLERSTORAGEINTERFACE_
#define INCLUDED_DIAGNOSTICSHANDLERSTORAGEINTERFACE_

#include "dynaatypes.h"
#include "handlertoken.h"
#include "event.h"
#include "eventexpression.h"
#include "handlerreport.h"
#include <vector>


namespace dynaa {

long typedef HandlerReportLabel;

struct DiagnosticsHandlerReport {
    // Simulation timestamp
    SimTime timestamp;
    // Handler report
    HandlerReport hReport;
    // Identifier of Handler token
    HandlerToken hToken;
    // Report label
    HandlerReportLabel label;
};

std::vector<DiagnosticsHandlerReport> typedef DiagnosticsHandlerReportsList;

class DiagnosticsHandlerStorageInterface {
public:
    /**
     * Sets a value for the label of future records
     */
    virtual void setLabel(const HandlerReportLabel new_label) = 0;

    /**
     * Returns the value for the label currently used
     */
    virtual HandlerReportLabel getLabel() const = 0;

    /**
     * Clears the memory
     */
    virtual void clear() = 0;

    /**
     * Adds a new record
     */
    virtual void addDiagnosticsRecord(const SimTime timestamp, const HandlerReport hReport, const HandlerToken hToken) = 0;

    /**
     * Retrieves all the records including information of the run
     */
    virtual DiagnosticsHandlerReportsList getRecords() = 0;

    DiagnosticsHandlerStorageInterface() = default;
    DiagnosticsHandlerStorageInterface(const DiagnosticsHandlerStorageInterface&) = default;
    DiagnosticsHandlerStorageInterface(DiagnosticsHandlerStorageInterface&&) = default;
    virtual ~DiagnosticsHandlerStorageInterface() = default;
    DiagnosticsHandlerStorageInterface& operator=(const DiagnosticsHandlerStorageInterface&) = default;
    DiagnosticsHandlerStorageInterface& operator=(DiagnosticsHandlerStorageInterface&&) noexcept = default;

};
}

#endif //INCLUDED_DIAGNOSTICSHANDLERSTORAGEINTERFACE_