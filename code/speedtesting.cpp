#include "coordinate_system.h"
#include "easylogging++.h"
#include "isolation_finder_sl/isolation_sl_processor.h"
#include "isolation_task.h"
#include "point_map.h"
#include "tile.h"
#include "tile_loading_policy.h"

#include "ThreadPool.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <iostream>
#include <ratio>

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

INITIALIZE_EASYLOGGINGPP

using std::ceil;
using std::floor;
using std::string;

static const string baseFolder = "/data02/funke/SRTM/viewfinderpanoramas.org/dem3"; 
static const string baseFolderDem1 = "/data02/funke/SRTM/viewfinderpanoramas.org/dem1"; 
static const string testFolder = "/data02/funke/SRTM/SRTM"; 
static const string testResultFile = "/home/huening/testresults-world-dem3-randomsample.txt";

// static const string baseFolder = "/home/pc/Data2/SRTM-US-DEM3";
// static const string baseFolderDem1 = "/home/pc/SRTM-DEM1";
// static const string testFolder = "/home/pc/SRTM";
// static const string testResultFile = "/home/pc/tmp/testresults.txt";

struct DynamicTestCase {
  Offsets centerTile;
  int tileNumber;
};

struct TestCase {
  TestCase(float minLat, float maxLat, float minLng, float maxLng, int size) {
    this->minLat = minLat;
    this->maxLat = maxLat;
    this->minLng = minLng;
    this->maxLng = maxLng;
    this->size = size;
  }
  float minLat;
  float minLng;
  float maxLat;
  float maxLng;
  int size;
};

vector<TestCase> getTestCases() {
  vector<TestCase> testCases;
  testCases.push_back(TestCase(75, 90, 151, 180, 4));
  testCases.push_back(TestCase(74, 90, 149, 180, 9));
  testCases.push_back(TestCase(72, 90, 145, 180, 21));
  testCases.push_back(TestCase(71, 90, 145, 180, 35));
  testCases.push_back(TestCase(70, 90, 144, 180, 62));
  testCases.push_back(TestCase(68, 90, 142, 180, 151));
  testCases.push_back(TestCase(66, 90, 140, 180, 249));
  testCases.push_back(TestCase(61, 90, 132, 180, 550));
  testCases.push_back(TestCase(53, 90, 118, 180, 1025));
  testCases.push_back(TestCase(42, 90, 98, 180, 2027));
  testCases.push_back(TestCase(26, 90, 67, 180, 4094));
  testCases.push_back(TestCase(0, 90, 17, 180, 8245));
  testCases.push_back(TestCase(-55, 90, -91, 180, 16387));
  testCases.push_back(TestCase(-90, 90, -180, 180, 26095));
  return testCases;
}

vector<TestCase> getHimalayTestCase() {
  vector<TestCase> testCases;
  testCases.push_back(TestCase(34, 36, 88, 90, 4));
  testCases.push_back(TestCase(33, 36, 87, 90, 9));
  testCases.push_back(TestCase(32, 36, 86, 90, 16));
  testCases.push_back(TestCase(30, 36, 84, 90, 32));
  testCases.push_back(TestCase(28, 36, 82, 90, 64));
  testCases.push_back(TestCase(25, 36, 78, 90, 132));
  testCases.push_back(TestCase(21, 36, 72, 90, 270));
  return testCases;
}

vector<TestCase> getAtlanticTestCase() {
  vector<TestCase> testCases;
  testCases.push_back(TestCase(39, 52, -35, -10, 4));
  testCases.push_back(TestCase(37, 52, -39, -10, 8));
  testCases.push_back(TestCase(30, 52, -52, -10, 14));
  testCases.push_back(TestCase(28, 52, -54, -10, 32));
  testCases.push_back(TestCase(26, 52, -58, -10, 66));
  testCases.push_back(TestCase(22, 52, -64, -10, 131));
  testCases.push_back(TestCase(17, 52, -72, -10, 270));
  return testCases;
}

vector<TestCase> getSouthTestCase() {
  vector<TestCase> testCases;
  testCases.push_back(TestCase(-3, 0, 175, 180, 4));
  testCases.push_back(TestCase(-6, 0, 169, 180, 8));
  testCases.push_back(TestCase(-10, 0, 162, 180, 17));
  testCases.push_back(TestCase(-12, 0, 160, 180, 31));
  testCases.push_back(TestCase(-15, 0, 154, 180, 65));
  testCases.push_back(TestCase(-18, 0, 148, 180, 128));
  testCases.push_back(TestCase(-22, 0, 142, 180, 266));
  testCases.push_back(TestCase(-27, 0, 134, 180, 498));
  testCases.push_back(TestCase(-34, 0, 120, 180, 1026));
  testCases.push_back(TestCase(-82, 0, 28, 180, 4087));
  return testCases;
}

