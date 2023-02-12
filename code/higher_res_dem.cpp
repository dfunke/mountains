#include "easylogging++.h"
#include "tile.h"

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <set>
#include <chrono>
#include <string>

using std::string;

int main(int argc, char **argv) {
  string terrain_directory("/home/pc/");
  string erg_directory(".");

  float minIsolation = 1;
  int numThreads = 1;
  bool sweepline = false;

  // Parse options
  START_EASYLOGGINGPP(argc, argv);


}


