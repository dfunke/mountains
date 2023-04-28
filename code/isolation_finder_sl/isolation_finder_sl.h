/*
 * MIT License
 * 
 * Copyright (c) 2022 Nicolai Huening
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _ISOLATION_FINDER_SL_H_
#define _ISOLATION_FINDER_SL_H_

// #include "isolation_finder.h"
#include "../primitives.h"
#include "sweepline_primitives.h"
#include "concurrent_isolation_results.h"
#include "../isolation_results.h"
#include "../tile_cache.h"
#include "tile_cell.h"
#include "../tile.h"
#include "../coordinate_system.h"
#include "ilp_search_area_tree.h"

#include <memory>


class IsolationFinderSl {
public:
    explicit IsolationFinderSl(TileCache *tileCache, ILPSearchAreaTree *ilpSearchTree, int minLat, int minLng, FileFormat format);
    ~IsolationFinderSl();
    uint fillPeakBuckets(float minIsolationKm);
    IsolationResults run(float minIsolationKm);
    
    bool nullPtrTile = false;
private:
  
  SlEvent *mEventQueue;
  
  ILPSearchAreaTree *mIlpSearchTree;

  std::size_t currSize = 0;
  TileCache *mTileCache;
  FileFormat mFormat;
  int mMinLat, mMinLng, mWidth, mHeight, skipVal;
  float mMaxLat, mMaxLng;

  // An array with one entry per row of the tile.
  // Each entry is a scale factor in [0, 1] that should be multiplied by
  // any distance in the longitude (x) direction.  This compensates for
  // lines of longitude getting closer as latitude increases.  The value
  // of the factor is the cosine of the latitude of the row.
  float *mLngDistanceScale;
  CoordinateSystem *mCoordinateSystem;

  uint setup(const Tile* tile, const ConcurrentIsolationResults* prevResults);

  void addPeakToBucket(const LatLng &peakLocation, int elevation, float isolationKm);

  IsolationResults runSweepline(float mMinIsolationKm, bool fast);

  void run(int latMin, int lngMin, float mMinIsolationKm, ConcurrentIsolationResults* prevResults);

  Offsets toOffsets(float latitude, float longitude) const;
  
};
#endif  // _ISOLATION_FINDER_SL_H_
