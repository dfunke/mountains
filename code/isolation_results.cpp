/*
 * MIT License
 * 
 * Copyright (c) 2017 Andrew Kirmse
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "isolation_results.h"
#include <limits.h>

using std::string;
using std::vector;

IsolationResults::IsolationResults() {
}

void IsolationResults::addResult(const LatLng &peakLocation, Elevation elevation,
                                 const LatLng &higherLocation, float isolationKm) {
  IsolationResult result;
  result.peak = peakLocation;
  result.peakElevation = elevation;
  result.higher = higherLocation;
  result.isolationKm = isolationKm;

  mResults.push_back(result);
}

bool IsolationResults::save(const string &directory, float lat, float lng) const {
  string filename = directory + "/" + filenameForCoordinates(lat, lng);
  FILE *file = fopen(filename.c_str(), "w");
  if (file == nullptr) {
    printf("Couldn't open file %s for writing\n", filename.c_str());
    return false;
  }

  for (auto it : mResults) {
    fprintf(file, "%.4f,%.4f,%.2f,%.4f,%.4f,%.4f\n",
            it.peak.latitude(), it.peak.longitude(), it.peakElevation,
            it.higher.latitude(), it.higher.longitude(),
            it.isolationKm);
  }
  
  fclose(file);
  return true;
}

bool IsolationResults::saveSl(const string &directory, float minLat, float minLng, float maxLat, float maxLng) const {
  char filename[PATH_MAX];
  // TODO: Maybe support sub-degree tiles one day, if anyone cares
  sprintf(filename, "all-isolations-%02d-%03d-to-%02d-%03d.txt",
          static_cast<int>(minLat), static_cast<int>(minLng), static_cast<int>(maxLat), static_cast<int>(maxLng));
    
  string path = directory + "/" + filename;
  FILE *file = fopen(path.c_str(), "w");
  if (file == nullptr) {
    printf("Couldn't open file %s for writing\n", path.c_str());
    return false;
  }

  for (auto it : mResults) {
    fprintf(file, "%.4f,%.4f,%.2f,%.4f,%.4f,%.4f\n",
            it.peak.latitude(), it.peak.longitude(), it.peakElevation,
            it.higher.latitude(), it.higher.longitude(),
            it.isolationKm);
  }
  
  fclose(file);
  return true;
}

IsolationResults *IsolationResults::loadFromFile(const string &directory, float lat, float lng) {
  string filename = directory + "/" + filenameForCoordinates(lat, lng);
  FILE *file = fopen(filename.c_str(), "r");
  if (file == nullptr) {
    printf("Couldn't open file %s for reading\n", filename.c_str());
    return nullptr;
  }

  vector<IsolationResult> results;

  IsolationResult result;
  while (!feof(file)) {
    float peakLatitude, peakLongitude, higherLatitude, higherLongitude;
    if (fscanf(file, "%f,%f,%f,%f,%f,%f\n",
               &peakLatitude, &peakLongitude, &result.peakElevation,
               &higherLatitude, &higherLongitude,
               &result.isolationKm) != 6) {
        return nullptr;
    }
    result.peak = LatLng(peakLatitude, peakLongitude);
    result.higher = LatLng(higherLatitude, higherLongitude);
    results.push_back(result);
  }

  IsolationResults *obj = new IsolationResults();
  obj->mResults = results;
  return obj;
}

string IsolationResults::filenameForCoordinates(float lat, float lng) {
  char filename[PATH_MAX];
  // TODO: Maybe support sub-degree tiles one day, if anyone cares
  sprintf(filename, "isolation-%02d-%03d.txt",
          static_cast<int>(lat), static_cast<int>(lng));
  return filename;
}
