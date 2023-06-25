
#ifndef _SLDEM_LOADER_H_
#define _SLDEM_LOADER_H_

#include "tile_loader.h"
#include "file_format.h"


// Load a FLOAT IMG tile.  This is the format used by SLDEM2015 data.

class SldemLoader : public TileLoader {
public:
  SldemLoader(FileFormat format){
    mFormat = format;
  }
  // minLat and minLng name the SW corner of the tile, in degrees
  virtual Tile *loadTile(const std::string &directory, float minLat, float minLng);
private:
  FileFormat mFormat;
};

#endif  // _SLDEM_LOADER_H_
