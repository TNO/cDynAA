#include "../include/diagnosticshandlerstorageinterface.h"
#include "../include/diagnosticshandlerstorage.h"
#include <numeric>

using namespace dynaa;

DiagnosticsHandlerStorage::DiagnosticsHandlerStorage() {
    records.reserve(PREALLOC_SIZE);
    label=0;
}

DiagnosticsHandlerStorage::~DiagnosticsHandlerStorage() {
    this->clear();
}

void DiagnosticsHandlerStorage::setLabel(const HandlerReportLabel new_label) {
    label = new_label;
}

HandlerReportLabel DiagnosticsHandlerStorage::getLabel() const {
    return label;
}

void DiagnosticsHandlerStorage::clear() {
    records.clear();
}

void DiagnosticsHandlerStorage::addDiagnosticsRecord(const SimTime timestamp, const HandlerReport hReport,
        const HandlerToken hToken) {
    records.push_back({timestamp, hReport, hToken, label});
}

DiagnosticsHandlerReportsList DiagnosticsHandlerStorage::getRecords() {
    return records;
}

// void DiagnosticsHandlerStorage::printFullRecords() {
//     DiagnosticsFullHandlerReportsList recs = getFullRecords();
//     std::cout << "TIME     RECORD      TOKEN           RUN" << std::endl;
//     for (auto it = recs.begin(); it < recs.end(); it++) {
//         std::cout << it->record.timestamp << "     " << "[not displayed]" << "      " << "[not implemented yet]" <<
//                   "      " << it->run << std::endl;
//     }
// }