
#ifndef _CONCURRENT_ISOLATION_RESULTS_H_
#define _CONCURRENT_ISOLATION_RESULTS_H_

#include "../latlng.h"
#include "../isolation_results.h"
#include <tbb/concurrent_vector.h>
#include <atomic>

class ConcurrentIsolationResults {
public:
  ConcurrentIsolationResults(){}

  void addResult(const LatLng &peakLocation, int elevationMeters, const LatLng &higherLocation,
                 float isolationKm);

  std::size_t size() const;
  tbb::concurrent_vector<IsolationResult> mResults;
};

#endif