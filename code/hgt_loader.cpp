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

#include "hgt_loader.h"
#include "tile.h"
#include "util.h"
#include "easylogging++.h"
#include "file_format.h"

#include <stdio.h>
#include <string>
#include <cmath>
#include "assert.h"

using std::string;
using std::floor;
using std::ceil;

static const int16 HGT_NODATA_ELEVATION = -32768;

static uint16 swapByteOrder16(uint16 us) {
  return (us >> 8) | (us << 8);
}

HgtLoader::HgtLoader(FileFormat format) {
  mFormat = format;
}


Tile *incTileRes(Tile *base, FileFormat oldFormat, FileFormat newFormat) {
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
      x2 = std::min(x2, oldFormat.rawSamplesAcross()-1);
      int y1 = floor(y);
      int y2 = ceil(y);
      y2 = std::min(y2, oldFormat.rawSamplesAcross()-1);
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

Tile *HgtLoader::loadTile(const std::string &directory, float minLat, float minLng) {
  char buf[100];
  snprintf(buf, sizeof(buf), "%c%02d%c%03d.hgt",
           (minLat >= 0) ? 'N' : 'S',
           abs(static_cast<int>(minLat)),
           (minLng >= 0) ? 'E' : 'W',
           abs(static_cast<int>(minLng)));
  string filename(buf);
  if (!directory.empty()) {
    filename = directory + "/" + filename;
  }

  FILE *infile = fopen(filename.c_str(), "rb");
  if (infile == nullptr) {
    VLOG(3) << "Failed to open file " << filename;
    return nullptr;
  }
  FileFormat hgt3 = FileFormat(FileFormat::Value::HGT3);
  int num_samples = hgt3.rawSamplesAcross() * hgt3.rawSamplesAcross();
  
  int16 *inbuf = (int16 *) malloc(sizeof(int16) * num_samples);
  
  Tile *retval = nullptr;
  
  int samples_read = static_cast<int>(fread(inbuf, sizeof(int16), num_samples, infile));
  if (samples_read != num_samples) {
    fprintf(stderr, "Couldn't read tile file: %s, got %d samples expecting %d\n",
            filename.c_str(), samples_read, num_samples);
    free(inbuf);
  } else {
    Elevation *samples = (Elevation *) malloc(sizeof(Elevation) * num_samples);
    // SRTM data is in big-endian order; convert to Elevation
    for (int i = 0; i < num_samples; ++i) {
      int16 elevation = swapByteOrder16(inbuf[i]);
      if (elevation == HGT_NODATA_ELEVATION) {
        samples[i] = Tile::NODATA_ELEVATION;
      } else {
        samples[i] = static_cast<Elevation>(elevation);
      }
    }
    retval = new Tile(hgt3.rawSamplesAcross(), hgt3.rawSamplesAcross(), samples, hgt3);
  }

  free(inbuf);
  fclose(infile);
  if (mFormat.rawSamplesAcross() != hgt3.rawSamplesAcross()) {
    // Interpolate
    Tile *interpolatedTile = incTileRes(retval, hgt3, mFormat);
    delete retval;
    return interpolatedTile;
  }

  return retval;
}
