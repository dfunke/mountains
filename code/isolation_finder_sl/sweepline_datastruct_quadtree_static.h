#ifndef _SWEEPLINE_DATASTRUCT_QUADTREE_STATIC_H__
#define _SWEEPLINE_DATASTRUCT_QUADTREE_STATIC_H__

#include "spherical_math_util.h"
#include "isolation_finder_sl.h"
#include "sweepline_primitives.h"
#include "sweepline_datastruct.h"

#include "../tile_cache.h"

#include <bits/stdc++.h>
#include <set>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm> // std::find_if
#include <vector>    // std::vector
#include <functional>

//#define CELL_VECTOR_SIZE 256

template <std::size_t N>
struct Leaf
{
   SlEvent *events[N];
   std::size_t activeCounter = 0;
};

struct Quad
{
   float mMaxLat, mMinLat, mMaxLng, mMinLng, minDistanceToPoint;
   int idx, counterIdx;
   void set(int idx, int counterIdx, float minLat, float maxLat, float minLng, float maxLng, float minDistanceToPoint)
   {
      this->idx = idx;
      this->counterIdx = counterIdx;
      this->mMinLat = minLat;
      this->mMaxLat = maxLat;
      this->mMinLng = minLng;
      this->mMaxLng = maxLng;
      this->minDistanceToPoint = minDistanceToPoint;
   }
};

template <std::size_t LeafSize>
class SweeplineDatastructQuadtreeStatic : public SweeplineDatastruct
{
public:
   // Points will be inserted at the given level of the tree.
   // The space used by the tree is quadradic in max_level.
   explicit SweeplineDatastructQuadtreeStatic(float latMin, float latMax, float lngMin, float lngMax, std::size_t maxPoints, std::function<Offsets (float lat, float lng)> toOffsets)
   {
      this->mToOffsets = toOffsets;
      basicSetup(latMin, latMax, lngMin, lngMax, maxPoints);
   }

   ~SweeplineDatastructQuadtreeStatic()
   {
      //delete[] mNodeCounts;
      delete[] mNodeOccupied;
      delete[] mCells;
   }

   // Insert the given point to the quadtree.
   // A copy is not made; the point must live as long as the quadtree.
   void insert(SlEvent *n) override
   {
      assert (n != nullptr);
      float lat = n->latitude();
      float lng = n->longitude();
      int level = 0;
      float left = this->mMinLng;
      float right = this->mMaxLng;
      float top = this->mMaxLat;
      float bottom = this->mMinLat;
      int index = 0;
      int counterIndex = 0;
      int bucket = mCellsSize;
      int counterBucket = (mCellsSize - 1) / 3;
      while (level < mMaxLevel)
      {
         // Add to node counter
         //mNodeCounts[counterIndex] = true;
         mNodeOccupied[counterIndex] = true;
         --counterBucket;
         ++counterIndex;
         const float mid_lng = (left + right) / 2;
         const float mid_lat = (top + bottom) / 2;
         const int latSmaller = !(lat > mid_lat);
         const int lngSmaller = !(lng > mid_lng);
         const int bucketMultiplyer = latSmaller * 2 + (1 - lngSmaller);
         bucket >>= 2;
         counterBucket >>= 2;
         index += bucket * bucketMultiplyer;
         counterIndex += counterBucket * bucketMultiplyer;
         bottom = (1 - latSmaller) * mid_lat + latSmaller * bottom;
         top = latSmaller * mid_lat + (1 - latSmaller) * top;
         left = (1 - lngSmaller) * mid_lng + lngSmaller * left;
         right = lngSmaller * mid_lng + (1 - lngSmaller) * right;
         ++level;
         assert(top >= lat);
         assert(bottom <= lat);
         assert(left <= lng);
         assert(right >= lng);
      }
      int activeCounter = mCells[index].activeCounter;
      // In some edge-cases mor data is in one leaf.
      // Override oldest data is that happens.
      //if (mCells[index].activeCounter >= LeafSize) {
      //   std::cout << "Skipping" << std::endl;
      //}
       mCells[index].events[mCells[index].activeCounter % LeafSize] = n;
       ++mCells[index].activeCounter;
   }

   IsolationRecord calcPeak(const SlEvent *node) override
   {
      IsolationRecord ir;
      if (! mNodeOccupied[0])
      {
         return ir;
      }
      Quad root;
      root.set(0, 0, mMinLat, mMaxLat, mMinLng, mMaxLng, 0); // min distance here not important!!
      findNearest(node, root, 0, mCellsSize, (mCellsSize - 1) / 3, &ir);
      ir.distance = ir.closestHigherGround.distanceEllipsoid(*node);
      return ir;
   }

