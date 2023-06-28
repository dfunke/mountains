#include "coordinate_system.h"
#include "easylogging++.h"
#include "isolation_results.h"
#include "primitives.h"
#include "tile.h"
#include "tile_cache.h"
#include "tile_loading_policy.h"

#include "sldem_writer.h"
#include "hgt_writer.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#ifdef PLATFORM_LINUX
#include <unistd.h>
#endif
#ifdef PLATFORM_WINDOWS
#include "getopt-win.h"
#endif

#include "isolation_finder_sl/spherical_math_util.h"
#include "latlng.h"
#include <algorithm>
#include <glm/vec3.hpp>

INITIALIZE_EASYLOGGINGPP

using std::ceil;
using std::floor;
using std::string;
using namespace std::chrono;

int mergeOldResults() {
  IsolationResults res;
  string folder = "/home/pc/tmp/results";
  for (int lat = -90; lat < 90; ++lat) {
    for (int lng = -180; lng < 180; ++lng) {
      IsolationResults *ir = IsolationResults::loadFromFile(folder, lat, lng);
      if (ir != nullptr) {
        for (auto r : ir->mResults) {
          res.mResults.push_back(r);
        }
      }
    }
  }
  std::sort(res.mResults.begin(), res.mResults.end(),
            [](IsolationResult const &lhs, IsolationResult const &rhs) {
              return lhs.isolationKm > rhs.isolationKm;
            });
  res.save("/home/pc/tmp", 1, 1);

  return 0;
}

Offsets toOffests(float latitude, float longitude, int width, int height) {
  float minLng = std::floor(longitude);
  float minLat = std::floor(latitude);
  int x = (int)((longitude - minLng) * (width - 1) + 0.5);
  int y = (int)(((minLat + 1) - latitude) * (height - 1) + 0.5);
  return Offsets(x, y);
}

struct IsolationResultDiff {
  IsolationResult r1;
  IsolationResult r2;
  float diff;
};

int compare() {
  IsolationResults *newRes =
      IsolationResults::loadFromFile("/home/pc/tmp", 1, 2);
  IsolationResults *oldRes =
      IsolationResults::loadFromFile("/home/pc/tmp", 1, 1);
  std::cout << "Start calculation" << std::endl;

  string terrain_directory("/home/pc/Data2/SRTM-DEM3/");
  FileFormat fileFormat(FileFormat::Value::HGT3);
  BasicTileLoadingPolicy policy(terrain_directory.c_str(), fileFormat);
  TileCache *cache = new TileCache(&policy, 1);
  std::vector<IsolationResultDiff> diffs;

  for (auto res : newRes->mResults) {
    if (res.isolationKm < 3) {
      break;
    }
    IsolationResult resInOld = res;
    // start from bottom with searching
    bool foundOne = false;
    for (std::size_t i = 0; i < oldRes->mResults.size(); ++i) {
      if (oldRes->mResults[i].isolationKm < 3) {
        break;
      }
      if (oldRes->mResults[i].peak == res.peak) {
        foundOne = true;
        resInOld = oldRes->mResults[i];
        if (std::abs(res.isolationKm - resInOld.isolationKm) > 0) {
          IsolationResultDiff resDiff;
          resDiff.r1 = res;
          resDiff.r2 = resInOld;
          resDiff.diff = resInOld.isolationKm - res.isolationKm;
          diffs.push_back(resDiff);
        }
        break;
      }
    }
    if (!foundOne) {
      std::cout << "Did not find: " << res.peak.latitude() << " "
                << res.peak.longitude() << std::endl;
    }
  }
  std::sort(diffs.begin(), diffs.end(),
            [](IsolationResultDiff const &lhs, IsolationResultDiff const &rhs) {
              return lhs.diff > rhs.diff;
            });

  for (auto &diff : diffs) {
    printf("%f,%f,%f,%f,%f,  %f,%f,%f,%f,%f,  %f\n", diff.r1.peak.latitude(),
           diff.r1.peak.longitude(), diff.r1.higher.latitude(),
           diff.r1.higher.longitude(), diff.r1.isolationKm,
           diff.r2.peak.latitude(), diff.r2.peak.longitude(),
           diff.r2.higher.latitude(), diff.r2.higher.longitude(),
           diff.r2.isolationKm, diff.diff);
  }

  return 0;
}

