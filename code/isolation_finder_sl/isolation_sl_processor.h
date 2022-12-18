#ifndef _ISOLATION_SL_PROCESSOR_H_
#define _ISOLATION_SL_PROCESSOR_H_

#include "../tile_cache.h"
#include "../isolation_results.h"
#include "tile_cell.h"

using std::vector;

class IsolationSlProcessor {
public:
    explicit IsolationSlProcessor(TileCache *cache);
    IsolationResults findIsolations(int numThreads, float bounds[], float mMinIsolationKm);
private:
    TileCache *mCache;
    vector<IsolationResults> mBuckets;
    TileCell *mTileTreeHead;
};

#endif // _ISOLATION_SL_PROCESSOR_H_