   void saveAsImage(int w, int h, int level, Offsets peak, Offsets highestElev) override {
      FILE *imageFile;
      int height = h - 1, width = w - 1;
      std::ostringstream stream;
      if (level < 10)
      {
         stream << "insertMap-00" << level << ".ppm";
      }
      else if (level < 100)
      {
         stream << "insertMap-0" << level << ".ppm";
      }
      else
      {
         stream << "insertMap-" << level << ".ppm";
      }
      imageFile = fopen(stream.str().c_str(), "wb");
      if (imageFile == NULL)
      {
         perror("ERROR: Cannot open output file");
         exit(EXIT_FAILURE);
      }

      fprintf(imageFile, "P6\n");                   // P6 filetype
      fprintf(imageFile, "%d %d\n", width, height); // dimensions
      fprintf(imageFile, "255\n");                  // Max pixel

      // unsigned char pix[width * height * 3] = {255};
      unsigned char *pix = new unsigned char[width * height * 3];
      std::fill(pix, pix + (width * height * 3), 0);

      for (int i = 0; i < mCellsSize; ++i)
      {
         for (uint j = 0; j < mCells[i].activeCounter && j < LeafSize; ++j)
         {
            Offsets o = mToOffsets(mCells[i].events[j]->latitude(), mCells[i].events[j]->longitude());
            if (o.x() >= width || o.y() >= height)
            {
               continue;
            }
            pix[((width - o.x() - 1) * width + o.y()) * 3] = 200;
            pix[((width - o.x() - 1) * width + o.y()) * 3 + 1] = 200;
            pix[((width - o.x() - 1) * width + o.y()) * 3 + 2] = 200;
         }
      }
      float len = std::max(std::abs(peak.x() - highestElev.x()), std::abs(peak.y() - highestElev.y()));
      for (int i = 0; i < len; i++) {
         float t = ((float)i)/len;
         int x = std::round(peak.x() * (1-t) + highestElev.x() * t);
         int y = std::round(peak.y() * (1-t) + highestElev.y() * t);
         for (int j = x-1; j <= x+1; j++) {
            if (j < 0 || j >= width) {
               continue;
            }
            for (int k = y-1; k <= y+1; k++) {
               if (k < 0 || k >= width) {
                  continue;
               }
               pix[((width - j-1) * width + k) * 3] = 50;
               pix[((width - j-1) * width + k) * 3 + 1] = 255;
               pix[((width - j-1) * width + k) * 3 + 2] = 50;
            }
         }
      }
      for (int i = peak.x()-4; i <= peak.x()+4; ++i) {
         if (i < 0 || i >= width) {
            continue;
         }
         for (int j = peak.y()-4; j <= peak.y()+4; ++j) {
            if (j < 0 || j >= height) {
               continue;
            }
            pix[((width - i - 1) * width + j) * 3] = 255;
            pix[((width - i - 1) * width + j) * 3 + 1] = 0;
            pix[((width - i - 1) * width + j) * 3 + 2] = 0;
         }
      }
      for (int i = highestElev.x()-2; i <= highestElev.x()+2; ++i) {
         if (i < 0 || i >= width) {
            continue;
         }
         for (int j = highestElev.y()-2; j <= highestElev.y() +2; ++j) {
            if (j < 0 || j >= height) {
               continue;
            }
            pix[((width-i-1) * width + j) * 3] = 50;
            pix[((width-i-1) * width + j) * 3 + 1] = 255;
            pix[((width-i-1) * width + j) * 3 + 2] = 50;
         }
      }

      fwrite(pix, 1, 3 * width * height, imageFile);
      fclose(imageFile);
   }

