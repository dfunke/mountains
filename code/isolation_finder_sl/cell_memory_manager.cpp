#include "cell.h"

#include "cell_memory_manager.h"
#include "../latlng.h"
#include <assert.h>
#include <vector>
using std::vector;

CellMemoryManager::CellMemoryManager()
{
  cellChunksHead = nullptr;
  cellChunkStackIdx = 0;
  slEventChunksHead = nullptr;
  slEventChunksStackIdx = 0;
}

CellMemoryManager::~CellMemoryManager() {
  CellChunk* cellChunkRemove = cellChunksHead;
  while (cellChunkRemove != nullptr) {
    CellChunk* next = cellChunkRemove->next;
    delete cellChunkRemove;
    cellChunkRemove = next;
  }
  
  SlEventArrChunk* slEventChunksToRemove = slEventChunksHead;
  while (slEventChunksToRemove != nullptr) {
    SlEventArrChunk* next = slEventChunksToRemove->next;
    delete slEventChunksToRemove;
    slEventChunksToRemove = next;
  }
}

SlEvent **CellMemoryManager::allocSlEvents()
{
  if (slEventChunksStackIdx > 0)
  {
    // Iterate head untill it hase space
    slEventChunksStackIdx--;
    return slEventChunksStack[slEventChunksStackIdx];
  }
  assert(slEventChunksStackIdx==0);
  // buffer full, new buffer as head
  SlEventArrChunk *oldHead = slEventChunksHead;
  slEventChunksHead = new SlEventArrChunk();
  slEventChunksHead->next = oldHead;
  fillSlEventChunkStack();
  slEventChunksStackIdx--;
  return slEventChunksStack[slEventChunksStackIdx];
}

void CellMemoryManager::freeSlEvents(SlEvent **space)
{
  if (slEventChunksStackIdx + 1 < 3 * NODE_CHUNK_SIZE) {
    std::fill(space, space + MAX_VECTOR_SIZE, nullptr);
    slEventChunksStack[slEventChunksStackIdx] = space;
    ++slEventChunksStackIdx;
  }
}

Cell *CellMemoryManager::allocCell()
{
  if (cellChunkStackIdx > 0)
  {
    cellChunkStackIdx--;
    return cellChunksStack[cellChunkStackIdx];
  }
  // buffer full, new buffer as head
  CellChunk *oldHead = cellChunksHead;
  cellChunksHead = new CellChunk();
  cellChunksHead->next = oldHead;
  fillCellChunkStack();
  cellChunkStackIdx--;
  return cellChunksStack[cellChunkStackIdx];
}

Cell *CellMemoryManager::allocCell(LatLng topLeft, LatLng bottomRight)
{
  Cell *c = allocCell();
  c->content = allocSlEvents();
  c->size = 0;
  c->memoryManager = this;
  c->topLeft = topLeft;
  c->bottomRight = bottomRight;
  return c;
}

Cell *CellMemoryManager::allocCell(LatLng topLeft, LatLng bottomRight, SlEvent **content, std::size_t size)
{
  Cell *c = allocCell();
  c->content = content;
  c->size = size;
  c->topLeft = topLeft;
  c->bottomRight = bottomRight;
  return c;
}

void CellMemoryManager::freeCell(Cell *cell)
{
  if (cell->content != nullptr) {
    freeSlEvents(cell->content);
    cell->content = nullptr;
  }
  if (cell->bigger != nullptr) {
    freeCell(cell->bigger);
    cell->bigger = nullptr;
  }
  if (cell->smaller != nullptr) {
    freeCell(cell->smaller);
    cell->smaller = nullptr;
  }
  if (cellChunkStackIdx + 1 < 3 * CELL_CHUNK_SIZE) {
    cell->size = 0;
    cellChunksStack[cellChunkStackIdx] = cell;
    ++cellChunkStackIdx;
  }
}

void CellMemoryManager::fillCellChunkStack(){
  for (int i = 0; i < CELL_CHUNK_SIZE; ++i) {
    cellChunksStack[i] = cellChunksHead->cells + i;
  }
  cellChunkStackIdx = CELL_CHUNK_SIZE;
}

void CellMemoryManager::fillSlEventChunkStack(){
  std::fill(slEventChunksHead->nodes, slEventChunksHead->nodes + (NODE_CHUNK_SIZE * MAX_VECTOR_SIZE), nullptr);
  for (int i = 0; i < NODE_CHUNK_SIZE; ++i) {
    slEventChunksStack[i] = slEventChunksHead->nodes + (i *  MAX_VECTOR_SIZE);
  }
  slEventChunksStackIdx = NODE_CHUNK_SIZE;
}
