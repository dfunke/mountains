#include "tile_cell.h"
#include "spherical_math_util.h"

#include "../lock.h"
#include "../latlng.h"

#include <cmath>
#include "assert.h"
#include <algorithm>

using std::floor;

TileCell::TileCell(int minLat, int minLng, int offsetLat, int offsetLng) {
    // build tree
    mBottomRight = LatLng(minLat, minLng);
    mTopLeft = LatLng(minLat + offsetLat, minLng + offsetLng);
   
    if (offsetLat ==1 && offsetLng == 1) {
        smaller = nullptr;
        bigger = nullptr;
        return;
    }
    if (offsetLat > offsetLng) {
        smaller = new TileCell(minLat, minLng, offsetLat / 2 + (offsetLat % 2), offsetLng);
        bigger = new TileCell(minLat + offsetLat / 2 + (offsetLat % 2), minLng, offsetLat / 2, offsetLng);
    } else {
        smaller = new TileCell(minLat, minLng, offsetLat, offsetLng / 2 + (offsetLng % 2));
        bigger = new TileCell(minLat, minLng + offsetLng / 2 + (offsetLng % 2), offsetLat, offsetLng / 2);
    }
}

TileCell::~TileCell() {
    if (smaller != nullptr) {
        delete smaller;
        delete bigger;
    }
}

void TileCell::insert(int minLat, int minLng, Elevation maxElev, ConcurrentIsolationResults*bucket) {
    mLock.lock();
    if (maxElev > mMaxElev) {
        mMaxElev = maxElev;
    }
    mLock.unlock();
    if (smaller != nullptr) {
        // Not a note, check if it needs to go in smaller or bigger
        if ((int) floor(smaller->mBottomRight.latitude()) <= minLat 
        && (int) floor(smaller->mBottomRight.longitude()) <= minLng
        && (int) floor(smaller->mTopLeft.latitude()) > minLat
        && (int) floor(smaller->mTopLeft.longitude()) > minLng) {
            smaller->insert(minLat, minLng, maxElev, bucket);
        } else {
            bigger->insert(minLat, minLng, maxElev, bucket);
        }
    } else {
        mBucket = bucket;
    }
}

void TileCell::distributeToBuckets(const LatLng &peakLocation, int elevation, const LatLng &higherLocation, float *isolationKm) {
    if (elevation > this->mMaxElev) {
        return;
    }
    if (smaller == nullptr) {
        if (mBucket == nullptr) {
            return;
        }
        if (*isolationKm < 0) {
            // trying to add a not found peak in bucket of his tile
            //if (floor(peakLocation.latitude()) == mBottomRight.latitude() && floor(peakLocation.longitude()) == mBottomRight.longitude()) {
            //    return;
            //}
            *isolationKm = maxDistanceToCell(peakLocation) / 1000;
        }
        mBucket->addResult(peakLocation, elevation, higherLocation, *isolationKm);
        return;
    }
    if (elevation < smaller->mMaxElev && elevation < bigger->mMaxElev) {
        float shortestDistanceToSmaller = shortestDistanceToQuadrillateral(&peakLocation, smaller->mTopLeft, smaller->mBottomRight) / 1000;
        float shortestDistanceToBigger = shortestDistanceToQuadrillateral(&peakLocation, bigger->mTopLeft, bigger->mBottomRight) / 1000;
        if (shortestDistanceToSmaller < shortestDistanceToBigger) {
            // go first in smaller
            if (*isolationKm < 0 || *isolationKm > shortestDistanceToSmaller) {
                smaller->distributeToBuckets(peakLocation, elevation, higherLocation, isolationKm);
            }
            if (*isolationKm < 0 || *isolationKm > shortestDistanceToBigger) {
                bigger->distributeToBuckets(peakLocation, elevation, higherLocation, isolationKm);
            }
        } else {
            // go first in bigger
            if (*isolationKm < 0 || *isolationKm > shortestDistanceToBigger) {
                bigger->distributeToBuckets(peakLocation, elevation, higherLocation, isolationKm);
            }
            if (*isolationKm < 0 || *isolationKm > shortestDistanceToSmaller) {
                smaller->distributeToBuckets(peakLocation, elevation, higherLocation, isolationKm);
            }
        }

    } else if (elevation < smaller->mMaxElev) {
        float shortestDistanceToSmaller = shortestDistanceToQuadrillateral(&peakLocation, smaller->mTopLeft, smaller->mBottomRight) / 1000;
        if (*isolationKm < 0 || *isolationKm > shortestDistanceToSmaller) {
            smaller->distributeToBuckets(peakLocation, elevation, higherLocation, isolationKm);
        }
    } else if (elevation < bigger->mMaxElev) {
        float shortestDistanceToBigger = shortestDistanceToQuadrillateral(&peakLocation, bigger->mTopLeft, bigger->mBottomRight) / 1000;
        if (*isolationKm < 0 || *isolationKm > shortestDistanceToBigger) {
            bigger->distributeToBuckets(peakLocation, elevation, higherLocation, isolationKm);
        }
    }
}

float TileCell::maxDistanceToCell(const LatLng &peak) const {
    return shortestDistanceToQuadrillateral(&peak, mTopLeft, mBottomRight) + 2*searchDistance(&mTopLeft, mBottomRight);
}

ConcurrentIsolationResults *TileCell::findBucket(int minLat, int minLng) {
    if (smaller != nullptr) {
        // Not a note, check if it needs to go in smaller or bigger
        if ((int)smaller->mBottomRight.latitude() <= minLat 
        && (int)smaller->mBottomRight.longitude() <= minLng
        && (int)smaller->mTopLeft.latitude() > minLat
        && (int)smaller->mTopLeft.longitude() > minLng) {
            return smaller->findBucket(minLat, minLng);
        } else {
            return bigger->findBucket(minLat, minLng);
        }
    } else {
        return mBucket;
    }
}
