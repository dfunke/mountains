#include "../primitives.h"
#include "../tile_cache.h"

#include "sweepline_datastruct_naive.h"
#include "isolation_finder_sl.h"

#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>    // std::find_if
#include <vector>       // std::vector

using std::vector;

SweeplineDatastructNaive::SweeplineDatastructNaive(int width, int height, LatLng topLeft,  LatLng bottomRight) {
    this->pWidth = width;
    this->pHeight = height;
    this->nodes = new SlEvent*[pWidth*pHeight];
    this->mSamples = new Elevation[pWidth*pHeight];
    // set mSamples to zero
    for (int i = 0; i < pWidth*pHeight; i++) {
      mSamples[i] = 0;
    }
    this->mTopLeft = topLeft;
    this->mBottomRight = bottomRight;
}

SweeplineDatastructNaive::~SweeplineDatastructNaive() {
    delete [] nodes;
    delete [] mSamples;
}

void SweeplineDatastructNaive::insert(SlEvent* node) {
    Offsets o = getOffsets(node);
    nodes[o.y() * pWidth + o.x()] = node;
    mSamples[o.y() * pWidth + o.x()] = 1;
}

void SweeplineDatastructNaive::remove(const SlEvent* node) {
    Offsets o = getOffsets(node);
    //std::remove(nodes.begin(), nodes.end(), node);
    nodes[o.y() * pWidth + o.x()] = nullptr;
    mSamples[o.y() * pWidth + o.x()] = 0;
}

SlEvent* SweeplineDatastructNaive::findByCoordinates(int x, int y) const {
    return nodes[y * pWidth + x];
}

IsolationRecord SweeplineDatastructNaive::calcPeak(const SlEvent *node) {
/*
  Tile *t = Tile::createFromOtherTile(pWidth, pHeight, mSamples);
  IsolationFinder ifinder(nullptr, t);
  //LatLng location = t->latlng(node.offsets);
  //IsolationRecord ir = ifinder.findIsolation(t, &location, node.offsets, node.getElev());
  IsolationRecord ir = ifinder.findIsolation(node->mOffsets);
  */
 IsolationRecord ir;
 ir.distance = FLT_MAX;
 for (int i = 0; i < pHeight * pWidth; i++) {
    if (nodes[i] != nullptr && !nodes[i]->equal(*node)) {
        float dist = node->distance(*nodes[i]);
        if (dist < ir.distance) {
            ir.distance = dist;
            ir.closestHigherGround = *nodes[i];
            ir.foundHigherGround = true;
        }
    }
 }

 if (ir.foundHigherGround) {
    ir.distance = node->distanceEllipsoid(ir.closestHigherGround);
 }

  //delete t;
  return ir;
}

void SweeplineDatastructNaive::saveAsImage(int w, int h, int level, SlEvent **allNodes, int nodeSize) {
    FILE *imageFile;
   int x,y,height=h-1,width=w-1;
   std::ostringstream stream;
   if (level < 10) {
     stream << "step-00" << level << ".ppm";
   } else if (level < 100) {
     stream << "step-0" << level << ".ppm";
   } else {
     stream << "step-" << level << ".ppm";
   }
   imageFile=fopen(stream.str().c_str() ,"wb");
   if(imageFile==NULL){
      perror("ERROR: Cannot open output file");
      exit(EXIT_FAILURE);
   }

   fprintf(imageFile,"P6\n");               // P6 filetype
   fprintf(imageFile,"%d %d\n",width,height);   // dimensions
   fprintf(imageFile,"255\n");              // Max pixel

   unsigned char pix[w*h*3] = {0};
   
    int active = 0;
    for (int j = 0; j < pHeight; j++) {
        for (int i = 0; i < pWidth; i++) {
            if (findByCoordinates(i, j) != nullptr) {
                pix[(j*width + i)*3 + 1] = 255;
                active++;
            }
        }
    }
   fwrite(pix,1,3*w*h,imageFile);
   fclose(imageFile);

   std::ofstream foutput;
   std::ifstream finput;
   finput.open("sketch.txt");
   foutput.open("sketch.txt", std::ios::app);
   if (finput.is_open()) {
    foutput << level * 5000 - 5000 << "," << active << "\n";
   }
   finput.close();
   foutput.close();
   //delete [] pix;
}

Offsets SweeplineDatastructNaive::getOffsets(const LatLng* pos) const {
    Coord x = pWidth - pWidth * ((pos->latitude() - mBottomRight.latitude()) / (mTopLeft.latitude() - mBottomRight.latitude()));
    Coord y = pHeight - pHeight * ((pos->longitude() - mBottomRight.longitude()) / (mTopLeft.longitude() - mBottomRight.longitude()));
    assert(x < pWidth);
    assert(y < pHeight);
    assert(x >= 0);
    assert(y >= 0);
    return Offsets(x, y);
}

void SweeplineDatastructNaive::saveHeatMap(int level){}
