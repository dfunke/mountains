#include "cell.h"
#include "cell_memory_manager.h"
#include "spherical_math_util.h"
#include <iostream>

#include <algorithm>

void Cell::presetChilds(Cell* childs, std::size_t childsSize, CellMemoryManager* memoryManager) {
  this->memoryManager = memoryManager;
  smaller = &childs[0];
  bigger = &childs[childsSize/2];
  this->splitAtLat = (this->topLeft.latitude() - this->bottomRight.latitude()) > (this->topLeft.longitude() - this->bottomRight.longitude());
  configureChilds();
  if (childsSize > 2) {
    smaller->presetChilds(childs+1, (childsSize / 2) - 2 , memoryManager);
    bigger->presetChilds(childs+ (childsSize / 2) + 1, (childsSize / 2) - 2, memoryManager);
  } else {
    this->size = 0;
    this->content = this->memoryManager->allocSlEvents();
  }
}

void Cell::configureChilds() {
  float center = (this->topLeft.get(splitAtLat) + this->bottomRight.get(splitAtLat)) / 2;
  LatLng lowerBound;
  // set values of lowe bound depending on split dimension
  lowerBound.set(splitAtLat, center);
  lowerBound.set(!splitAtLat, this->bottomRight.get(!splitAtLat));
  this->bigger = memoryManager->allocCell(this->topLeft, lowerBound);
  LatLng upperBound;
  upperBound.set(splitAtLat, center);
  upperBound.set(!splitAtLat, this->topLeft.get(!splitAtLat));
  this->smaller = memoryManager->allocCell(upperBound, this->bottomRight);
  this->bigger->parent = this;
  this->smaller->parent = this;
  assert(bigger->bottomRight.latitude() != bigger->topLeft.latitude());
  assert(bigger->bottomRight.longitude() != bigger->topLeft.longitude());

  assert(smaller->bottomRight.latitude() != smaller->topLeft.latitude());
  assert(smaller->bottomRight.longitude() != smaller->topLeft.longitude());
}

void Cell::split() {
  assert(this->content != nullptr);
  assert(this->size == MAX_VECTOR_SIZE);
  this->splitAtLat = (this->topLeft.latitude() - this->bottomRight.latitude()) > (this->topLeft.longitude() - this->bottomRight.longitude());

  float center = (this->topLeft.get(splitAtLat) + this->bottomRight.get(splitAtLat)) / 2;
 configureChilds();

  for (std::size_t i = 0; i < MAX_VECTOR_SIZE; ++i)
  {
    if (content[i]->get(splitAtLat) > center)
    {
      this->bigger->insert(content[i]);
    }
    else
    {
      this->smaller->insert(content[i]);
    }
  }
  // delete this->content;
  memoryManager->freeSlEvents(this->content);
  this->content = nullptr;
  if (smaller->size == MAX_VECTOR_SIZE) {
    smaller->split();
  }
  if (bigger->size == MAX_VECTOR_SIZE) {
    bigger->split();
  }
  assert(this->smaller->size + this->bigger->size == this->size);
}

Cell *Cell::getSmaller()
{
  if (smaller == nullptr)
  {
    if (splitAtLat)
    {
      smaller = memoryManager->allocCell (
          LatLng(this->bigger->bottomRight.latitude(), topLeft.longitude()),
          bottomRight);
    }
    else
    {
      smaller = memoryManager->allocCell(
          LatLng(topLeft.latitude(), this->bigger->bottomRight.longitude()),
          bottomRight);
    }
  }
  return smaller;
}

Cell *Cell::getBigger()
{
  if (bigger == nullptr)
  {
    // Create cell from nullprtr
    if (splitAtLat)
    {
      bigger = memoryManager->allocCell (
          topLeft,
          LatLng(this->smaller->topLeft.latitude(), bottomRight.longitude()));
    }
    else
    {
      bigger = memoryManager->allocCell (
          topLeft,
          LatLng(bottomRight.latitude(), this->smaller->topLeft.longitude()));
    }
  }
  return bigger;
}

void Cell::insert(SlEvent *point)
{
  // First step, find cell containing point and add one to size
  Cell* toCheck = this;
  // find leaf where node needs to be removed
  while(!toCheck->isLeaf()) {
    toCheck->size++;
    if (toCheck->isInBigger(point)) {
      toCheck = toCheck->getBigger();
    } else {
      toCheck = toCheck->getSmaller();
    }
  }
  if (toCheck->size < MAX_VECTOR_SIZE)
  {
    for (int i = 0; i < MAX_VECTOR_SIZE; i++) {
      if (toCheck->content[i] == nullptr) {
        toCheck->content[i] = point;
        toCheck->size++;
        return;
      }
    }
    assert(false); // If it is reached, there was no nullptr in that array
    return;
  }
  else
  {
    // Split up and insert aggain
    toCheck->split();
    toCheck->insert(point);
  }
}

