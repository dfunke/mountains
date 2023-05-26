#ifndef _GEOTIFF_LOADER_H_
#define _GEOTIFF_LOADER_H_

#include "tile_loader.h"
#include "file_format.h"


// Load a .hgt tile.  This is the format used by SRTM data.

class GeotiffLoader : public TileLoader {
public:
  GeotiffLoader(FileFormat format);
  // minLat and minLng name the SW corner of the tile, in degrees
  virtual Tile *loadTile(const std::string &directory, float minLat, float minLng);
private:
  FileFormat mFormat;
};

#endif  // _HGT_LOADER_H_