   void saveHeatMap(int level) override
   {
      FILE *imageFile;
      int cellWIdth = sqrt(mCellsSize);
      int height = cellWIdth, width = cellWIdth;
      std::ostringstream stream;
      if (level < 10)
      {
         stream << "heatMap-00" << level << ".ppm";
      }
      else if (level < 100)
      {
         stream << "heatMap-0" << level << ".ppm";
      }
      else
      {
         stream << "heatMap-" << level << ".ppm";
      }
      imageFile = fopen(stream.str().c_str(), "wb");
      if (imageFile == NULL)
      {
         perror("ERROR: Cannot open output file");
         exit(EXIT_FAILURE);
      }

      fprintf(imageFile, "P6\n");                   // P6 filetype
      fprintf(imageFile, "%d %d\n", width, height); // dimensions
      fprintf(imageFile, "255\n");                  // Max pixel

      unsigned char pix[cellWIdth * cellWIdth * 3] = {0};
      std::size_t maxCounter = 0;
      for (int i = 0; i < cellWIdth; ++i)
      {
         for (int j = 0; j < cellWIdth; ++j)
         {
            int idx = GetIndexForLatLng(mMinLat + ((i + 0.f) / cellWIdth), mMinLng + ((j + 0.f) / cellWIdth));
            float ratio = (2.f * mCells[idx].activeCounter) / (1.f * LeafSize);
         
            if (maxCounter < mCells[idx].activeCounter)
            {
               maxCounter = mCells[idx].activeCounter;
            }

            int b = std::max(0, (int)(255 * (1 - ratio)));
            int r = std::max(0, (int)(255 * (ratio - 1)));
            int g = 255 - b - r;
            pix[(i * cellWIdth + j) * 3] = r;
            pix[(i * cellWIdth + j) * 3 + 1] = g;
            pix[(i * cellWIdth + j) * 3 + 2] = b;
         }
      }
      fwrite(pix, 1, 3 * cellWIdth * cellWIdth, imageFile);
      fclose(imageFile);
      std::cout << "Max Counter: " << maxCounter << std::endl;
   }

   void saveInsertValuesMap(int w, int h, uint insertCount, int level)
   {
      FILE *imageFile;
      int height = h - 1, width = w - 1;
      std::ostringstream stream;
      if (level < 10)
      {
         stream << "insertMap-00" << level << ".ppm";
      }
      else if (level < 100)
      {
         stream << "insertMap-0" << level << ".ppm";
      }
      else
      {
         stream << "insertMap-" << level << ".ppm";
      }
      imageFile = fopen(stream.str().c_str(), "wb");
      if (imageFile == NULL)
      {
         perror("ERROR: Cannot open output file");
         exit(EXIT_FAILURE);
      }

      fprintf(imageFile, "P6\n");                   // P6 filetype
      fprintf(imageFile, "%d %d\n", width, height); // dimensions
      fprintf(imageFile, "255\n");                  // Max pixel

      // unsigned char pix[width * height * 3] = {0};
      unsigned char *pix = new unsigned char[width * height * 3];
      std::fill(pix, pix + (width * height * 3), 0);

      for (int i = 0; i < mCellsSize; ++i)
      {
         for (uint j = 0; j < mCells[i].activeCounter; ++j)
         {
            Offsets o = mToOffsets(mCells[i].events[j]->latitude(), mCells[i].events[j]->longitude());
            if (o.x() >= width || o.y() >= height)
            {
               continue;
            }
            float ratio = (2.f * mCells[i].events[j]->latitude()) / insertCount;
            int b = std::max(0, (int)(255 * (1 - ratio)));
            int r = std::max(0, (int)(255 * (ratio - 1)));
            int g = 255 - b - r;
            pix[(o.x() * width + o.y()) * 3] = r;
            pix[(o.x() * width + o.y()) * 3 + 1] = g;
            pix[(o.x() * width + o.y()) * 3 + 2] = b;
         }
      }
      fwrite(pix, 1, 3 * width * height, imageFile);
      fclose(imageFile);
   }

   std::function<Offsets(float lat, float lng)> mToOffsets;

private:
   void basicSetup(float latMin, float latMax, float lngMin, float lngMax, std::size_t maxPoints)
   {
      mMinLat = latMin;
      mMaxLat = latMax;
      mMinLng = lngMin;
      mMaxLng = lngMax;
      assert(maxPoints >= 0);
      mMaxLevel = ceil(log(maxPoints / LeafSize) / log(4));
      mCellsSize = 1 << (2 * mMaxLevel);
      // mCells.resize(1 << (2 * max_level));
      //  One fore every "branch"
      //mNodeCounts = new uint[(mCellsSize - 1) / 3];
      mNodeOccupied = new bool[(mCellsSize - 1) / 3];
      //std::fill(mNodeCounts, mNodeCounts + (mCellsSize - 1) / 3, 0);
      std::fill(mNodeOccupied, mNodeOccupied + (mCellsSize - 1) / 3, false);
      mCells = new Leaf<LeafSize>[mCellsSize];
      std::fill(mCells, mCells + mCellsSize, Leaf<LeafSize>());
   }
   
