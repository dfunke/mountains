#include "file_format.h"
#include "tile.h"
#include <string>

class HgtWriter {
  public:
  HgtWriter(FileFormat format) {
    mFormat = format;
  }
  void writeTile(const std::string &directory, float minLat, float minLng, Tile *t);

private:
  FileFormat mFormat;
};
