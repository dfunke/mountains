#include "ilp_search_area_tree.h"
#include "sweepline_datastruct.h"
#include "sweepline_datastruct_kd.h"
#include "sweepline_primitives.h"
#include "spherical_math_util.h"
#include "tile_cell.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

ILPSearchAreaTree::ILPSearchAreaTree(int minLat, int minLng, int offsetLat,
                                     int offsetLng, float degreeAcross) {
  mRoot = new TileCell(minLat, minLng, offsetLat, offsetLng, degreeAcross);
  mMinLat = minLat;
  mMinLng = minLng;
  mMaxLat = minLat + offsetLat;
  mMaxLng = minLng + offsetLng;
  mDegreeAccross = degreeAcross;
  int tWidth = static_cast<int>(std::ceil(offsetLng/degreeAcross));
  int tHeight = static_cast<int>(std::ceil(offsetLat/degreeAcross));
  Elevation *samples = (Elevation*)malloc(tWidth * tHeight * sizeof(Elevation));
  std::fill(samples, samples + tWidth * tHeight, Tile::NODATA_ELEVATION);
  mTileRes = new Tile(tWidth, tHeight, samples, FileFormat::Value::HGT1);
}

ILPSearchAreaTree::~ILPSearchAreaTree() {
  delete mRoot; 
  delete mTileRes;
}

void ILPSearchAreaTree::distributeToTiles(const LatLng &peakLocation,
                                          Elevation elevation, float *isolationKm) {
  if (*isolationKm > 0) {
    mRoot->distributeToTiles(peakLocation, elevation, *isolationKm);
  } else {
    // Add first to unbound and proccess at the end.
    int tx = static_cast<int>((peakLocation.longitude() - mMinLng)/mDegreeAccross);
    int ty = static_cast<int>((peakLocation.latitude() - mMinLat)/mDegreeAccross);
    mTileRes->set(tx, ty, elevation);
    mUnboundResults.addResult(peakLocation, elevation, *isolationKm);
  }
}

ConcurrentIsolationResults *ILPSearchAreaTree::findBucket(int minLat,
                                                          int minLng) {
  return mRoot->findBucket(minLat, minLng);
}

void ILPSearchAreaTree::registerTile(int minLat, int minLng,
                                     Elevation maxElev) {
  //mTileEvents.push_back(
  //    SlEvent(maxElev, SlEventType::ADD,
  //            LatLng(static_cast<float>(minLat), static_cast<float>(minLng))));
  mRoot->registerTile(minLat, minLng, maxElev);
}

IsolationResult ILPSearchAreaTree::proccessUnbound() {
  SlEvent *queue =
      new SlEvent[3*mUnboundResults.mResults.size()];
  int idx = 0;
  SlEvent highPoint;
  Elevation highestElev = -32768;
  for (auto &res : mUnboundResults.mResults) {
    queue[idx].initialize(res.peakElevation, SlEventType::PEAK, res.peak,
                          Offsets());
    if (res.peakElevation > highestElev) {
      highPoint = queue[idx];
      highestElev = res.peakElevation;
    }
    idx++;
  }
  for (auto &res : mUnboundResults.mResults) {
    queue[idx].initialize(res.peakElevation, SlEventType::ADD, res.peak,
                          Offsets());
    idx++;
    int tx = static_cast<int>((res.peak.longitude() - mMinLng)/mDegreeAccross);
    int ty = static_cast<int>((res.peak.latitude() - mMinLat)/mDegreeAccross);
    queue[idx].initialize(mTileRes->minSorrounding(tx, ty, 1), SlEventType::REMOVE, res.peak, Offsets());
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
  SweeplineDatastruct *sld = new SweeplineDatastructKD(
    mMinLat, mMaxLat, mMinLng, mMaxLng, nullptr, nullptr
  );
  //SweeplineDatastruct *sld = new SweeplineDatastructQuadtreeStatic<16>(
  //    mMinLat, mMaxLat, mMinLng, mMaxLng,
  //    (mMaxLat - mMinLat) * (mMaxLng - mMinLng),
  //    [=](float lat, float lng) { return Offsets(); });
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
      mRoot->distributeToTiles(
          queue[i], queue[i].getElev(),
          nearestTile.distance / 1000);
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
