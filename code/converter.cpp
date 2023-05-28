// Convert TIFF to HGT
#include "tiff.h"
#include "tiffio.h"
#include "file_format.h"
#include "hgt_writer.h"
#include "primitives.h"
#include "easylogging++.h"
#include <cstdio>
#include <string>


INITIALIZE_EASYLOGGINGPP

static uint16_t swapByteOrder16(uint16_t us) {
  return (us >> 8) | (us << 8);
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  FileFormat format = FileFormat(FileFormat::Value::HGT_MARS);

  TIFF *tif = TIFFOpen("/home/pc/Data1/Mars/Mars_HRSC_MOLA_BlendDEM_Global_200mp_v2.tif", "r");

  if (tif == nullptr) {
    VLOG(3) << "Failed to open file";
    return -1;
  }
  uint32_t height;
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  uint32_t width = TIFFScanlineSize(tif) / 2;
  
  int16_t *inbuf = (int16_t *) malloc(sizeof(int16_t) * width);

  printf("Finished loading tiff-file\n");
  std::string dir = "/home/pc/tmp";
  // Split Tile into rectangles
  //int splitX = 14;
  //int splitY = 7;
  int splitX = 14;
  int splitY = 7;
  int newWidth = width / splitX;
  int newHeight = height / splitY;
  HgtWriter *writer = new HgtWriter(format);
  for (int i = 0; i < splitX; ++i) {
    for (int j = 0; j < splitY; ++j) {
      printf("Create tile %d, %d\n", i, j);
      Elevation *samples = (Elevation *) malloc(sizeof(Elevation) * (newWidth+1) * (newHeight+1));
      // On overlapp on left and top
      for (int y = 0; y < newHeight+1; ++y) {
        int offY = j*newHeight + y-1;
        if (offY < 0) {
          offY = height-1;
        }
        if (!TIFFReadScanline(tif, inbuf, offY)) {
          std::cout << "Error: unable to read line: " << offY << std::endl;
          return -1;
        }
        for (int x = 0; x < newWidth+1; ++x) {
          int offX = i*newWidth + x-1;
          if (offX < 0) {
            offX = width-1;
          }
          samples[y*(newWidth+1) + x] = static_cast<Elevation>(swapByteOrder16(inbuf[offX]));
          //std::cout << samples[y*newWidth + x] << std::endl;
        }
      }
      Tile *t = new Tile(newWidth+1, newHeight+1, samples, format);
      writer->writeTile(dir, i * (180.f / splitY) - 90.f, j * (360.f / splitX) - 180.f, t);
      t->saveAsImage("/home/pc/tmp",  i * (180.f / splitY) - 90.f, j * (360.f / splitX) - 180.f);
      delete t;
    }
  }
  free(inbuf);
  TIFFClose(tif);
  return 0;
}
