

#ifndef _ISOLATION_FINDER_SL_CELL_H__
#define _ISOLATION_FINDER_SL_CELL_H__

#define MAX_VECTOR_SIZE 64

#include "../primitives.h"
#include "sweepline_primitives.h"

#include <functional>

class CellMemoryManager;

struct Cell {
  SlEvent **content = nullptr;
  std::size_t size;
  // smaller lat / lng values
  Cell *smaller = nullptr;
  // bigger lat / lng values
  Cell *bigger = nullptr;
  Cell *parent = nullptr;
  LatLng topLeft;
  LatLng bottomRight;
  bool splitAtLat;
  CellMemoryManager* memoryManager;

  void presetChilds(Cell* childs, std::size_t childsSize, CellMemoryManager* memoryManager);

  bool isLeaf() const { return this->content != nullptr; }
  // Insert SlEvent to tree
  void insert(SlEvent *point);
  // remove event from tree + collaps
  void remove(const SlEvent *point);
  void collect(SlEvent **destination, uint *currSize);
  // Check if point is in cell
  bool isIn(const SlEvent *point) const;
  // Get the distance to the cell
  float distanceToCell(const SlEvent *point) const;
  void shortestDistance(const SlEvent *point, float* currShortest, LatLng *shortestPoint) const;
  // Get the distance to the cell
  float fastDistanceToCell(const SlEvent *point, float *lngDistanceScale, std::function<Offsets (float lat, float lng)> &toOffsets) const;
  void fastShortestDistance(const SlEvent *point, float* currShortest, LatLng *shortestPoint, float *lngDistanceScale, std::function<Offsets (float lat, float lng)> &toOffsets) const;
  // Splits the cell, this will be the parent cell
  void split();
  bool isInBigger(const SlEvent *point) const;
  Cell *getSmaller();
  // Return bigger, garantee not a nullptr
  Cell *getBigger();
  int getMaxDepth() const;
  int countLeafs() const;
  int countNodes() const;
  void configureChilds();
};
#include "cell_memory_manager.h"

#endif // ifndef _ISOLATION_FINDER_SL_CELL_H__
