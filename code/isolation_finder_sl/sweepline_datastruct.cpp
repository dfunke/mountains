
#include "sweepline_datastruct.h"
#include "../primitives.h"
#include "../isolation_finder.h"

void SweeplineDatastruct::insert(SlEvent* node) {
    // pass
}
void SweeplineDatastruct::remove(const SlEvent* node) {
    // pass
}

IsolationRecord SweeplineDatastruct::calcPeak(const SlEvent *node) {
    return IsolationRecord();
}

SweeplineDatastruct::~SweeplineDatastruct(){}
