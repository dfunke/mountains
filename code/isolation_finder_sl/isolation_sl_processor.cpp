#include "isolation_sl_processor.h"
#include "../ThreadPool.h"
#include "../isolation_results.h"
#include "../lock.h"
#include "../math_util.h"
#include "../tile_cache.h"
#include "cell_memory_manager.h"
#include "concurrent_isolation_results.h"
#include "isolation_finder_sl.h"
#include "ilp_search_area_tree.h"

#include <algorithm>
#include <unordered_set>
#include <vector>
#include <iostream>

using std::vector;

IsolationSlProcessor::IsolationSlProcessor(TileCache *cache, FileFormat format) { 
  mCache = cache; 
  mFormat = format;
}

bool compare(IsolationResult const &el, IsolationResult const &er) {
  // Sort by peak possition
  return (el.peak == er.peak) ? (el.isolationKm > er.isolationKm)
         : el.peak.latitude() != er.peak.latitude()
             ? el.peak.latitude() < er.peak.latitude()
             : el.peak.longitude() < er.peak.longitude();
}

template <typename T>
using maxheap = std::priority_queue<T, vector<T>, decltype(&compare)>;

IsolationResults IsolationSlProcessor::findIsolations(int numThreads,
                                                      float bounds[],
                                                      float mMinIsolationKm) {
  int latMax = (int)ceil(bounds[1]);
  int lngMax = (int)ceil(bounds[3]);
  int latMin = (int)floor(bounds[0]);
  int lngMin = (int)floor(bounds[2]);
  mSearchTree=
      new ILPSearchAreaTree(latMin, lngMin, latMax - latMin, lngMax - lngMin);
  ThreadPool *threadPool = new ThreadPool(numThreads);
  vector<std::future<int>> voidFutures;
  // Create Buckets and build TileTree
  vector<IsolationFinderSl *> finders;
  //std::cout << "Start building tile-tree" << std::endl;
  vector<IsolationFinderSl *> *pFinders = &finders;
  // create finders
  for (int j = lngMin; j < lngMax; ++j) {
    for (int i = latMin; i < latMax; ++i) {
      IsolationFinderSl *finder =
          new IsolationFinderSl(mCache, mSearchTree, i, j, mFormat);
      pFinders->push_back(finder);
    }
  }

  // Fille Buckets step
  for (auto &finder : finders) {
    voidFutures.push_back(threadPool->enqueue(
        [=] { return finder->fillPeakBuckets(mMinIsolationKm); }));
  }

  for (auto &&waitFor : voidFutures) {
    waitFor.get();
  }

  // Proccess all unbound peaks
  mSearchTree->proccessUnbound();
  int i = 0;
  vector<std::future<int>> futureResults;
  vector<IsolationResults> results;
  //std::cout << "Start exact calculations" << std::endl;
  maxheap<IsolationResult> q(&compare);
  // Start exact calculations
  for (auto finder : finders) {
    if (!finder->nullPtrTile) {
      futureResults.push_back(
        threadPool->enqueue([=] { return finder->run(mMinIsolationKm); }));
    }
  }
  int64 phaseTwoPeaks = 0;
  for (auto &res : futureResults) {
    phaseTwoPeaks += res.get();
  }
  std::cout << phaseTwoPeaks << std::endl;
  //std::cout << "Start merging" << std::endl;
  //   Merge results
  IsolationResults finalResults;
  return finalResults;
}
