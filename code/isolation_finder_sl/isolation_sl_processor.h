#ifndef _ISOLATION_SL_PROCESSOR_H_
#define _ISOLATION_SL_PROCESSOR_H_

#include "../isolation_results.h"
#include "../tile_cache.h"
#include "ilp_search_area_tree.h"

using std::vector;

class IsolationSlProcessor {
public:
  explicit IsolationSlProcessor(TileCache *cache, FileFormat format);
  double findIsolations(int numThreads, float bounds[],
                                  float mMinIsolationKm);

private:
  TileCache *mCache;
  vector<IsolationResults> mBuckets;
  ILPSearchAreaTree *mSearchTree;
  FileFormat mFormat;
};

#endif // _ISOLATION_SL_PROCESSOR_H_