vector<TestCase> getUsTestCase() {
  vector<TestCase> testCases;
  testCases.push_back(TestCase(45, 47, -70, -68, 4));
  testCases.push_back(TestCase(44, 47, -71, -68, 9));
  testCases.push_back(TestCase(43, 47, -72, -68, 16));
  testCases.push_back(TestCase(41, 47, -74, -68, 33));
  testCases.push_back(TestCase(39, 47, -77, -68, 59));
  testCases.push_back(TestCase(36, 47, -82, -68, 121));
  testCases.push_back(TestCase(32, 47, -89, -68, 247));
  testCases.push_back(TestCase(23, 47, -106, -68, 502));
  return testCases;
}

TestCase getNorthAmerika() { return TestCase(12, 90, -168, -51, 502); }

bool fileExists(const char *fileName) {
  FILE *file;
  if ((file = fopen(fileName, "r"))) {
    fclose(file);
    return true;
  }
  return false;
}

void writeToTestResults(std::size_t tileCount, double oldTime, double newTime) {
  std::ofstream outfile;
  outfile.open(testResultFile.c_str(), std::ios_base::app);
  outfile << tileCount << "," << oldTime << "," << newTime << std::endl;
  outfile.close();
}

int copyTile(int lat, int lng) {
  char buf[100];
  sprintf(buf, "%c%02d%c%03d.hgt", (lat >= 0) ? 'N' : 'S', abs(lat),
          (lng >= 0) ? 'E' : 'W', abs(lng));
  string command(buf);
  string testFile = testFolder + "/" + command;
  if (fileExists(testFile.c_str())) {
    // to not copy if file already exists.
    return 0;
  }
  command = baseFolder + "/" + command;
  if (fileExists(command.c_str())) {
    command = "cp " + command + " " + testFolder;
    // command = "cp /home/pc/Data2/SRTM-DEM1/" + command + "
    // /home/pc/SRTM/";
    int success = system(command.c_str());
    if (success < 0) {
      std::cout << "copiing DEM-File" << std::endl;
    }
    return 1;
  }
  return 0;
}

float *setupSrtmFolder(DynamicTestCase testCase) {
  int r = 0;
  int i = 0;
  int j = 0;
  int counter = 0;
  int addToJ = 1;
  float *bounds = new float[4];
  bounds[0] = 180.f;
  bounds[1] = -180.f;
  bounds[2] = 180.f;
  bounds[3] = -180.f;
  for (int r = 0; counter != testCase.tileNumber; r++) {
    for (i = -r; i <= r; ++i) {
      if (i == abs(-r)) {
        addToJ = 1;
      } else {
        addToJ = 2 * r + 1;
      }
      for (j = -r; j <= r; j += addToJ) {
        int lat = i + testCase.centerTile.x();
        if (lat >= 90) {
          lat = 89;
        }
        if (lat < -90) {
          lat = -90;
        }
        int lng = j + testCase.centerTile.y();
        if (lng >= 180) {
          lng = 179;
        }
        if (lng < -180) {
          lng = -180;
        }
        bounds[0]= std::min(lat+0.f, bounds[0]);
        bounds[1]= std::max(lat+0.f, bounds[1]);
        bounds[2]= std::min(lng+0.f, bounds[2]);
        bounds[3]= std::max(lng+0.f, bounds[3]);

        counter += copyTile(lat, lng);
        if (counter == testCase.tileNumber) {

          bounds[1] = bounds[1]+1;
          bounds[3] = bounds[3] +1;
          return bounds;
        }
      }
      if (i > 180) {
      }
    }
    if (r > 360) {
      // not enoth tiles
      return nullptr;
    }
  }
  return bounds;
}

void cleanSrtmFolder() {
  string cleanCommand = "rm " + testFolder + "/*";
  int success = system(cleanCommand.c_str());
  if (success < 0) {
    std::cout << "Error removing" << std::endl;
  }
}

int setupSrtmFolder(float *bounds) {
  int counter = 0;
  cleanSrtmFolder();
  for (int lat = (int)floor(bounds[0]); lat < (int)ceil(bounds[1]); ++lat) {
    for (int lng = (int)floor(bounds[2]); lng < (int)ceil(bounds[3]); ++lng) {
      counter += copyTile(lat, lng);
    }
  }
  return counter;
}