void Cell::remove(const SlEvent *point)
{
  // First step, find cell containing point and add one to size
  Cell* toCheck = this;
  // find leaf where node needs to be removed
  while(!toCheck->isLeaf()) {
    toCheck->size--;
    if (toCheck->isInBigger(point)) {
      toCheck = toCheck->getBigger();
    } else {
      toCheck = toCheck->getSmaller();
    }
  }
  assert(toCheck->size > 0);
  // Set point to nullptr in array
  for (uint i = 0; i < MAX_VECTOR_SIZE; i++) {
    if (toCheck->content[i] != nullptr && toCheck->content[i]->latitude() == point->latitude() && toCheck->content[i]->longitude() == point->longitude()) {
      toCheck->content[i] = nullptr;
      toCheck->size--;
      break;
    }
  }
  // Start collapsing if parent size to small
  if (toCheck->parent != nullptr && toCheck->parent->size < MAX_VECTOR_SIZE / 2) {
    // go back up untill parent-size > MAX_VECTOR_SIZE/2
    while (toCheck->parent != nullptr && toCheck->parent->size < MAX_VECTOR_SIZE)
    {
      toCheck = toCheck->parent;
    }
    assert(toCheck->content == nullptr);
    toCheck->content = memoryManager->allocSlEvents();
    uint currSize = 0;
    if (toCheck->bigger != nullptr) {
      toCheck->bigger->collect(toCheck->content, &currSize);
      memoryManager->freeCell(toCheck->bigger);
      toCheck->bigger = nullptr;
    }
    if (toCheck->smaller != nullptr) {
      toCheck->smaller->collect(toCheck->content, &currSize);
      memoryManager->freeCell(toCheck->smaller);
      toCheck->smaller = nullptr;
    }
    assert(currSize == toCheck->size);
  }
}

void Cell::collect(SlEvent** dest, uint *currSize) {
  if (this->isLeaf()) {
    uint idx = 0;
    for(uint i = 0; i < MAX_VECTOR_SIZE; i++) {
      if (content[i] != nullptr) {
        // fill new array from bottom
        dest[idx + *currSize] = content[i];
        idx++;
      }
    }
    *currSize = *currSize + size;
  } else {
    if (smaller != nullptr) {
      smaller->collect(dest, currSize);
    }
    if (bigger != nullptr) {
      bigger->collect(dest, currSize);
    }
  }
}

bool Cell::isInBigger(const SlEvent *point) const {
  if (this->bigger != nullptr) {
    return this->bigger->isIn(point);
  }
  return !this->smaller->isIn(point);
}

bool Cell::isIn(const SlEvent *point) const {
   return topLeft.latitude() >= point->latitude() && bottomRight.latitude() < point->latitude()
    && topLeft.longitude() >= point->longitude() && bottomRight.longitude() < point->longitude();
  /*
  return topLeft.latitude() > point->latitude() &&
    topLeft.longitude() > point->longitude() &&
    bottomRight.latitude() <= point->latitude() &&
    bottomRight.longitude() <= point->longitude();
    */
}

int Cell::getMaxDepth() const {
  if (isLeaf()) {
    return 1;
  }
  int biggerCount = 0;
  int smallerCount = 0;
  if (bigger != nullptr) {
    biggerCount = bigger->getMaxDepth();
  }
  if (smaller != nullptr) {
    smallerCount = smaller->getMaxDepth();
  }

  if (smallerCount > biggerCount) {
    return smallerCount + 1;
  } 
  return biggerCount + 1;
}

int Cell::countLeafs() const {
  if (this->isLeaf()) {
    return 1;
  }
  int biggerCount = 0;
  int smallerCount = 0;
  if (bigger != nullptr) {
    biggerCount = bigger->countLeafs();
  }
  if (smaller != nullptr) {
    smallerCount = smaller->countLeafs();
  }
  return biggerCount + smallerCount;
}

int Cell::countNodes() const {
  if (this->isLeaf()) {
    return 1;
  }
  int biggerCount = 0;
  int smallerCount = 0;
  if (bigger != nullptr) {
    biggerCount = bigger->countNodes();
  }
  if (smaller != nullptr) {
    smallerCount = smaller->countNodes();
  }
  return biggerCount + smallerCount + 1;
}

float Cell::distanceToCell(const SlEvent *point) const {
  return shortestDistanceToQuadrillateral(point, topLeft, bottomRight);
}

