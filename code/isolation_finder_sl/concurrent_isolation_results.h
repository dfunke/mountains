
#ifndef _CONCURRENT_ISOLATION_RESULTS_H_
#define _CONCURRENT_ISOLATION_RESULTS_H_

#include "../isolation_results.h"
#include "../latlng.h"
#include <atomic>
#include <tbb/concurrent_vector.h>

class ConcurrentIsolationResults {
public:
  ConcurrentIsolationResults() {}

  void addResult(const LatLng &peakLocation, int elevationMeters,
                 float isolationKm);

  std::size_t size() const;
  tbb::concurrent_vector<IsolationResult> mResults;
};

#endif
