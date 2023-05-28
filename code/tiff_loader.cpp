#include "tiff_loader.h"
#include "tile.h"
#include "util.h"
#include "easylogging++.h"
#include "file_format.h"

#include "tiff.h"
#include "tiffio.h"

#include <cstdint>
#include <stdio.h>
#include <string>
#include <cmath>
#include "assert.h"

using std::string;

static const int16_t TIFF_NODATA_ELEVATION = -32768;

static uint16_t swapByteOrder16(uint16_t us) {
  return (us >> 8) | (us << 8);
}

GeotiffLoader::GeotiffLoader(FileFormat format) {
  mFormat = format;
}


Tile *GeotiffLoader::loadTile(const std::string &directory, float minLat, float minLng) {
  TIFF *tif = TIFFOpen(directory.c_str(), "r");

  if (tif == nullptr) {
    VLOG(3) << "Failed to open file " << directory;
    return nullptr;
  }
  uint32_t height;
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  uint32_t width = TIFFScanlineSize(tif) / 2;
  
  int16_t *inbuf = (int16_t *) malloc(sizeof(int16_t) * width);
  
  Tile *retval = nullptr;
  uint32_t row;
  Elevation *samples = (Elevation *) malloc(sizeof(Elevation) * width * height);
  for (row = 0; row < height; ++row) {
    TIFFReadScanline(tif, inbuf, row);
  // SRTM data is in big-endian order; convert to Elevation
    for (uint32_t i = 0; i < width; ++i) {
      //int16_t elevation = swapByteOrder16(inbuf[i]);
      int16_t elevation = inbuf[i];
      if (elevation == TIFF_NODATA_ELEVATION) {
        samples[row * width + i] = Tile::NODATA_ELEVATION;
      } else {
        samples[row * width + i] = static_cast<Elevation>(elevation);
      }
    }
  }
  TIFFClose(tif);
  
  retval = new Tile(width, height, samples, mFormat);
  free(inbuf);

  return retval;
}
