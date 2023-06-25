// Convert TIFF to HGT
#include "tiff.h"
#include "tiffio.h"
#include "file_format.h"
#include "hgt_writer.h"
#include "primitives.h"
#include "easylogging++.h"
#include "tile.h"
#include <cstdio>
#include <string>


INITIALIZE_EASYLOGGINGPP

static uint16_t swapByteOrder16(uint16_t us) {
  return (us >> 8) | (us << 8);
}

int convertMars() {
  FileFormat format = FileFormat(FileFormat::Value::HGT_MARS);

  TIFF *tif = TIFFOpen("/home/pc/Data1/Mars/Mars_HRSC_MOLA_BlendDEM_Global_200mp_v2.tif", "r");

  if (tif == nullptr) {
    VLOG(3) << "Failed to open file";
    return -1;
  }
  uint32_t height;
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  uint32_t width = TIFFScanlineSize(tif) / 2;
  
  //int16_t *inbuf = (int16_t *) malloc(sizeof(int16_t) * width);
  int16_t inbuf[width];

  printf("Finished loading tiff-file\n");
  std::string dir = "/home/pc/Data1/Mars/dem15";
  // Split Tile into rectangles
  //int splitX = 14;
  //int splitY = 7;
  //int splitX = 12;
  //int splitY = 6;
  int splitX = 24;
  int splitY = 12;
  //int splitX = 360;
  //int splitY = 180;
  uint64_t newWidth = width / splitX;
  uint64_t newHeight = height / splitY;
  HgtWriter *writer = new HgtWriter(format);
  std::cout << newWidth+1 << "," << newHeight+1 << std::endl;
  for (int i = 0; i < splitX; ++i) {
    for (int j = 0; j < splitY; ++j) {
      printf("Create tile %d, %d\n", i, j);
      uint64_t sampleSize = (newWidth+1) * (newHeight+1) * sizeof(Elevation);
      int intSampleSize = (newWidth+1) * (newHeight+1) * sizeof(Elevation);
      Elevation *samples = (Elevation *) malloc(sampleSize);
      // On overlapp on left and top
      for (uint64_t y = 0; y < newHeight+1; ++y) {
        int offY = j*newHeight + y-1;
        if (offY < 0) {
          offY = height-1;
        }
        if (!TIFFReadScanline(tif, inbuf, offY)) {
          std::cout << "Error: unable to read line: " << offY << std::endl;
          return -1;
        }
        for (uint64_t x = 0; x < newWidth+1; ++x) {
          int offX = i*newWidth + x-1;
          if (offX < 0) {
            offX = width-1;
          }
          uint64_t sampleIdx = y * (newWidth + 1) + x;
          samples[sampleIdx] = static_cast<Elevation>(inbuf[offX]);
          //std::cout << samples[y*newWidth + x] << std::endl;
        }
      }
      Tile *t = new Tile(newWidth+1, newHeight+1, samples, format);
      writer->writeTile(dir, (splitY - j) * (180.f / splitY) - 90.f, i * (360.f / splitX) - 180.f, t);
      //t->saveAsImage("/home/pc/tmp",  i * (180.f / splitY) - 90.f, j * (360.f / splitX) - 180.f);
      delete t;
    }
  }
//free(inbuf);
  TIFFClose(tif);
  return 0;
}

int convertSOAA() {

  TIFF *tif = TIFFOpen(
      "/home/pc/Data1/NOAA-TIFF/ncei19_n24x75_w081x75_2022v1.tif", "r");
  if (tif) {
  FILE *fd = stdout;
    TIFFPrintDirectory(tif, fd);
    int tileSize = TIFFTileSize(tif);
    uint32_t tNumber = TIFFNumberOfTiles(tif);
    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t rowSize = TIFFTileRowSize64(tif);
    TIFFDefaultTileSize(tif, &width, &height);
    std::cout << "Tiles: " << tNumber  << " rows: " << rowSize << " w: " << width << " h: " << height << std::endl;
    std::cout << tileSize / 4 << ", " << sizeof(Elevation) << std::endl;

    Elevation *buf;
    for (int i = 0; i < tNumber; i++) {
      buf = (Elevation*)malloc(tileSize);
      TIFFReadEncodedTile(tif, i, buf, tileSize);
      FileFormat format(FileFormat::Value::HGT1);
      Tile *t = new Tile(512, 512, buf, format);
      t->saveAsImage("/home/pc/tmp/noaa", i, 1.0);
      delete t;
    }
    TIFFClose(tif);
  }
  return 0;
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);
  return convertMars();
  //return convertSOAA();
}
