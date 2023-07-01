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

#include "tile.h"
#include "math_util.h"
#include "util.h"
#include "file_format.h"

#include "easylogging++.h"

#include <algorithm>
#include <assert.h>
#include <stdlib.h>

Tile::Tile(int width, int height, Elevation *samples, FileFormat format)
{
  mWidth = width;
  mHeight = height;
  mSamples = samples;
  mFormat = format;

  mMaxElevation = 0;

  recomputeMaxElevation();
}

Tile::~Tile()
{
  free(mSamples);
}

void Tile::flipElevations()
{
  for (int i = 0; i < mWidth * mHeight; ++i)
  {
    Elevation elev = mSamples[i];
    if (elev != NODATA_ELEVATION)
    {
      mSamples[i] = -elev;
    }
  }
}

void Tile::recomputeMaxElevation()
{
  mMaxElevation = computeMaxElevation();
}

Elevation Tile::computeMaxElevation() const
{
  Elevation max_elevation = 0;
  for (int i = 0; i < mWidth * mHeight; ++i)
  {
    Elevation elev = mSamples[i];
    max_elevation = std::max(max_elevation, elev);
  }

  return max_elevation;
}

void Tile::saveAsImage(std::string dir, float lat, float lng)
{
  FILE *imageFile;
  int height = mHeight, width = mWidth;
  char buf[100];
  snprintf(buf, sizeof(buf), "%c%02d%c%03d.ppm",
             (lat >= 0) ? 'N' : 'S',
           abs(static_cast<int>(lat)),
           (lng >= 0) ? 'E' : 'W',
           abs(static_cast<int>(lng)));
  std::string file(buf);
  file = dir + "/"  + file;
  imageFile = fopen(file.c_str(), "wb");
  if (imageFile == NULL)
  {
    perror("ERROR: Cannot open output file");
    exit(EXIT_FAILURE);
  }

  fprintf(imageFile, "P6\n");                   // P6 filetype
  fprintf(imageFile, "%d %d\n", width, height); // dimensions
  fprintf(imageFile, "255\n");                  // Max pixel
  Elevation minElev = 1e10;

  for (int j = 0; j < height; ++j)
  {
    for (int i = 0; i < width; ++i)
    {
      if (get(i,j) > NODATA_ELEVATION && get(i,j) < minElev) {
        minElev = get(i,j);
      }
    }
  }
  Elevation maxElev = mMaxElevation - minElev;

  //unsigned char pix[width * height * 3];
  uint64_t size = width * height * 3.0;
  unsigned char *pix = (unsigned char*)malloc(size * sizeof(char));
  std::fill(pix, pix + size, 0);

  for (int i = 0; i < width; ++i)
  {
    for (int j = 0; j < height; ++j)
    {
      Elevation elev = get(i, j);
      elev -= minElev;
      if (elev < 0) {
        elev = 0;
      }
      float ratio = (2.f * elev) / (1.f * maxElev);

      int b = std::max(0, (int)(255 * (1 - ratio)));
      int r = std::max(0, (int)(255 * (ratio - 1)));
      int g = 255 - b - r;
      uint64_t idx = (j * width + i) * 3;
      pix[idx] = r;
      pix[idx + 1] = g;
      pix[idx + 2] = b;
    }
  }
  fwrite(pix, 1, size, imageFile);
  fclose(imageFile);
  free(pix);
}

Elevation Tile::minSorrounding(int x, int y, int space) const {
  if (x - space < 0 ||
      y - space < 0 ||
      x + space >= mHeight ||
      y + space >= mWidth ) {
    return NODATA_ELEVATION;
  }
  Elevation minSorrounding = get(x,y);
  if (minSorrounding == NODATA_ELEVATION) {
    // Ignore remaining no data elevation
    return NODATA_ELEVATION;
  }
  Elevation elev = get(x + space, y);
  if (elev < minSorrounding && elev != NODATA_ELEVATION) {
    minSorrounding = elev;
  }
  elev = get(x, y + space);
  if (elev < minSorrounding && elev != NODATA_ELEVATION) {
    minSorrounding = elev;
  }
  elev = get(x - space, y);
  if (elev < minSorrounding && elev != NODATA_ELEVATION) {
    minSorrounding = elev;
  }
  elev = get(x, y - space);
  if (elev < minSorrounding && elev != NODATA_ELEVATION) {
    minSorrounding = elev;
  }
  return minSorrounding;
}
