#ifndef _ISOLATION_FINDER_SL_CELL_MEMORY_MANAGER_H__
#define _ISOLATION_FINDER_SL_CELL_MEMORY_MANAGER_H__

#define CELL_CHUNK_SIZE 128
#define NODE_CHUNK_SIZE 64

#include "sweepline_primitives.h"
#include "cell.h"

#include <vector>
#include <algorithm>
using std::vector;

//
// CellChunks as double linked list
//
struct CellChunk {
  Cell cells[CELL_CHUNK_SIZE];
  CellChunk* next = nullptr;
};

//
// Array of SlEvent pointer chunk as double linked list
//
struct SlEventArrChunk {
  SlEvent* nodes[NODE_CHUNK_SIZE * MAX_VECTOR_SIZE];
  SlEventArrChunk* next = nullptr;
};

class CellMemoryManager {
public:
  CellMemoryManager();
  ~CellMemoryManager();
  SlEvent** allocSlEvents();
  void freeSlEvents(SlEvent** space);
  Cell* allocCell();
  Cell* allocCell(LatLng topLeft, LatLng bottomRight);
  Cell* allocCell(LatLng topLeft, LatLng bottomRight, SlEvent **content, std::size_t size);
  void freeCell(Cell* cell);
private:
  void fillCellChunkStack();
  void fillSlEventChunkStack();
  CellChunk* cellChunksHead;
  Cell* cellChunksStack[3*CELL_CHUNK_SIZE];
  uint cellChunkStackIdx;
  SlEventArrChunk* slEventChunksHead;
  SlEvent** slEventChunksStack[3*NODE_CHUNK_SIZE];
  uint slEventChunksStackIdx;
};

#endif // ifndef _ISOLATION_FINDER_SL_CELL_MEMORY_MANAGER_H__
