// Mock for sweepline datasctruct

#ifndef _SWEEPLINE_DATASTRUCT_NAIVE_H__
#define _SWEEPLINE_DATASTRUCT_NAIVE_H__

#include "isolation_finder_sl.h"
#include "sweepline_primitives.h"
#include "sweepline_datastruct.h"

#include "../tile_cache.h"
#include "../isolation_finder.h"

#include <vector>

class SweeplineDatastructNaive : public SweeplineDatastruct {
public:
    SweeplineDatastructNaive(int width, int height, LatLng topLeft, LatLng bottomRight);
    ~SweeplineDatastructNaive();

    void insert(SlEvent *node) override;

    void remove(const SlEvent *node);

    virtual void saveHeatMap(int level) override;

    IsolationRecord calcPeak(const SlEvent *node) override;

    void saveAsImage(int w, int h, int level, SlEvent **allNodes, int nodeSize);

    SlEvent* findByCoordinates(int x, int y) const;
    
private:
    Offsets getOffsets(const LatLng* pos) const;
    SlEvent** nodes;
    Elevation *mSamples;
    LatLng mTopLeft;
    LatLng mBottomRight;
    int pWidth;
    int pHeight;
};

# endif // _SWEEPLINE_DATASTRUCT_NAIVE_H__