   int GetIndexForLatLng(float lat, float lng) const
   {
      int level = 0;
      float left = this->mMinLng;
      float right = this->mMaxLng;
      float top = this->mMaxLat;
      float bottom = this->mMinLat;
      int index = 0;
      int bucket = mCellsSize;
      while (level < mMaxLevel)
      {
         float mid_lng = (left + right) / 2;
         float mid_lat = (top + bottom) / 2;
         if (lat >= mid_lat)
         {
            // top
            bottom = mid_lat;
         }
         else
         {
            // bottom
            index += bucket / 2;
            top = mid_lat;
         }

         if (lng >= mid_lng)
         {
            // right
            left = mid_lng;
            index += bucket / 4;
         }
         else
         {
            // left
            right = mid_lng;
         }

         bucket >>= 2;
         ++level;
      }
      return index;
   }

   void findNearest(const SlEvent *n, const Quad p, const int level, const int bucketSize, const int counterSize, IsolationRecord *ir)
   {
      if (level == mMaxLevel)
      {
         std::size_t cellSize = mCells[p.idx].activeCounter;
         if (cellSize >= LeafSize) {
            cellSize = LeafSize;
         }
         for (uint i = 0; i < cellSize; ++i)
         {
            SlEvent *toCheck = mCells[p.idx].events[i];

            if (toCheck->equal(*n))
            {
               continue;
            }
            float distance = toCheck->distance(*n);
            if (!ir->foundHigherGround || ir->distance > distance)
            {
               ir->distance = distance;
               ir->foundHigherGround = true;
               ir->closestHigherGround = *mCells[p.idx].events[i];
            }
         }
         return;
      }

      // std::cout << "Size of this cell " << mCellSizes[p.counterIdx] << std::endl;
      //  Create 4 Quads
      const int nBucket = bucketSize >> 2;
      const int nCunterSize = (counterSize - 1) >> 2;
      Quad quads[4];

      int quadsIdx = 0;
      const float midLatitude = (p.mMaxLat + p.mMinLat) / 2;
      const float midLongitude = (p.mMaxLng + p.mMinLng) / 2;

      for (int i = 0; i < 4; ++i)
      {
         int activeCounter = mCells[p.idx + i * nBucket].activeCounter;
         if ((nCunterSize > 0 && mNodeOccupied[p.counterIdx + 1 + i * nCunterSize]) || (nCunterSize == 0 && mCells[p.idx + i * nBucket].activeCounter > 0))
         {
            const float maxLat = (1 - i / 2) * p.mMaxLat + i / 2 * midLatitude;
            const float maxLng = (1 - i % 2) * midLongitude + i % 2 * p.mMaxLng;
            const float minLat = (1 - i / 2) * midLatitude + i / 2 * p.mMinLat;
            const float minLng = (1 - i % 2) * p.mMinLng + i % 2 * midLongitude;
            const float minDistance = shortestDistanceToQuadrillateral(n, LatLng(maxLat, maxLng), LatLng(minLat, minLng));
            quads[quadsIdx].set(p.idx + i * nBucket, p.counterIdx + 1 + i * nCunterSize, minLat, maxLat, minLng, maxLng, minDistance);
            ++quadsIdx;
         }
      }
      assert(quadsIdx > 0);
      assert(quadsIdx < 5);
      // Sort by Distance
      std::sort(quads, quads + quadsIdx, [](Quad const &lq, Quad const &rq)
                { return lq.minDistanceToPoint < rq.minDistanceToPoint; });

      for (int i = 0; i < quadsIdx; ++i)
      {
         if (!ir->foundHigherGround || quads[i].minDistanceToPoint < ir->distance)
         {
             findNearest(n, quads[i], level + 1, nBucket, nCunterSize, ir);
         }
      }
   }

   float mMinLng;
   float mMaxLng;
   float mMinLat;
   float mMaxLat;

   int mMaxLevel;
   int mCellsSize;

   /*
    * amount of SlEvents in nodes of quadtree.
    * if maxLevel = 3, levels are saved as following:
    * [0 1 2 2 2 2 1 2 2 2 2 1 2 2 2 2 1 2 2 2 2]
    */
   //uint32 *mNodeCounts;
   bool *mNodeOccupied;
   Leaf<LeafSize> *mCells;
};
#endif // ifndef _SWEEPLINE_DATASTRUCT_QUADTREE_STATIC_H__
