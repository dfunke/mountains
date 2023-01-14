#ifndef _ILP_SEARCH_AREA_TREE_H_
#define _ILP_SEARCH_AREA_TREE_H_

#include "concurrent_isolation_results.h"
#include "tile_cell.h"

class ILPSearchAreaTree {
public:
  ILPSearchAreaTree(int minLat, int minLng, int offsetLat, int offsetLng);
  ~ILPSearchAreaTree();

  // Register a tile to the ilp search area tree.
  void registerTile(int minLat, int minLng, Elevation maxElev);

  // Distribute the peak to tiles which could contain a closer higher-ground
  void distributeToTiles(const LatLng &peakLocation, int elevation,
                           float *isolationKm);

  // Find the bucket for the tile represented by minLat and minLng
  ConcurrentIsolationResults *findBucket(int minLat, int minLng);

  // process all unbound peaks which
  void proccessUnbound();

private:
  TileCell *mRoot;
  // Results which have currently no upper bound.
  // This will be added in the last step.
  ConcurrentIsolationResults mUnboundResults;
};

#endif // _ILP_SEARCH_AREA_TREE_H_
