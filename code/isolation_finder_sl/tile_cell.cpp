#include "tile_cell.h"
#include "concurrent_isolation_results.h"
#include "spherical_math_util.h"

#include "../latlng.h"
#include "../lock.h"

#include "assert.h"
#include <algorithm>
#include <cmath>

using std::floor;
float EPSILON = 0.001;

TileCell::TileCell(int minLat, int minLng, int offsetLat, int offsetLng,
                   float arcsecondsAcross) {
  // build tree
  mArcsecondsAcross = arcsecondsAcross;
  mBottomRight = LatLng(minLat, minLng);
  mTopLeft = LatLng(minLat + offsetLat, minLng + offsetLng);
  mSmaller = nullptr;
  mBigger = nullptr;
  mBucket = nullptr;
}

TileCell::~TileCell() {
  if (mSmaller != nullptr) {
    delete mSmaller;
    delete mBigger;
  }
  if (mBucket != nullptr) {
    delete mBucket;
  }
}

void TileCell::registerTile(int minLat, int minLng, Elevation maxElev) {
  mLock.lock();
  if (maxElev > mMaxElev) {
    mMaxElev = maxElev;
  }
  mLock.unlock();
  if (isLeaf()) {
    return;
  }
  checkAndSplit();
  // Not a note, check if it needs to go in smaller or bigger
  if ((int)floor(mSmaller->mBottomRight.latitude()) <= minLat &&
      (int)floor(mSmaller->mBottomRight.longitude()) <= minLng &&
      (int)floor(mSmaller->mTopLeft.latitude()) > minLat &&
      (int)floor(mSmaller->mTopLeft.longitude()) > minLng) {
    mSmaller->registerTile(minLat, minLng, maxElev);
  } else {
    mBigger->registerTile(minLat, minLng, maxElev);
  }
}

void TileCell::distributeToTiles(const LatLng &peakLocation, Elevation elevation,
                                 float isolationKm) {
  if (isLeaf()) {
    if (mBucket == nullptr) {
      mLock.lock();
      // Race-condition ask aggain while locket.
      if (mBucket == nullptr) {
        mBucket = new ConcurrentIsolationResults();
      }
      mLock.unlock();
    }
    // If this tile is registered check max elev. Else just add and deal with to
    // big elevations later.
    if (mMaxElev == INT16_MIN || elevation <= mMaxElev) {
      mBucket->addResult(peakLocation, elevation, isolationKm);
    }
    return;
  }
  checkAndSplit();
  float shortestDistanceToSmaller =
      shortestDistanceToQuadrillateral(&peakLocation, mSmaller->mTopLeft,
                                       mSmaller->mBottomRight) /
      1000;
  float shortestDistanceToBigger =
      shortestDistanceToQuadrillateral(&peakLocation, mBigger->mTopLeft,
                                       mBigger->mBottomRight) /
      1000;
  if (shortestDistanceToSmaller < shortestDistanceToBigger) {
    // go first in smaller
    if (isolationKm > shortestDistanceToSmaller) {
      mSmaller->distributeToTiles(peakLocation, elevation, isolationKm);
    }
    if (isolationKm > shortestDistanceToBigger) {
      mBigger->distributeToTiles(peakLocation, elevation, isolationKm);
    }
  } else {
    // go first in bigger
    if (isolationKm > shortestDistanceToBigger) {
      mBigger->distributeToTiles(peakLocation, elevation, isolationKm);
    }
    if (isolationKm > shortestDistanceToSmaller) {
      mSmaller->distributeToTiles(peakLocation, elevation, isolationKm);
    }
  }
}

float TileCell::maxDistanceToCell(const LatLng &peak) const {
  return shortestDistanceToQuadrillateral(&peak, mTopLeft, mBottomRight) +
         searchDistance(&mTopLeft, mBottomRight);
}

ConcurrentIsolationResults *TileCell::findBucket(int minLat, int minLng) {
  if (mSmaller != nullptr) {
    // Not a note, check if it needs to go in smaller or bigger
    if ((int)mSmaller->mBottomRight.latitude() <= minLat &&
        (int)mSmaller->mBottomRight.longitude() <= minLng &&
        (int)mSmaller->mTopLeft.latitude() > minLat &&
        (int)mSmaller->mTopLeft.longitude() > minLng) {
      return mSmaller->findBucket(minLat, minLng);
    } else {
      return mBigger->findBucket(minLat, minLng);
    }
  } else {
    ConcurrentIsolationResults *b = mBucket;
    mBucket = nullptr;
    return b;
  }
}

bool TileCell::isLeaf() {
  return mTopLeft.latitude() - mBottomRight.latitude() <
             mArcsecondsAcross + EPSILON &&
         mTopLeft.longitude() - mBottomRight.longitude() <
             mArcsecondsAcross + EPSILON;
}

void TileCell::checkAndSplit() {
  if (mSmaller == nullptr) {
    int offsetLat = (int)mTopLeft.latitude() - (int)mBottomRight.latitude();
    int offsetLng = (int)mTopLeft.longitude() - (int)mBottomRight.longitude();
    // Make sure to just split at tile edges.
    // Assumes square tiles
    if (offsetLat > offsetLng) {
      int centerLat = std::floor(offsetLat / ( 2 *mArcsecondsAcross)) * mArcsecondsAcross;
      mSmaller = new TileCell(
          (int)mBottomRight.latitude(), (int)mBottomRight.longitude(),
          centerLat, offsetLng, mArcsecondsAcross);
        
      mBigger = new TileCell((int)mBottomRight.latitude() + centerLat,
                             (int)mBottomRight.longitude(), offsetLat - centerLat,
                             offsetLng, mArcsecondsAcross);
    } else {
      int centerLng = std::floor(offsetLng / ( 2 *mArcsecondsAcross)) * mArcsecondsAcross;
      mSmaller = new TileCell(
          (int)mBottomRight.latitude(), (int)mBottomRight.longitude(),
          offsetLat, centerLng, mArcsecondsAcross);
      mBigger = new TileCell((int)mBottomRight.latitude(),
                             (int)mBottomRight.longitude() + centerLng,
                             offsetLat, offsetLng - centerLng, mArcsecondsAcross);
    }
    // Split
  }
}
