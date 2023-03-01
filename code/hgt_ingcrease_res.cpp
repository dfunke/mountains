#include "easylogging++.h"
#include "file_format.h"
#include "hgt_loader.h"
#include "hgt_writer.h"
#include "tile.h"
#include "tile_cache.h"
#include "tile_loading_policy.h"
#include "getopt_internal.h"

#ifdef PLATFORM_LINUX
#include <unistd.h>
#endif
#ifdef PLATFORM_WINDOWS
#include "getopt-win.h"
#endif

#include "assert.h"
#include <cmath>
#include <stdio.h>
#include <stdlib.h>

INITIALIZE_EASYLOGGINGPP

using std::ceil;
using std::floor;
Tile *incRes(Tile *base, FileFormat oldFormat, FileFormat newFormat) {
  std::size_t num_samples =
      newFormat.rawSamplesAcross() * newFormat.rawSamplesAcross();
  float ratio =
      newFormat.rawSamplesAcross() / (1.f * oldFormat.rawSamplesAcross());
  assert(ratio > 1);
  Elevation *samples = (Elevation *)malloc(sizeof(Elevation) * num_samples);
  for (int j = 0; j < newFormat.rawSamplesAcross(); j++) {
    for (int i = 0; i < newFormat.rawSamplesAcross(); i++) {
      float x = i / ratio;
      float y = j / ratio;
      int x1 = floor(x);
      int x2 = ceil(x);
      int y1 = floor(y);
      int y2 = ceil(y);
      Elevation x1y1 = base->get(x1, y1);
      Elevation x2y1 = base->get(x2, y1);
      Elevation x1y2 = base->get(x1, y2);
      Elevation x2y2 = base->get(x2, y2);
      Elevation e1 = (x - x1) * x1y1 + (1 - x + x1) * x2y1;
      Elevation e2 = (x - x1) * x1y2 + (1 - x + x1) * x2y2;
      Elevation erg = (y - y1) * e1 + (1 - y + y1) * e2;
      int idx = j * newFormat.rawSamplesAcross() + i;
      samples[idx] = erg;
    }
  }
  Tile *erg = new Tile(newFormat.rawSamplesAcross(),
                       newFormat.rawSamplesAcross(), samples, newFormat);
  return erg;
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  std::string hgt3Folder = "/home/pc/Data2/SRTM-DEM3";
  std::string saveFolder = "/home/pc/tmp";
  std::string format = "SRTM3";
  std::string toFormatName = "SRTM1";
  argc -= optind;
  argv += optind;
  float bounds[4];

  for (int i = 0; i < 4; ++i) {
    char *endptr;
    bounds[i] = strtof(argv[i], &endptr);
    if (*endptr != 0) {
      printf("Couldn't parse argument %d as number: %s\n", i + 1, argv[i]);
      return 1;
    }
  }
  argc -= 3;
  argv += 3;
  int ch;

  // Swallow --v that's parsed by the easylogging library
  const struct option long_options[] = {
      {"v", required_argument, nullptr, 0},
      {nullptr, 0, 0, 0},
  };
  while ((ch = getopt_long(argc, argv, "i:o:f:t:", long_options, nullptr)) !=
         -1) {
    switch (ch) {
    case 'i':
      hgt3Folder = optarg;
      break;
    case 'o':
      saveFolder = optarg;
      break;
    case 'f':
      format = optarg;
      break;
    case 't':
      toFormatName = optarg;
      break;
    }
  }
  FileFormat *fileFormat = FileFormat::fromName(format);
  if (fileFormat == nullptr) {
    std::cout << "Error format: " << format << " unknown" << std::endl;
    return 1;
  }
  // FileFormat fileFormat(FileFormat::Value::HGT3);
  BasicTileLoadingPolicy policy(hgt3Folder.c_str(), *fileFormat);
  TileCache *cache = new TileCache(&policy, 0);


  FileFormat *higherFormat = FileFormat::fromName(toFormatName);
  if (higherFormat == nullptr) {
    std::cout << "Error format: " << toFormatName << " unknown" << std::endl;
    return 1;
  }
  
  for (auto lat = (float) floor(bounds[0]); lat < ceil(bounds[1]); lat += 1) {
    for (auto lng = (float) floor(bounds[2]); lng < ceil(bounds[3]); lng += 1) {
      CoordinateSystem *coordinateSystem =
          fileFormat->coordinateSystemForOrigin(lat, lng);
      Tile *t = cache->loadWithoutCaching(lat, lng, *coordinateSystem);
      if (t != nullptr) {
        Tile *higherRes = incRes(t, *fileFormat, *higherFormat);
        HgtWriter writer(*higherFormat);
        writer.writeTile(saveFolder.c_str(), lat, lng, higherRes);
        delete t;
        delete higherRes;
      }
    }
  }


  // Compare tiles
  /*
   BasicTileLoadingPolicy policy2(saveFolder.c_str(),fileFormat);
  TileCache *cache2 = new TileCache(&policy2, 0);
  Tile *t2 = cache2->loadWithoutCaching(46, 9, *coordinateSystem);


  Tile *t2 = cache2->loadWithoutCaching(46, 9, *coordinateSystem);
  for (int i = 0; i < t->width(); i++) {
      for (int j = 0; j < t->height(); j++) {
          Offsets o(i,j);
          if (t->get(i, j) != t2->get( 3*i, 3*j)) {
              std::cout << "Diff at: " << i <<", "  << j << " t1: " << t->get(o)
  << " t2: " << t2->get(o) << std::endl;

          }
      }
  }
  delete t2;
  */
  return 0;
}
