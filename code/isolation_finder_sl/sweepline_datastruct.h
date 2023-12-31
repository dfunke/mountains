// Mock for sweepline datasctruct

#ifndef _SWEEPLINE_DATASTRUCT_H__
#define _SWEEPLINE_DATASTRUCT_H__

#include "isolation_finder_sl.h"
#include "sweepline_primitives.h"
#include "../isolation_finder.h"

class SweeplineDatastruct {
public:
    virtual ~SweeplineDatastruct();
    virtual void insert(SlEvent *node);
    virtual void remove(const SlEvent *node);
    virtual IsolationRecord calcPeak(const SlEvent *node);
};

# endif // _SWEEPLINE_DATASTRUCT_H__
