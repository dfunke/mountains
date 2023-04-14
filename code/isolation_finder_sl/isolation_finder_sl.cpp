
// #include "isolation_finder.h"
#include "isolation_finder_sl.h"
// #include "easylogging++.h"
// #include "math_util.h"
#include "../ThreadPool.h"
#include "../file_format.h"
#include "../isolation_results.h"
#include "../peak_finder.h"
#include "../tile_cache.h"
#include "concurrent_isolation_results.h"
#include "ilp_search_area_tree.h"
#include "sweepline_datastruct.h"
#include "sweepline_datastruct_naive.h"
#include "sweepline_datastruct_quadtree_dynamic.h"
#include "sweepline_datastruct_quadtree_static.h"

#include "ips4o.hpp"

#include <algorithm>
#include <assert.h>
#include <stdlib.h>

#include <chrono>
#include <iostream>

#include <unordered_set>

using std::pair;
using std::vector;
static const std::size_t DEM1_LEAF_SIZE =
    3166; // 12967201 points in DEM1, leafeSize 3166 will create 6 level.
static const std::size_t DEM3_LEAF_SIZE = 370; //  353

bool compare(std::pair<SlEvent, int> const &el,
             std::pair<SlEvent, int> const &er) {
  return el.first.getElev() < er.first.getElev();
}

template <typename T>
using maxheap = std::priority_queue<T, vector<T>, decltype(&compare)>;

IsolationFinderSl::IsolationFinderSl(TileCache *tileCache,
                                     ILPSearchAreaTree *ilpSearchTree,
                                     int minLat, int minLng,
                                     FileFormat format) {
  mCoordinateSystem = format.coordinateSystemForOrigin(minLat, minLng);
  mFormat = format;
  mTileCache = tileCache;
  mIlpSearchTree = ilpSearchTree;
  mMinLat = minLat;
  mMinLng = minLng;
}

IsolationFinderSl::~IsolationFinderSl() {
  delete[] mEventQueue;
  delete mCoordinateSystem;
}

inline Elevation getMinSorrounding(const Tile *tile, const Offsets offset,
                                   int skipVal) {
  Elevation minSorrounding = tile->get(offset);
  if (minSorrounding == tile->NODATA_ELEVATION) {
    // Ignore remaining no data elevation
    return tile->NODATA_ELEVATION;
  }
  Elevation elev = tile->get(Offsets(offset.x() + skipVal, offset.y()));
  if (elev < minSorrounding && elev != tile->NODATA_ELEVATION) {
    minSorrounding = elev;
  }
  elev = tile->get(Offsets(offset.x(), offset.y() + skipVal));
  if (elev < minSorrounding && elev != tile->NODATA_ELEVATION) {
    minSorrounding = elev;
  }
  elev = tile->get(Offsets(offset.x() - skipVal, offset.y()));
  if (elev < minSorrounding && elev != tile->NODATA_ELEVATION) {
    minSorrounding = elev;
  }
  elev = tile->get(Offsets(offset.x(), offset.y() - skipVal));
  if (elev < minSorrounding && elev != tile->NODATA_ELEVATION) {
    minSorrounding = elev;
  }
  return minSorrounding;
}

