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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "tile.h"
#include "file_format.h"
#include "math_util.h"
#include "primitives.h"
#include "util.h"

#include "easylogging++.h"

#include <algorithm>
#include <assert.h>
#include <cstdint>
#include <stdlib.h>

Tile::Tile(int width, int height, Elevation *samples, FileFormat format) {
  mWidth = width;
  mHeight = height;
  mSamples = samples;
  mFormat = format;

  mMaxElevation = 0;

  recomputeMaxElevation();
}

Tile::~Tile() { free(mSamples); }

void Tile::flipElevations() {
  for (int i = 0; i < mWidth * mHeight; ++i) {
    Elevation elev = mSamples[i];
    if (elev != NODATA_ELEVATION) {
      mSamples[i] = -elev;
    }
  }
}

void Tile::recomputeMaxElevation() { mMaxElevation = computeMaxElevation(); }

Elevation Tile::computeMaxElevation() const {
  Elevation max_elevation = 0;
  for (int i = 0; i < mWidth * mHeight; ++i) {
    Elevation elev = mSamples[i];
    max_elevation = std::max(max_elevation, elev);
  }

  return max_elevation;
}

void Tile::saveAsImage(float minLat, float minLng) {
  FILE *imageFile;
  int height = mHeight, width = mWidth;
  char buf[100];
  snprintf(buf, sizeof(buf), "%c%02d%c%03d.ppm", (minLat >= 0) ? 'N' : 'S',
           abs(static_cast<int>(minLat)), (minLng >= 0) ? 'E' : 'W',
           abs(static_cast<int>(minLng)));
  std::string filename(buf);
  imageFile = fopen(filename.c_str(), "wb");
  if (imageFile == NULL) {
    perror("ERROR: Cannot open output file");
    exit(EXIT_FAILURE);
  }

  fprintf(imageFile, "P6\n");                   // P6 filetype
  fprintf(imageFile, "%d %d\n", width, height); // dimensions
  fprintf(imageFile, "255\n");                  // Max pixel

  unsigned char pix[width * height * 3] = {0};

  std::size_t maxCounter = 0;
  Elevation minElev = INT16_MAX;

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      Elevation elev = get(i, j);
      if (elev > 0 && elev < minElev) {
        minElev = elev;
      }
    }
  }
  Elevation maxElev = maxElevation() - minElev;
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      Elevation elev = get(i, j);
      if (elev < 0) {
        elev = 0;
      }
      elev = elev - minElev;
      float ratio = (2.f * elev / (1.f * maxElev));
      int b = std::max(0, (int)(255 * (1 - ratio)));
      int r = std::max(0, (int)(255 * (ratio - 1)));
      int g = 255 - b - r;
      pix[(i * height + j) * 3] = r;
      pix[(i * height + j) * 3 + 1] = g;
      pix[(i * height + j) * 3 + 2] = b;
    }
  }
  fwrite(pix, 1, 3 * width * height, imageFile);
  fclose(imageFile);
  std::cout << "Max Counter: " << maxCounter << std::endl;
}
