#include "file_format.h"
#include "tile.h"
#include <string>

class SldemWriter {
  public:
  SldemWriter(FileFormat format) {
  }
  void writeTile(const std::string &directory, float minLat, float minLng, Tile *t);
};