void IsolationFinderSl::setup(const Tile *tile,
                              const ConcurrentIsolationResults *prevResults) {
  // Tiles overlap one pixel on all sides
  int width = tile->width();
  int height = tile->height();

  int idx = 0;
  vector<Offsets> peaks;
  if (prevResults != nullptr) {
    skipVal = 1;
    mEventQueue =
        new SlEvent[((width - 1) / skipVal) * ((height - 2) * 2 / skipVal) +
                    prevResults->size()];
  } else {
    // skipVal = tile->width() / (tile->metersPerSample() * 4);
    //  Reduce to
    skipVal = mFormat.inMemorySamplesAcross() / (mFormat.degreesAcross() * 500);
    // skipVal = 3;
    PeakFinder pfinder(tile);
    peaks = pfinder.findPeaks();
    // Get distance-scale from tile
    mLngDistanceScale = (float *)malloc(sizeof(float) * tile->height());
    for (int y = skipVal; y < tile->height() - skipVal; y += skipVal) {
      for (int y = 0; y < tile->height(); ++y) {
        LatLng point = mCoordinateSystem->getLatLng(Offsets(0, y));
        mLngDistanceScale[y] = cosf(degToRad(point.latitude()));
      }
    }
    int queueSize =
        ((width - 1) / skipVal) * ((height - 1) / skipVal) * 2 + peaks.size();
    mEventQueue = new SlEvent[queueSize];
  }

  // Add peaks first, to guarantee > all inserted samples.
  if (prevResults != nullptr) {
    for (auto it = prevResults->mResults.cbegin();
         it < prevResults->mResults.cend(); ++it) {
      mEventQueue[idx].initialize(it->peakElevation, SlEventType::PEAK,
                                  it->peak, Offsets(0, 0));
      ++idx;
    }
  } else {
    // Add peaks
    for (Offsets &peak : peaks) {
      mEventQueue[idx].initialize(tile->get(peak), SlEventType::PEAK,
                                  mCoordinateSystem->getLatLng(peak), peak);
      ++idx;
    }
  }
  int peakIdx = idx;

  if (peakIdx == 0) {
    currSize = 0;
    return;
  }

  // On SRTM 1 pixel overlapp
  for (int j = 0; j < height - skipVal; j += skipVal) {
    for (int i = 0; i < width - skipVal; i += skipVal) {
      Offsets currentOffsets(i, j);
      // skip if smaller than minPeakElev
      /*
      if (tile->get(currentOffsets) < minPeakElev) {
          continue;
      }
      */

      // Get minimum of sorounding elevations

      Elevation minSorrounding =
          getMinSorrounding(tile, currentOffsets, skipVal);

      if (minSorrounding < tile->get(currentOffsets)) {
        mEventQueue[idx].initialize(
            tile->get(currentOffsets), SlEventType::ADD,
            mCoordinateSystem->getLatLng(currentOffsets), currentOffsets);
        ++idx;
        mEventQueue[idx].initialize(
            minSorrounding, SlEventType::REMOVE,
            mCoordinateSystem->getLatLng(currentOffsets), currentOffsets);
        ++idx;
      }
    }
  }

  // sort peaks
  ips4o::sort(mEventQueue, mEventQueue + peakIdx - 1,
              [](SlEvent const &lhs, SlEvent const &rhs) {
                return lhs.getElev() > rhs.getElev();
              });
  // sort events
  ips4o::sort(mEventQueue + peakIdx, mEventQueue + idx,
              [](SlEvent const &lhs, SlEvent const &rhs) {
                return lhs.getElev() > rhs.getElev();
              });
  // Merge peaks and events
  std::inplace_merge(mEventQueue, mEventQueue + peakIdx, mEventQueue + idx,
                     [](SlEvent const &lhs, SlEvent const &rhs) {
                       return lhs.getElev() > rhs.getElev();
                     });

  // Sort using height value
  // std::stable_sort(mEventQueue, mEventQueue + idx,
  // std::stable_sort(mEventQueue, mEventQueue + idx,
  //                 [](SlEvent const &lhs, SlEvent const &rhs) {
  //                   return lhs.getElev() > rhs.getElev();
  //                 });

  // saveTileAsImage(tile);
  currSize = idx;
}

