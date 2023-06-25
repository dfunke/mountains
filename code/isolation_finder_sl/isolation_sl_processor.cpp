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

#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <vector>

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
      new ILPSearchAreaTree(latMin, lngMin, latMax - latMin, lngMax - lngMin, mFormat.degreesAcross());
  ThreadPool *threadPool = new ThreadPool(numThreads);
  vector<std::future<void>> voidFutures;
  // Create Buckets and build TileTree
  vector<IsolationFinderSl *> finders;
  std::cout << "Start building tile-tree" << std::endl;
  vector<IsolationFinderSl *> *pFinders = &finders;
  // create finders
  for (int j = lngMin; j < lngMax; j+= mFormat.degreesAcross()) {
    for (int i = latMin; i < latMax; i+= mFormat.degreesAcross()) {
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
  IsolationResult highPoint = mSearchTree->proccessUnbound();
  std::cout << "High point:" << highPoint.peak.latitude() << "," << highPoint.peak.longitude() << std::endl;
  int i = 0;
  vector<std::future<IsolationResults>> futureResults;
  vector<IsolationResults> results;
  std::cout << "Start exact calculations" << std::endl;
  maxheap<IsolationResult> q(&compare);
  // Start exact calculations
  for (auto finder : finders) {
    if (!finder->nullPtrTile) {
      futureResults.push_back(
        threadPool->enqueue([=] { return finder->run(mMinIsolationKm); }));
    }
  }
  for (auto &res : futureResults) {
    IsolationResults newResults = res.get();
    for (IsolationResult &oneResult : newResults.mResults) {
      q.emplace(oneResult);
    }
    newResults.mResults.clear();
  }
  std::cout << "Start merging" << std::endl;
  //   Merge results
  IsolationResults finalResults;
  TileCell newRoot(latMin, lngMin, latMax - latMin, lngMax - lngMin, mFormat.degreesAcross());
  while (!q.empty()) {
    if (q.top().isolationKm > mMinIsolationKm) {
      finalResults.mResults.push_back(q.top());
    }
    IsolationResult minResPos = q.top();
    q.pop();
    while (!q.empty() && q.top().peak == minResPos.peak) {
      q.pop();
    }
  }
  delete mSearchTree;
  delete threadPool;
  std::cout << "Sort final results by isolation" << std::endl;
  std::sort(finalResults.mResults.begin(), finalResults.mResults.end(),
            [](IsolationResult const &lhs, IsolationResult const &rhs) {
              return lhs.isolationKm > rhs.isolationKm;
            });
  IsolationResults finalFinalResults;
  // merge multiple detected peaks (with nearly same isolation)
  if (finalResults.mResults.size() == 0) {
    return finalResults;
  }
  // Filter out dublicates
  // for (std::size_t i = 1; i < finalResults.mResults.size(); ++i)
  //{
  //    // Check if peaks are nearly identical
  //    if (r.peak.distance(finalResults.mResults[i].peak) < 900)
  //    {
  //        // assume identical peak, use one with smaller isolation
  //        if (finalResults.mResults[i].isolationKm < r.isolationKm)
  //        {
  //            r = finalResults.mResults[i];
  //        }
  //    }
  //    else
  //    {
  //        finalFinalResults.mResults.push_back(r);
  //        r = finalResults.mResults[i];
  //    }
  //    // Add last result
  //    if (i == finalResults.mResults.size() - 1)
  //    {
  //        finalFinalResults.mResults.push_back(r);
  //    }
  //}
  finalResults.mResults.push_back(highPoint);
  return finalResults;
}
