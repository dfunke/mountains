// Convert TIFF to HGT
#include "file_format.h"
#include "hgt_writer.h"
#include "primitives.h"
#include "tiff_loader.h"
#include "easylogging++.h"
#include <string>


INITIALIZE_EASYLOGGINGPP

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  FileFormat format = FileFormat(FileFormat::Value::HGT_MARS);
  GeotiffLoader *loader = new GeotiffLoader(format);
  Tile *tiffT = loader->loadTile("/home/pc/Data1/Mars/Mars_HRSC_MOLA_BlendDEM_Global_200mp_v2.tif", -90.f, -180.f);
  std::string dir = "~/tmp";

  // Split Tile into rectangles
  int splitX = 14;
  int splitY = 7;
  int newWidth = tiffT->width() / splitX;
  int newHeight = tiffT->height() / splitY;
  HgtWriter *writer = new HgtWriter(format);
  for (int i = 0; i < splitX; ++i) {
    for (int j = 0; j < splitY; ++j) {
    Elevation *samples = (Elevation *) malloc(sizeof(Elevation) * (newWidth+1) * (newHeight+1));
      // On overlapp on left and top
      for (int y = 0; y < newHeight+1; ++y) {
        for (int x = 0; x < newWidth+1; ++x) {
          int offX = i*newWidth + x-1;
          int offY = j*newHeight + y-1;
          if (offX < 0) {
            offX = tiffT->width()-1;
          }
          if (offY < 0) {
            offY = tiffT->height()-1;
          }
          samples[y*newWidth + x] = tiffT->get(Offsets(offX, offY));
        }
      }
      Tile *t = new Tile(newWidth, newHeight, samples, format);
      writer->writeTile(dir, i * (180.f / splitY) - 90.f, j * (360.f / splitX) - 180.f, t);
    }
  }
  return 0;
}