IsolationResults IsolationFinderSl::runSweepline(float mMinIsolationKm,
                                                 bool fast) {
  // int counter = 0;
  // int level = 1;
  IsolationResults results;
  // SweeplineDatastruct *sld = new SweeplineDatastructNaive(1200, 1200,
  // LatLng(maxLat, maxLng), LatLng(minLat, minLng));
  SweeplineDatastruct *sld;
  if (fast) {
    sld = new SweeplineDatastructQuadtreeDynamic(
        mMinLat, mMaxLat, mMinLng, mMaxLng, mLngDistanceScale,
        [=](float lat, float lng) { return this->toOffsets(lat, lng); });
  } else {
    // sld = new SweeplineDatastructNaive(1200, 1200, LatLng(mMaxLat, mMaxLng),
    // LatLng(mMinLat, mMinLng));
    //  sld = new SweeplineDatastructQuadtreeDynamic(mMinLat, mMaxLat, mMinLng,
    //  mMaxLng);
    sld = new SweeplineDatastructQuadtreeDynamic(
        mMinLat, mMaxLat, mMinLng, mMaxLng, mLngDistanceScale, nullptr);
  }
  for (std::size_t i = 0; i < currSize; ++i) {
    SlEvent *node = &mEventQueue[i];
    switch (node->getType()) {
    case PEAK: {
      LatLng peakLoc = *node;
      IsolationRecord record = sld->calcPeak(node);

      if (record.foundHigherGround) {
        float distance = record.distance;
        if (distance > 0) {
          distance = distance / 1000;
        }
        if (fast) {
          if (distance > mMinIsolationKm) {
            addPeakToBucket(peakLoc, node->getElev(), distance);
            // std::cout << peakLoc.latitude() << "," << peakLoc.longitude() <<
            // "," << node->getElev() << "," <<
            // record.closestHigherGround.latitude() << "," <<
            // record.closestHigherGround.longitude() << "," << distance <<
            // std::endl;
          }
        } else {
          results.addResult(peakLoc, node->getElev(),
                            record.closestHigherGround, distance);
          // if (distance > mMinIsolationKm) {
          //     sld->saveAsImage(1200, 1200, level++,
          //     toOffsets(node->latitude(), node->longitude()),
          //     toOffsets(record.closestHigherGround.latitude(),
          //     record.closestHigherGround.longitude()));
          // }
        }
      } else {
        // std::cout << peakLoc.latitude() << "," << peakLoc.longitude() << ","
        // << node->getElev() << "," << record.closestHigherGround.latitude() <<
        // "," << record.closestHigherGround.longitude() << "," << -1 <<
        // std::endl;
        if (fast) {
          addPeakToBucket(peakLoc, node->getElev(), -1);
        }
        // Add not found higher ground peaks just to buckets, not to final
        // result.
      }
    } break;
    case ADD:
      sld->insert(node);
      break;
    case REMOVE:
      sld->remove(node);
      break;
    }
    // if ((mFormat == FileFormat::HGT_DEM3 && counter % 3333 == 0) || (mFormat
    // == FileFormat::HGT_DEM1 && counter % 10000 == 0) )
    //{
    //     //sld->saveAsImage(1200, 1200, ++level, mEventQueue, currSize);
    //     if (!fast) {
    //         sld->saveHeatMap(level);
    //         level++;
    //     }
    // }
    // counter++;
  }
  delete sld;
  delete[] mEventQueue;
  currSize = 0;
  return results;
}

void IsolationFinderSl::addPeakToBucket(const LatLng &peakLocation,
                                        int elevation, float isolationKm) {
  float *isolationPoint = new float;
  *isolationPoint = isolationKm;
  mIlpSearchTree->distributeToTiles(peakLocation, elevation, isolationPoint);
  delete isolationPoint;
}

void IsolationFinderSl::fillPeakBuckets(float mMinIsolationKm) {
  Tile *tile =
      mTileCache->loadWithoutCaching(mMinLat, mMinLng, *mCoordinateSystem);
  if (tile == nullptr) {
    nullPtrTile = true;
    return;
  }
  mIlpSearchTree->registerTile(mMinLat, mMinLng, tile->maxElevation());
  mMaxLat = mMinLat + mFormat.degreesAcross();
  mMaxLng = mMinLng + mFormat.degreesAcross();
  mWidth = tile->width();
  mHeight = tile->height();
  setup(tile, nullptr);
  delete tile;
  runSweepline(mMinIsolationKm, true);
  return;
}

IsolationResults IsolationFinderSl::run(float mMinIsolationKm) {
  Tile *tile =
      mTileCache->loadWithoutCaching(mMinLat, mMinLng, *mCoordinateSystem);
  ConcurrentIsolationResults *results =
      mIlpSearchTree->findBucket(mMinLat, mMinLng);
  setup(tile, results);
  delete results;
  delete tile;
  return runSweepline(mMinIsolationKm, false);
}

Offsets IsolationFinderSl::toOffsets(float latitude, float longitude) const {
  // Samples are edge-centered; OK to go half a pixel outside a corner
  int x = (int)((longitude - mMinLng) * (mWidth - 1) + 0.5);
  int y = (int)((mMaxLat - latitude) * (mHeight - 1) + 0.5);

  return Offsets(x, y);
}
