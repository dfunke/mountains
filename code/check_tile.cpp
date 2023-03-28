#include "coordinate_system.h"
#include "easylogging++.h"
#include "isolation_results.h"
#include "primitives.h"
#include "tile.h"
#include "tile_cache.h"
#include "tile_loading_policy.h"

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
  float ilpDistance;
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
        IsolationResultDiff resDiff;
        resDiff.r1 = res;
        resDiff.r2 = resInOld;
        resDiff.diff = resInOld.isolationKm - res.isolationKm;
        if (resDiff.diff > 0.5 || resDiff.diff < -0.5) {
          resDiff.ilpDistance = resInOld.higher.distanceEllipsoid(res.higher) / 1000;
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
  string filename = "/home/pc/tmp/peak_diffs.txt";
  FILE *outFile = fopen(filename.c_str(), "w");
  fprintf(outFile, "peakLat,peakLng,ilp1Lat,ilp1Lng,isolation1,ilp2Lat,ilp2Lng,isolation2,ilpDistance,diff\n");
  for (auto &diff : diffs) {
    fprintf(outFile, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
            diff.r1.peak.latitude(), diff.r1.peak.longitude(),
            diff.r1.higher.latitude(), diff.r1.higher.longitude(),
            diff.r1.isolationKm, diff.r2.higher.latitude(),
            diff.r2.higher.longitude(), diff.r2.isolationKm, diff.ilpDistance, diff.diff);
  }
  fclose(outFile);
  return 0;
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  return compare();
  // return mergeOldResults();

  int ch;
  string terrain_directory("/home/pc/Data2/SRTM-DEM3/");
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

  int latMin = (int)floor(bounds[2]);
  int lngMin = (int)floor(bounds[3]);

  LatLng point(bounds[0], bounds[1]);
  FileFormat fileFormat(FileFormat::Value::HGT3);
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
    std::cout << t->maxElevation() << std::endl;
    // find
    for (int lng = 0; lng < t->height() - 1; ++lng)
      for (int lat = 0; lat < t->width() - 1; ++lat) {
        if (t->get(Offsets(lat, lng)) > elev) {
          if ((*coordinateSystem)
                  .getLatLng(Offsets(lat, lng))
                  .distanceEllipsoid(point) < maxIsolation) {
            maxIsolation = (*coordinateSystem)
                               .getLatLng(Offsets(lat, lng))
                               .distanceEllipsoid(point);
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
