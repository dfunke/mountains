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

#include "file_format.h"
#include "degree_coordinate_system.h"
#include "easylogging++.h"
#include "utm_coordinate_system.h"

#include <cassert>
#include <map>

using std::string;

int FileFormat::rawSamplesAcross() const {
  switch (mValue) {
  case Value::NED13_ZIP:
    return 10812;
  case Value::NED1_ZIP:
    return 3612;
  case Value::NED19:
    return 8112;
  case Value::HGT3:
    return 1201;
  case Value::HGT1:
    return 3601;
  case Value::HGT04:
    return 9001;
  case Value::THREEDEP_1M:
    return 10012;
  case Value::HGT_MARS1:
    return 297;
  case Value::HGT_MARS:
    //return 7622;
    return 4446;
  case Value::SLDEM:
    //return 15360;
    //return 7681;
    return 2561;
  default:
    // In particular, fail on GLO, because this number is variable with
    // latitude.
    LOG(ERROR) << "Couldn't compute tile size of unknown file format";
    exit(1);
  }
}

int FileFormat::inMemorySamplesAcross() const {
  switch (mValue) {
  case Value::NED13_ZIP:
    return 10801;
  case Value::NED1_ZIP:
    return 3601;
  case Value::NED19:
    return 8101;
  case Value::HGT3:
    return 1201;
  case Value::HGT1:
    return 3601;
  case Value::HGT04:
    return 9001;
  case Value::THREEDEP_1M:
    return 10001;
  case Value::HGT_MARS1:
    return 297;
  case Value::HGT_MARS:
    return 4446;
  case Value::GLO30: // Fall through
  case Value::FABDEM:
    return 3601;
  case Value::SLDEM:
    //return 15360;
    //return 7681;
    return 2561;
  default:
    LOG(ERROR) << "Couldn't compute tile size of unknown file format";
    exit(1);
  }
}

float FileFormat::degreesAcross() const {
  switch (mValue) {
  case Value::NED13_ZIP:
    return 1.0f;
  case Value::NED1_ZIP:
    return 1.0f;
  case Value::NED19:
    return 0.25f;
  case Value::HGT3:
    return 1.0f;
  case Value::HGT1:
    return 1.0f;
  case Value::HGT04:
    return 1.0f;
  case Value::HGT_MARS1:
    return 1.0f;
  case Value::HGT_MARS:
    //return 25.71428571f;
    return 15.f;
  case Value::GLO30: // Fall through
  case Value::FABDEM:
    return 1.0f;
  case Value::SLDEM:
    return 5.0f;
  case Value::THREEDEP_1M:
    // This is a misnomer, as these tiles are in UTM coordinates.  The "degrees"
    // across means one x or y unit per tile (where each tile is 10000m in UTM).
    return 1.0f;
  default:
    LOG(ERROR) << "Couldn't compute degree span of unknown file format";
    exit(1);
  }
}

bool FileFormat::isUtm() const { return mValue == Value::THREEDEP_1M; }

double FileFormat::getRadius() const {
  switch (mValue) {
  case Value::NED13_ZIP: // fall through
  case Value::NED1_ZIP:
  case Value::NED19:
  case Value::HGT3:
  case Value::HGT1:
  case Value::HGT04:
  case Value::GLO30:
  case Value::THREEDEP_1M:
  case Value::FABDEM:
    return 6371.01 * 1000.0;
  case Value::HGT_MARS:
  case Value::HGT_MARS1:
    return 3396190.0;
  case Value::SLDEM:
    return 1737.4 * 1000.0;
  default:
    return 0;
  }
}

CoordinateSystem *FileFormat::coordinateSystemForOrigin(float lat, float lng,
                                                        int utmZone) const {
  switch (mValue) {
  case Value::NED13_ZIP: // fall through
  case Value::NED1_ZIP:
  case Value::NED19:
  case Value::HGT3:
  case Value::HGT1:
  case Value::HGT04:
  case Value::GLO30:
  case Value::HGT_MARS:
  case Value::HGT_MARS1:
  case Value::SLDEM:
  case Value::FABDEM: {
    // The -1 removes overlap with neighbors

    float samplesPerDegreeLat = (inMemorySamplesAcross() - 1) / degreesAcross();
    float samplesPerDegreeLng = (inMemorySamplesAcross() - 1) / degreesAcross();
    return new DegreeCoordinateSystem(lat, lng, lat + degreesAcross(),
                                      lng + degreesAcross(),
                                      samplesPerDegreeLat, samplesPerDegreeLng);
  }

  case Value::THREEDEP_1M:
    // 10km x 10km tiles, NW corner
    return new UtmCoordinateSystem(utmZone, static_cast<int>(lng * 10000),
                                   static_cast<int>((lat - 1) * 10000),
                                   static_cast<int>((lng + 1) * 10000),
                                   static_cast<int>(lat * 10000), 1.0f);

  default:
    assert(false);
  }

  return nullptr;
}

FileFormat *FileFormat::fromName(const string &name) {
  const std::map<string, FileFormat> fileFormatNames = {
      {
          "SRTM3",
          Value::HGT3,
      },
      {
          "SRTM1",
          Value::HGT1,
      },
      {
          "SRTM04",
          Value::HGT04,
      },
      {
          "NED1-ZIP",
          Value::NED1_ZIP,
      },
      {
          "NED13-ZIP",
          Value::NED13_ZIP,
      },
      {
          "NED19",
          Value::NED19,
      },
      {
          "GLO30",
          Value::GLO30,
      },
      {
          "FABDEM",
          Value::FABDEM,
      },
      {
          "HGT_MARS",
          Value::HGT_MARS,
      },
      {
          "HGT_MARS1",
          Value::HGT_MARS1,
      },
      {
          "3DEP-1M",
          Value::THREEDEP_1M,
      },
      {
          "SLDEM",
          Value::SLDEM,
      },
  };

  auto it = fileFormatNames.find(name);
  if (it == fileFormatNames.end()) {
    return nullptr;
  }

  return new FileFormat(it->second);
}
