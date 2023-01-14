#include "ilp_search_area_tree.h"
#include "tile_cell.h"

ILPSearchAreaTree::ILPSearchAreaTree(int minLat, int minLng, int offsetLat,
                                     int offsetLng) {
  mRoot = new TileCell(minLat, minLng, offsetLat, offsetLng);
}

ILPSearchAreaTree::~ILPSearchAreaTree() { delete mRoot; }

void ILPSearchAreaTree::distributeToTiles(const LatLng &peakLocation,
                                          int elevation,
                                          float *isolationKm) {
  if (*isolationKm > 0) {
    mRoot->distributeToTiles(peakLocation, elevation,
                               isolationKm);
  } else {
    // Add first to unbound and proccess at the end.
    mUnboundResults.addResult(peakLocation, elevation, *isolationKm);
  }
}

ConcurrentIsolationResults *ILPSearchAreaTree::findBucket(int minLat,
                                                          int minLng) {
  return mRoot->findBucket(minLat, minLng);
}

void ILPSearchAreaTree::registerTile(int minLat, int minLng,
                                     Elevation maxElev) {
  mRoot->registerTile(minLat, minLng, maxElev);
}

void ILPSearchAreaTree::proccessUnbound() {
  for (auto &res : mUnboundResults.mResults) {
    float *findIsolation = new float();
    *findIsolation = -1;
    mRoot->findUpperBoundAndDistributeToTiles(res.peak, res.peakElevation, findIsolation);
    delete findIsolation;
  }
}
