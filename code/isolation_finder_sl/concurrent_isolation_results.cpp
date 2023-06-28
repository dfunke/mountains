#include "concurrent_isolation_results.h"
#include "../latlng.h"
#include "../isolation_results.h"

#include <tbb/concurrent_vector.h>

void ConcurrentIsolationResults::addResult(const LatLng &peakLocation, Elevation elevation, float isolationKm) {
  IsolationResult result;
  result.peak = peakLocation;
  result.peakElevation = elevation;
  result.isolationKm = isolationKm;
  mResults.push_back(result);
}

std::size_t ConcurrentIsolationResults::size() const {
    return mResults.size();
}
