#include "isolation_sl_processor.h"
#include "../ThreadPool.h"
#include "../isolation_results.h"
#include "../lock.h"
#include "../math_util.h"
#include "../tile_cache.h"
#include "cell_memory_manager.h"
#include "concurrent_isolation_results.h"
#include "ilp_search_area_tree.h"
#include "isolation_finder_sl.h"

#include <algorithm>
#include <chrono>
#include <unordered_set>
#include <vector>

using std::vector;
using namespace std::chrono;

IsolationSlProcessor::IsolationSlProcessor(TileCache *cache,
                                           FileFormat format) {
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

double IsolationSlProcessor::findIsolations(int numThreads, float bounds[],
                                            float mMinIsolationKm) {
  double time = 0;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  int latMax = (int)ceil(bounds[1]);
  int lngMax = (int)ceil(bounds[3]);
  int latMin = (int)floor(bounds[0]);
  int lngMin = (int)floor(bounds[2]);
  mSearchTree =
      new ILPSearchAreaTree(latMin, lngMin, latMax - latMin, lngMax - lngMin);
  vector<IsolationFinderSl *> finders;
  // std::cout << "Start building tile-tree" << std::endl;

  // create finders
  for (int j = lngMin; j < lngMax; ++j) {
    for (int i = latMin; i < latMax; ++i) {
      IsolationFinderSl *finder =
          new IsolationFinderSl(mCache, mSearchTree, i, j, mFormat);
      finders.push_back(finder);
    }
  }
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
  time = time_span.count();
  // Fille Buckets step

  for (auto &finder : finders) {
    Tile *tile = finder->getTile();
    if (tile != nullptr) {
      t1 = high_resolution_clock::now();
      finder->fillPeakBuckets(mMinIsolationKm, tile);
      t2 = high_resolution_clock::now();
      time_span = duration_cast<duration<double>>(t2 - t1);
      time += time_span.count();
    }
  }

  t1 = high_resolution_clock::now();
  // Proccess all unbound peaks
  mSearchTree->proccessUnbound();
  int i = 0;
  vector<IsolationResults> results;
  // std::cout << "Start exact calculations" << std::endl;
  maxheap<IsolationResult> q(&compare);
  t2 = high_resolution_clock::now();
  time_span = duration_cast<duration<double>>(t2 - t1);
  time += time_span.count();
  // Start exact calculations
  for (auto finder : finders) {
    
      Tile *t = finder->getTile();
      if (t != nullptr) {
      t1 = high_resolution_clock::now();
      IsolationResults res = finder->run(mMinIsolationKm, t);
      for (IsolationResult &oneRes : res.mResults) {
        q.emplace(oneRes);
      }
      t2 = high_resolution_clock::now();
      time_span = duration_cast<duration<double>>(t2 - t1);
      time += time_span.count();
    }
  }
  // std::cout << "Start merging" << std::endl;
  //    Merge results
  t1 = high_resolution_clock::now();
  IsolationResults finalResults;
  TileCell newRoot(latMin, lngMin, latMax - latMin, lngMax - lngMin);
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
  // std::cout << "Sort final results by isolation" << std::endl;
  std::sort(finalResults.mResults.begin(), finalResults.mResults.end(),
            [](IsolationResult const &lhs, IsolationResult const &rhs) {
              return lhs.isolationKm > rhs.isolationKm;
            });
  IsolationResults finalFinalResults;
  // merge multiple detected peaks (with nearly same isolation)
  t2 = high_resolution_clock::now();
  time_span = duration_cast<duration<double>>(t2 - t1);
  time += time_span.count();
  return time;
}
