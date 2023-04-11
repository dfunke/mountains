/*
 * MIT License
 *
 * Copyright (c) 2017 Andrew Kirmse
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

#ifndef _SWEEPLINE_DATASTRUCT_QUADTREE_DYNAMIC_H__
#define _SWEEPLINE_DATASTRUCT_QUADTREE_DYNAMIC_H__

#include "isolation_finder_sl.h"
#include "sweepline_primitives.h"
#include "sweepline_datastruct.h"
#include "cell.h"
#include "cell_memory_manager.h"

#include "../tile_cache.h"
#include "../math_util.h"

#include <functional>
#include <vector>

class SweeplineDatastructQuadtreeDynamic : public SweeplineDatastruct
{
public:
  SweeplineDatastructQuadtreeDynamic(double minLat, double maxLat, double minLng, double maxLng, float *latDistanceScale, std::function<Offsets (float lat, float lng)> toOffsets);
  ~SweeplineDatastructQuadtreeDynamic();

  // Insert the given point to the quadtree.
  // A copy is not made; the point must live as long as the quadtree.
  void insert(SlEvent *n) override;

  void remove(const SlEvent *n) override;

  IsolationRecord calcPeak(const SlEvent *node) override;

  void saveAsImage(int w, int h, int level, SlEvent **allNodes, int nodeSize);

  void saveAsImage(int w, int h, int level, Offsets peak, Offsets highestElev) override {}

  void createSnapshot(int staps);

  void saveHeatMap(int level) override;

private:
  // Find cell that would contain point
  Cell *mPresetCells = nullptr;
  bool fast = false;
  std::function<Offsets (float lat, float lng)> mToOffsets;
  float *mLngDistanceScale;
  CellMemoryManager memoryManager;
  uint maxContent;
  Cell *mRoot;
};

#endif // ifndef _SWEEPLINE_DATASTRUCT_QUADTREE_DYNAMIC_H__
