#include "sweepline_datastruct_quadtree_dynamic.h"
#include "../isolation_finder.h"
//#include "math.h"

#include "../isolation_finder.h"

#include <vector>    // std::vector
#include <stdio.h>
#include <stdlib.h>

using std::vector;

SweeplineDatastructQuadtreeDynamic::SweeplineDatastructQuadtreeDynamic(double minLat, double maxLat, double minLng, double maxLng, float *lngDistanceScale, std::function<Offsets (float lat, float lng)> toOffsets)
{
  mLngDistanceScale = lngDistanceScale;
  mToOffsets = toOffsets;
  //this->mRoot = memoryManager.allocCell(LatLng(maxLat, maxLng), LatLng(minLat, minLng));
  int childSize = (1 << 4)-1;
  this->mRoot = memoryManager.allocCell();
  this->mRoot->content = memoryManager.allocSlEvents();
  this->mRoot->memoryManager = &memoryManager;
  // make root slightly bigger to prevent <= issues ad the edges.
  this->mRoot->topLeft = LatLng(maxLat + 0.00001, maxLng + 0.00001);
  this->mRoot->bottomRight = LatLng(minLat - 0.00001, minLng - 0.00001);
  this->mRoot->size = 0;
  //this->mRoot->presetChilds(mPresetCells + 1, childSize-1, &memoryManager);
}

SweeplineDatastructQuadtreeDynamic::~SweeplineDatastructQuadtreeDynamic() {
  delete []mPresetCells;
}

void SweeplineDatastructQuadtreeDynamic::insert(SlEvent *n)
{
  mRoot->insert(n);
}


void SweeplineDatastructQuadtreeDynamic::remove(const SlEvent *n){
  mRoot->remove(n);
}

IsolationRecord SweeplineDatastructQuadtreeDynamic::calcPeak(const SlEvent *node)
{
  IsolationRecord ir;
  float *currShortestDistance = new float;
  *currShortestDistance = MAXFLOAT;
  LatLng *shortestPoint = new LatLng();
  if (mToOffsets == nullptr) {
    mRoot->shortestDistance(node, currShortestDistance, shortestPoint);
  } else {
    mRoot->fastShortestDistance(node, currShortestDistance, shortestPoint, mLngDistanceScale, mToOffsets);
  }
  
  if (*currShortestDistance < MAXFLOAT) {
    ir.foundHigherGround = true;
    ir.closestHigherGround = *shortestPoint;
    if (mToOffsets == nullptr) {
      ir.distance = ir.closestHigherGround.distanceEllipsoid(*node);
    } else {
      ir.distance = ir.closestHigherGround.distance(*node);
    }
  }
  delete shortestPoint;
  delete currShortestDistance;
  return ir;
}

void SweeplineDatastructQuadtreeDynamic::saveAsImage(int w, int h, int level, SlEvent **allNodes, int nodeSize)
{
  /*
    FILE *imageFile;
    int height = h - 1, width = w - 1;
    std::ostringstream stream;
    if (level < 10)
    {
      stream << "step-00" << level << ".ppm";
    }
    else if (level < 100)
    {
      stream << "step-0" << level << ".ppm";
    }
    else
    {
      stream << "step-" << level << ".ppm";
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

    unsigned char pix[w * h * 3] = {0};
    int active = 0;
    for (auto it = mTree.begin(); it != mTree.end(); it++)
    {
      pix[(it->mOffsets.y() * width + it->mOffsets.x()) * 3] = 255;
      active++;
    }

    fwrite(pix, 1, 3 * w * h, imageFile);
    fclose(imageFile);

    std::ofstream foutput;
    std::ifstream finput;
    finput.open("sketch.txt");
    foutput.open("sketch.txt", std::ios::app);
    if (finput.is_open())
    {
      foutput << level * 5000 - 5000 << "," << active << "\n";
    }
    finput.close();
    foutput.close();
    */
}

void SweeplineDatastructQuadtreeDynamic::createSnapshot(int step) {
  int elements = this->mRoot->countNodes();
  int maxDepth = this->mRoot->getMaxDepth();
  int leafs = this->mRoot->countLeafs();
  printf("%d,%ld,%d,%d,%d", step, mRoot->size, maxDepth, elements, leafs);
}

void SweeplineDatastructQuadtreeDynamic::saveHeatMap(int level) {

}