float Cell::fastDistanceToCell(const SlEvent *point, float *lngDistanceScale, std::function<Offsets (float lat, float lng)> &toOffsets) const {
  return fastShortestDistanceToQuadrillateral(point, topLeft, bottomRight, lngDistanceScale, toOffsets);
}

void Cell::shortestDistance(const SlEvent *point, float *currShortest, LatLng *shortestPoint) const {

  if (this->isLeaf()) {
    for (uint i = 0; i < MAX_VECTOR_SIZE; i++) {
      if (content[i] == nullptr) {
        continue;
      }
      float d = searchDistance(point, *this->content[i]);
//      if (content[i]->equal(*point)) {
//         std::cout << "Found equal!" << std::endl;
//        continue;
//      }
      if (d < *currShortest) {
        *currShortest = d;
        *shortestPoint = LatLng(this->content[i]->latitude(), this->content[i]->longitude());
      }
    }
    return;
  }
  if (this->bigger != nullptr && this->smaller != nullptr) {
    float distToBigger = this->bigger->distanceToCell(point);
    float distToSmaller = this->smaller->distanceToCell(point);
    if (distToBigger < distToSmaller) {
      if (distToBigger > *currShortest) {
        // Do not need to go further
        return;
      }
      this->bigger->shortestDistance(point, currShortest, shortestPoint);
      if (distToSmaller < *currShortest) {
        this->smaller->shortestDistance(point, currShortest, shortestPoint);
      }
    } else {
        if (distToSmaller > *currShortest) {
        // Do not need to go further
        return;
      }
      this->smaller->shortestDistance(point, currShortest, shortestPoint);
      if (distToBigger < *currShortest) {
        this->bigger->shortestDistance(point, currShortest, shortestPoint);
      }
    }
  } else if (this->bigger != nullptr) {
    if (this->bigger->distanceToCell(point) < *currShortest) {
      this->bigger->shortestDistance(point, currShortest, shortestPoint);
    }
  } else if (this->smaller != nullptr) {
    if (this->smaller->distanceToCell(point) < *currShortest) {
      this->smaller->shortestDistance(point, currShortest, shortestPoint);
    }
  }
}

void Cell::fastShortestDistance(const SlEvent *point, float* currShortest, LatLng *shortestPoint, float *lngDistanceScale, std::function<Offsets (float lat, float lng)> &toOffsets) const {
  if (this->isLeaf()) {
    for (std::size_t i = 0; i < MAX_VECTOR_SIZE; ++i) {
      if (content[i] == nullptr) {
        continue;
      }
      float d = fastSearchDistance(point->getOffsets(), this->content[i]->getOffsets(), lngDistanceScale);
//      if (content[i]->equal(*point)) {
//         std::cout << "Found equal!" << std::endl;
//        continue;
//      }
      if (d < *currShortest) {
        *currShortest = d;
        *shortestPoint = LatLng(this->content[i]->latitude(), this->content[i]->longitude());
      }
    }
    return;
  }
  if (this->bigger != nullptr && this->smaller != nullptr) {
    float distToBigger = this->bigger->fastDistanceToCell(point, lngDistanceScale, toOffsets);
    float distToSmaller = this->smaller->fastDistanceToCell(point, lngDistanceScale, toOffsets);
    if (distToBigger < distToSmaller) {
      if (distToBigger > *currShortest) {
        // Do not need to go further
        return;
      }
      this->bigger->fastShortestDistance(point, currShortest, shortestPoint, lngDistanceScale, toOffsets);
      if (distToSmaller < *currShortest) {
        this->smaller->fastShortestDistance(point, currShortest, shortestPoint, lngDistanceScale, toOffsets);
      }
    } else {
        if (distToSmaller > *currShortest) {
        // Do not need to go further
        return;
      }
      this->smaller->fastShortestDistance(point, currShortest, shortestPoint, lngDistanceScale, toOffsets);
      if (distToBigger < *currShortest) {
        this->bigger->fastShortestDistance(point, currShortest, shortestPoint, lngDistanceScale, toOffsets);
      }
    }
  } else if (this->bigger != nullptr) {
    if (this->bigger->fastDistanceToCell(point, lngDistanceScale, toOffsets) < *currShortest) {
      this->bigger->fastShortestDistance(point, currShortest, shortestPoint, lngDistanceScale, toOffsets);
    }
  } else if (this->smaller != nullptr) {
    if (this->smaller->fastDistanceToCell(point, lngDistanceScale, toOffsets) < *currShortest) {
      this->smaller->fastShortestDistance(point, currShortest, shortestPoint, lngDistanceScale, toOffsets);
    }
  }
}