double runTest(FileFormat fileFormat, float bounds[], bool old) {
  int threads = 1;
  using namespace std::chrono;
  BasicTileLoadingPolicy policy(testFolder.c_str(), fileFormat);
  const int CACHE_SIZE = 50;
  double times = 0;
  double oldTimes = 0;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  TileCache *cache = new TileCache(&policy, CACHE_SIZE);
  PeakNumbers pNumbers;
  if (old) {
    ThreadPool *threadPool = new ThreadPool(threads);
    vector<std::future<bool>> results;
    for (int lat = (int)floor(bounds[0]); lat < (int)ceil(bounds[1]); ++lat) {
      for (int lng = (int)floor(bounds[2]); lng < (int)ceil(bounds[3]); ++lng) {
        std::shared_ptr<CoordinateSystem> coordinateSystem(
            fileFormat.coordinateSystemForOrigin(lat + 0.f, lng + 0.f));
        IsolationTask *task = new IsolationTask(cache, "~/tmp", bounds, 1);
        results.push_back(threadPool->enqueue([=] {
          return task->run(lat, lng, *coordinateSystem, fileFormat);
        }));
      }
    }
    int num_tiles_processed = 0;
    for (auto &&result : results) {
      if (result.get()) {
        num_tiles_processed += 1;
      }
    }
    delete threadPool;
  } else {
    IsolationSlProcessor *finder = new IsolationSlProcessor(cache, fileFormat);
    pNumbers = finder->findIsolations(threads, bounds, 1);
  }
  delete cache;
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
  if (!old) {
    std::cout << pNumbers.pixelCount << "," << pNumbers.finalPeakCount << "," << pNumbers.totalPeakCount;
  }
  return time_span.count();
}

int conductSpeedComparrisonTests() {
  using namespace std::chrono;
  FileFormat fileFormat(FileFormat::Value::HGT3);
  BasicTileLoadingPolicy policy(testFolder.c_str(), fileFormat);
  for (auto testCase : getUsTestCase()) {
    float bounds[4] = {testCase.minLat, testCase.maxLat, testCase.minLng,
                       testCase.maxLng};
    // float bounds[4] = {34, (34.f + t/d), -118,(-118.f + t)};
    // float bounds[4] = {47, (47.f + t/d), 1,(1.f + t)};

    int tileNumber = setupSrtmFolder(bounds);
    std::cout << "Start Processing " << bounds[0] << " " << bounds[1] << " "
              << bounds[2] << " " << bounds[3] << " " << std::endl;
    double oldTime = 0;
    double newTime = 0;
    bool old = false;
    for (int i = 0; i < 2; i++) {
      if (i == 0) {
        old = rand() % 2;
      } else {
        old = !old;
      }
      double time = runTest(fileFormat, bounds, old);
      if (old) {
        oldTime += time;
      } else {
        newTime += time;
      }
    }
    writeToTestResults(tileNumber, oldTime, newTime);
  }
  return 0;
}

 //testCases.push_back(TestCase(23, 47, -106, -68, 502));
int conductRandomSampleComparrisonTests() {
  int MAX_TILE_COUNT = 26095;
  //int MAX_TILE_COUNT = 502;
  FileFormat fileFormat(FileFormat::Value::HGT3);
  int maxTiles = 0;
  int testCases = 5;
  bool old = false;
  for (int n = 12; std::pow(2, n) < MAX_TILE_COUNT; n++) {
    if (n > 9) {
      testCases = 9-n/2;
    } else {
      testCases = (12-n) * (12-n) / 2;
    }
    double oldTime = 0;
    double newTime = 0;
    double maxOld = 0;
    double minOld = 100000000000000.f;
    double maxNew = 0;
    double minNew = 100000000000000.f;
    for (int j = 1; j <= testCases; j++) {
      int lat = rand() % 180 - 90;
      int lng = rand() % 360 - 180;
      // int lat = rand() % 24 + 23;
      // int lng = rand() % 38 - 106;
      auto testCase = DynamicTestCase();
      testCase.centerTile = Offsets(lat, lng);
      testCase.tileNumber = std::pow(2, n);
      std::cout << testCase.tileNumber << "," << lat << "," << lng << ",";
      cleanSrtmFolder();
      float* bounds = setupSrtmFolder(testCase);
      double oldTimeRun = 0;
      double newTimeRun = 0;
      for (int i = 0; i < 2; i++) {
        if (i == 0) {
          old = rand() % 2;
        } else {
          old = !old;
        }
        double time = runTest(fileFormat, bounds, old);
        if (old) {
          oldTime += time;
	  oldTimeRun = time;
          minOld = std::min(time, minOld);
          maxOld = std::max(time, maxOld);
        } else {
          newTime += time;
	  newTimeRun = time;
          minNew = std::min(time, minNew);
          maxNew = std::max(time, maxNew);
        }
      }
      std::cout << "," << oldTimeRun << "," << newTimeRun << std::endl;
      delete [] bounds;
    }
    oldTime /= testCases;
    newTime /= testCases;
    //std::cout << "maxMin:" << maxOld << "," << minOld << "," << maxNew << "," << minNew << std::endl;
    //std::cout << oldTime << "," << newTime << std::endl;
    writeToTestResults(std::pow(2,n), oldTime, newTime);
  }
  return 0;
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);
  return conductRandomSampleComparrisonTests();
  // return conductSpeedComparrisonTests();
  // return testCaseWithDem1Data();
  TestCase testCase = getNorthAmerika();
  float bounds[4] = {testCase.minLat, testCase.maxLat, testCase.minLng,
                     testCase.maxLng};
  setupSrtmFolder(bounds);
  // return testSpecificArea();
}
