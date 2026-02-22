#include "../include/diagnosticseventstorageinterface.h"
#include "../include/diagnosticseventstorage.h"
#include <numeric>

using namespace dynaa;

DiagnosticsEventStorage::DiagnosticsEventStorage() {
    records.reserve(PREALLOC_SIZE);
    label = 0;
}

DiagnosticsEventStorage::~DiagnosticsEventStorage() {
    this->clear();
}

void DiagnosticsEventStorage::setLabel(const EventRecordLabel new_label) {
    label = new_label;
}

EventRecordLabel DiagnosticsEventStorage::getLabel() const {
    return label;
}

void DiagnosticsEventStorage::clear() {
    records.clear();
}

void DiagnosticsEventStorage::addDiagnosticsRecord(const SimTime timestamp, const Event event) {
    records.push_back({timestamp, event, label});
}

DiagnosticsEventRecordsList DiagnosticsEventStorage::getRecords() {
    return records;
}

// void DiagnosticsEventStorage::printFullRecords() {
//     // TODO: Fix accessing source safely
//     DiagnosticsFullEventRecordsList recs = getFullRecords();
//     std::cout << "TIME     SOURCE               TYPE           RUN" << std::endl;
//     for (auto it = recs.begin(); it < recs.end(); it++) {
//         std::cout << it->record.timestamp << "     " << it->record.event.source() << "      " << it->record.event.type() <<
//                   "      " << it->run << std::endl;
//     }
// }