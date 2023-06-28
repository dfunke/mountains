#ifndef _TILE_CELL_H_
#define _TILE_CELL_H_

#include "concurrent_isolation_results.h"

#include "../isolation_results.h"
#include "../latlng.h"
#include "../lock.h"
#include "../primitives.h"

class TileCell {
public:
  TileCell(int minLat, int minLng, int offsetLat, int offsetLng, float arcsecondsAcross);
  ~TileCell();

  void registerTile(int minLat, int minLng, Elevation maxElev);

  void distributeToTiles(const LatLng &peakLocation, Elevation elevation,
                           float isolationKm);

  ConcurrentIsolationResults *findBucket(int mMinLat, int mMinLng);
  
private:
  float maxDistanceToCell(const LatLng &point) const;
  Lock mLock;
  int mArcsecondsAcross;
  ConcurrentIsolationResults *mBucket;
  LatLng mTopLeft;
  LatLng mBottomRight;
  TileCell *mSmaller;
  TileCell *mBigger;
  Elevation mMaxElev = INT16_MIN;
  void checkAndSplit();
  bool isLeaf();
};

#endif // _TILE_CELL_H_