int proccessMoonData() {
  string terrain_directory("/data02/funke/SRTM/Moon/hgt");
  //string terrain_directory("/home/pc/Data1/Moon/hgt");
  FileFormat fileFormat(FileFormat::Value::SLDEM);
  BasicTileLoadingPolicy policy(terrain_directory.c_str(), fileFormat);
  //policy.enableNeighborEdgeLoading(true);
  TileCache *cache = new TileCache(&policy, 1);

  int latMin = 0;
  int lngMin = 0;
  HgtWriter *writer = new HgtWriter(fileFormat);
  bool isSecondHalve = false;
  //int sideLength = 15360;
  int sideLength = 7681;
  int newSide = (sideLength-1)/3 + 1;
  Elevation *samples = (float*)malloc(sizeof(float)*newSide*newSide);
  for (latMin = -60; latMin < 60; latMin += 15) {
    for (lngMin = -180; lngMin < 180; lngMin += 15) {
      std::shared_ptr<CoordinateSystem> coordinateSystem(
          fileFormat.coordinateSystemForOrigin(latMin + 0.f, lngMin + 0.f));
      Tile *t = cache->loadWithoutCaching(latMin, lngMin, *coordinateSystem);
      for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
          for (int y = 0; y < newSide; ++y) {
            for (int x = 0; x < newSide; ++x) {
              samples[y*newSide + x] = t->get(i * (newSide-1) + x, j * (newSide-1) + y);
            }
          }
          Tile *newTile = new Tile(newSide, newSide, samples, fileFormat);
          writer->writeTile("/data02/funke/SRTM/Moon/hgt5", latMin + (10 -j * 5), lngMin + i * 5, newTile);
          //newTile->saveAsImage("/home/pc/tmp", latMin + (10 -j * 5), lngMin + i * 5);
          std::cout << latMin + j * 5 << "," << lngMin + i * 5 << std::endl;
        }
      }
      //writer->writeTile("/data02/funke/SRTM/Moon/hgt", latMin, lngMin, t);
      //t->saveAsImage("/data02/funke/SRTM/Moon/hgt", latMin, lngMin);
      //for (int offX = 0; offX < 2; ++offX) {
      //  for (int offY = 0; offY < 2; ++offY) {
      //    float lat = latMin + (1-offY) * 15;
      //    float lng = lngMin + offX * 15;
      //    for (int x = 0; x < (sideLength-1); ++x) {
      //      for (int y = 0; y < (sideLength-1); ++y) {
      //        samples[(y+1) * sideLength + x+1] = t->get(x + offX * (sideLength-1), y + offY * (sideLength-1));
      //      }
      //    }
      //    Tile *newTile = new Tile(sideLength, sideLength, samples, fileFormat);
      //    for (int x = 0; x < sideLength; x++) {
      //      newTile->set(x, 0, -32768);
      //      newTile->set(0, x, -32768);
      //    }
      //    writer->writeTile("/home/pc/tmp", lat , lng, newTile);
      //    //newTile->saveAsImage("/home/pc/tmp", lat, lng);
      //    std::cout << lat << "," << lng << std::endl;
      //  }
      //}
      //if (isSecondHalve) {
      //  // add first third to right part of sample
      //  for (int x = 0; x < sideLength/2; x++) {
      //    for (int y = 0; y < sideLength; y++) {
      //      samples[y*15360 + x + sideLength/2] = t->get(x,y);
      //    }
      //  }
      //  // write sample
      //  Tile *toWrite = new Tile(sideLength, sideLength, samples, FileFormat::Value::SLDEM);
      //  writer->writeTile("/home/pc/tmp", latMin, lngMin-15, toWrite);
      //  //toWrite->saveAsImage("/home/pc/tmp", latMin, lngMin-15);
      //  std::cout << "Wrote tile: " << latMin << ", " << lngMin-15 << std::endl;
      //  // the rest is a new tile
      //  for (int x = sideLength/2; x < sideLength + (sideLength/2); x++) {
      //    for (int y = 0; y < sideLength; y++) {
      //      samples[y*15360 + x - sideLength/2] = t->get(x,y);
      //    }
      //  }
      //  toWrite = new Tile(sideLength, sideLength, samples, FileFormat::Value::SLDEM);
      //  writer->writeTile("/home/pc/tmp", latMin, lngMin+15, toWrite);
      //  //toWrite->saveAsImage("/home/pc/tmp", latMin, lngMin+15);
      //  std::cout << "Wrote tile: " << latMin << ", " << lngMin+15 << std::endl;
      //} else {
      //  // Write first part
      //  for (int x = 0; x < sideLength; x++) {
      //    for (int y = 0; y < sideLength; y++) {
      //      samples[y*15360 + x] = t->get(x,y);
      //    }
      //  }
      //  Tile *toWrite = new Tile(sideLength, sideLength, samples, FileFormat::Value::SLDEM);
      //  writer->writeTile("/home/pc/tmp", latMin, lngMin, toWrite);
      //  //toWrite->saveAsImage("/home/pc/tmp", latMin, lngMin);
      //  std::cout << "Wrote tile: " << latMin << ", " << lngMin << std::endl;
      //  // save second part for later
      //  for (int x = sideLength; x < sideLength + sideLength/2; x++) {
      //    for (int y = 0; y < sideLength; y++) {
      //      samples[y*15360 + x - sideLength] = t->get(x,y);
      //    }
      //  }
      //}
      //isSecondHalve = !isSecondHalve;
      delete t;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  //return compare();
  // return mergeOldResults();
  return proccessMoonData();

  int ch;
  string terrain_directory("/home/pc/Data1/Mars/dem15");
  if (argc < 3) {
    return -1;
  }

  float bounds[4];
  argc -= optind;
  argv += optind;
  for (int i = 0; i < 4; ++i) {
    char *endptr;
    bounds[i] = strtof(argv[i], &endptr);
    if (*endptr != 0) {
      printf("Couldn't parse argument %d as number: %s\n", i + 1, argv[i]);
      break;
    }
  }
  argc -= 3;
  argv += 3;
  optind = 0;

  Elevation elev = 0;
  while ((ch = getopt(argc, argv, "i:e:")) != -1) {
    switch (ch) {
    case 'i':
      terrain_directory = optarg;
      break;
    case 'e':
      elev = atoi(optarg);
    }
  }

  int latMin = (int)floor(bounds[2]/15)*15;
  int lngMin = (int)floor(bounds[3]/15)*15;
  std::cout << "Loading tile: " << latMin << ", " << lngMin << std::endl;

  LatLng point(bounds[0], bounds[1]);
  FileFormat fileFormat(FileFormat::Value::HGT_MARS);
  BasicTileLoadingPolicy policy(terrain_directory.c_str(), fileFormat);
  TileCache *cache = new TileCache(&policy, 1);
  std::shared_ptr<CoordinateSystem> coordinateSystem(
      fileFormat.coordinateSystemForOrigin(latMin + 0.f, lngMin + 0.f));

  Tile *t = cache->loadWithoutCaching(latMin, lngMin, *coordinateSystem);
  float maxIsolation = 100000000;
  LatLng higherGround;
  Elevation higherGroundElev = 0;
  if (t != nullptr) {
    std::cout << "Tile: " << latMin << " " << lngMin << std::endl;
    std::cout << "max-elev: " << t->maxElevation() << std::endl;
    // find
    for (int lng = 0; lng < t->height() - 1; ++lng)
      for (int lat = 0; lat < t->width() - 1; ++lat) {
        if (t->get(Offsets(lat, lng)) > elev) {
          float distance = (*coordinateSystem)
                  .getLatLng(Offsets(lat, lng))
                  .distance(point);
          if (distance < maxIsolation && distance > 500) {
            maxIsolation = distance;
            higherGround = (*coordinateSystem).getLatLng(Offsets(lat, lng));
            higherGroundElev = t->get(Offsets(lat, lng));
          }
        }
      }
    std::cout << higherGround.latitude() << "," << higherGround.longitude()
              << "," << higherGroundElev << "," << maxIsolation / 1000
              << std::endl;
    std::cout << "Shortest distance to quadrilateral: "
              << shortestDistanceToQuadrillateral(
                     &point, LatLng(latMin + 1, lngMin + 1),
                     LatLng(latMin, lngMin)) /
                     1000
              << std::endl;
  }
  return 0;
}
