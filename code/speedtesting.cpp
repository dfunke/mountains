#include "coordinate_system.h"
#include "easylogging++.h"
#include "isolation_finder_sl/isolation_finder_sl.h"
#include "isolation_finder_sl/isolation_sl_processor.h"
#include "isolation_task.h"
#include "point_map.h"
#include "tile.h"
#include "tile_loading_policy.h"

#include "ThreadPool.h"
#include <chrono>
#include <cmath>
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

// static const string baseFolder =
// "/data02/funke/SRTM/viewfinderpanoramas.org/dem1"; static const string
// baseFolderDem1 = "/data02/funke/SRTM/viewfinderpanoramas.org/dem1"; static
// const string testFolder = "/home/huening/SRTM"; static const string
// testResultFile = "/home/huening/testresults.txt";

static const string baseFolder = "/home/pc/Data2/SRTM-DEM3";
static const string baseFolderDem1 = "/home/pc/Data2/SRTM-DEM1";
static const string testFolder = "/home/pc/SRTM";
static const string testResultFile = "/home/pc/tmp/testresults.txt";

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

int setupSrtmFolder(float *bounds) {
  int counter = 0;
  string cleanCommand = "rm " + testFolder + "/*";
  int success = system(cleanCommand.c_str());
  if (success < 0) {
    std::cout << "Error removing" << std::endl;
  }
  for (int lat = (int)floor(bounds[0]); lat < (int)ceil(bounds[1]); ++lat) {
    for (int lng = (int)floor(bounds[2]); lng < (int)ceil(bounds[3]); ++lng) {
      char buf[100];
      sprintf(buf, "%c%02d%c%03d.hgt", (lat >= 0) ? 'N' : 'S', abs(lat),
              (lng >= 0) ? 'E' : 'W', abs(lng));
      string command(buf);
      command = baseFolder + "/" + command;
      if (fileExists(command.c_str())) {
        command = "cp " + command + " " + testFolder;
        // command = "cp /home/pc/Data2/SRTM-DEM1/" + command + "
        // /home/pc/SRTM/";
        success = system(command.c_str());
        if (success < 0) {
          std::cout << "copiing DEM-File" << std::endl;
        }
        counter++;
      }
    }
  }
  return counter;
}

int conductSpeedComparrisonTests() {
  using namespace std::chrono;
  int threads = 1;
  bool old = false;
  FileFormat fileFormat(FileFormat::Value::HGT3);
  BasicTileLoadingPolicy policy(testFolder.c_str(), fileFormat);
  const int CACHE_SIZE = 50;
  auto setupCache = std::make_unique<TileCache>(&policy, CACHE_SIZE);
  for (auto testCase : getUsTestCase()) {
    double times = 0;
    double oldTimes = 0;
    float bounds[4] = {testCase.minLat, testCase.maxLat, testCase.minLng,
                       testCase.maxLng};
    // float bounds[4] = {34, (34.f + t/d), -118,(-118.f + t)};
    // float bounds[4] = {47, (47.f + t/d), 1,(1.f + t)};
    setupSrtmFolder(bounds);
    std::cout << "Start Processing " << bounds[0] << " " << bounds[1] << " "
              << bounds[2] << " " << bounds[3] << " " << std::endl;
    double finalPhaseOneSetup = 0;
    double finalPhaseOneSl = 0;
    double finalPhaseTwo = 0;
    double finalPhaseThreeSetup = 0;
    double finalPhaseThreeSl = 0;

    int testAmmount = 1;
    for (int j = 0; j < testAmmount; j++) {
      TileCache *cache = new TileCache(&policy, CACHE_SIZE);
      cache->resetLoadingTime();
      IsolationSlProcessor *finder =
          new IsolationSlProcessor(cache, fileFormat);
      IsolationResults res = finder->findIsolations(threads, bounds, 1);
      delete cache;
      finalPhaseOneSetup += phaseOneTimeSetup;
      finalPhaseOneSl += phaseOneTimeSw;
      finalPhaseTwo += phaseThreeTime;
      finalPhaseThreeSetup += phaseTwoTimeSetup;
      finalPhaseThreeSl += phaseTwoTimeSw;
    }
    finalPhaseOneSetup /= testAmmount;
    finalPhaseOneSl /= testAmmount;
    finalPhaseTwo /= testAmmount;
    finalPhaseThreeSetup /= testAmmount;
    finalPhaseThreeSl /= testAmmount;
    printf("%.5f,%.5f,%.5f,%.5f,%.5f", finalPhaseOneSetup, finalPhaseOneSl,
           finalPhaseTwo, finalPhaseThreeSetup, finalPhaseThreeSl);
  }
  return 0;
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);
  return conductSpeedComparrisonTests();
  // return testCaseWithDem1Data();
  TestCase testCase = getNorthAmerika();
  float bounds[4] = {testCase.minLat, testCase.maxLat, testCase.minLng,
                     testCase.maxLng};
  setupSrtmFolder(bounds);
  // return testSpecificArea();
}
