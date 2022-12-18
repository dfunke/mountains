
#include "sweepline_datastruct.h"
#include "../primitives.h"
#include "../isolation_finder.h"

void SweeplineDatastruct::insert(SlEvent* node) {
    // pass
}

IsolationRecord SweeplineDatastruct::calcPeak(const SlEvent *node) {
    return IsolationRecord();
}

SweeplineDatastruct::~SweeplineDatastruct(){}

void SweeplineDatastruct::saveHeatMap(int level){}

void SweeplineDatastruct::saveAsImage(int w, int h, int level, Offsets peak, Offsets highestElev) {}