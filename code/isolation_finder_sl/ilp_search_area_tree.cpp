#include "ilp_search_area_tree.h"
#include "sweepline_datastruct.h"
#include "sweepline_datastruct_quadtree_static.h"
#include "sweepline_primitives.h"
#include "tile_cell.h"

ILPSearchAreaTree::ILPSearchAreaTree(int minLat, int minLng, int offsetLat,
                                     int offsetLng) {
  mRoot = new TileCell(minLat, minLng, offsetLat, offsetLng);
  mMinLat = minLat;
  mMinLng = minLng;
  mMaxLat = minLat + offsetLat;
  mMaxLng = minLng + offsetLng;
}

ILPSearchAreaTree::~ILPSearchAreaTree() { delete mRoot; }

void ILPSearchAreaTree::distributeToTiles(const LatLng &peakLocation,
                                          int elevation, float *isolationKm) {
  if (*isolationKm > 0) {
    mRoot->distributeToTiles(peakLocation, elevation, *isolationKm);
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
  mTileEvents.push_back(
      SlEvent(maxElev, SlEventType::ADD,
              LatLng(static_cast<float>(minLat), static_cast<float>(minLng))));
  mRoot->registerTile(minLat, minLng, maxElev);
}

IsolationResult ILPSearchAreaTree::proccessUnbound() {
  SlEvent *queue =
      new SlEvent[mUnboundResults.mResults.size() + mTileEvents.size()];
  int idx = 0;
  for (auto &res : mUnboundResults.mResults) {
    queue[idx].initialize(res.peakElevation, SlEventType::PEAK, res.peak,
                          Offsets());
    idx++;
  }
  for (auto &tile : mTileEvents) {
    queue[idx] = tile;
    idx++;
  }
  std::stable_sort(queue, queue + idx,
                   [](SlEvent const &lhs, SlEvent const &rhs) {
                     return lhs.getElev() > rhs.getElev();
                   });
  SweeplineDatastruct *sld = new SweeplineDatastructQuadtreeStatic<16>(
      mMinLat, mMaxLat, mMinLng, mMaxLng,
      (mMaxLat - mMinLat) * (mMaxLng - mMinLng),
      [=](float lat, float lng) { return Offsets(); });
  SlEvent highPoint = queue[0];
  // run sweepline
  for (int i = 0; i < idx; i++) {
    switch (queue[i].getType()) {
    case PEAK: {
      IsolationRecord nearestTile = sld->calcPeak(queue + i);
      if (!nearestTile.foundHigherGround) {
        // std::cout << "Did not find higher ground for: " <<
        // queue[i].latitude() << ", " << queue[i].longitude() << std::endl;
        continue;
      }
      LatLng topLeft(std::floor(nearestTile.closestHigherGround.latitude()) + 1,
                     std::floor(nearestTile.closestHigherGround.longitude()) +
                         1);
      LatLng bottomRight(
          std::floor(nearestTile.closestHigherGround.latitude()),
          std::floor(nearestTile.closestHigherGround.longitude()));
      mRoot->distributeToTiles(
          queue[i], queue[i].getElev(),
          (nearestTile.distance + searchDistance(&topLeft, bottomRight)) /
              1000);
    } break;
    case ADD:
      sld->insert(queue + i);
      break;
    case REMOVE:
      sld->remove(queue + i);
      break;
    }
  }
  delete [] queue;
  delete sld;
  IsolationResult highPointRes;
  highPointRes.peak = highPoint;
  highPointRes.peakElevation = highPoint.getElev();
  highPointRes.isolationKm = -1;
  return highPointRes;
}
