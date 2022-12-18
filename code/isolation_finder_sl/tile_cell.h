#ifndef _TILE_CELL_H_
#define _TILE_CELL_H_

#include "concurrent_isolation_results.h"

#include "../latlng.h"
#include "../primitives.h"
#include "../isolation_results.h"
#include "../lock.h"

class TileCell {
public:
    TileCell(int minLat, int minLng, int offsetLat, int offsetLng);
    ~TileCell();
    void insert(int minLat, int minLng, Elevation maxElev, ConcurrentIsolationResults *bucket);
    void distributeToBuckets(const LatLng &peakLocation, int elevation, const LatLng &higherLocation, float *isolationKm);
    ConcurrentIsolationResults *findBucket(int mMinLat, int mMinLng);
private:
    float maxDistanceToCell(const LatLng &point) const;
    Lock mLock;
    ConcurrentIsolationResults *mBucket;
    LatLng mTopLeft;
    LatLng mBottomRight;
    TileCell* smaller;
    TileCell* bigger;
    Elevation mMaxElev = INT16_MIN;
};

# endif // _TILE_CELL_H_
