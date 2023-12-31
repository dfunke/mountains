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

#include "latlng.h"

#include "math_util.h"
#include <assert.h>
#include <math.h>
#include <algorithm>

static const float kEarthRadiusMeters = 6371.01 * 1000.0;
static const float kMinLatRadians = (float) -M_PI / 2;
static const float kMaxLatRadians = (float) M_PI / 2;
static const float kMinLngRadians = (float) -M_PI;
static const float kMaxLngRadians = (float) M_PI;

float LatLng::distance(const LatLng &other) const {
  // See http://www.movable-type.co.uk/scripts/latlong.html
  float lat1 = degToRad(mLatitude);
  float lat2 = degToRad(other.latitude());
  float lng1 = degToRad(mLongitude);
  float lng2 = degToRad(other.longitude());

  float deltaLat = lat1 - lat2;
  float deltaLng = lng1 - lng2;
  float a = sinf(deltaLat / 2);
  a = a * a;
  float term = sinf(deltaLng / 2);
  a += term * term * cosf(lat1) * cosf(lat2);
  float c = 2 * atan2f(sqrtf(a), sqrtf(1-a));
  
  const float earthRadius = 6371000;
  return c * earthRadius;
}

float LatLng::distanceEllipsoid(const LatLng &other) const {
  // Implementation from Greg Slayden's code; source unknown
  float lat1 = degToRad(mLatitude);
  float lat2 = degToRad(other.latitude());
  float lng1 = degToRad(mLongitude);
  float lng2 = degToRad(other.longitude());

  // WGS84 ellipsoid
  float majorAxisKm = 6378.137f;
  float flat = 1 / 298.257223563f;
  
  float sinF = sinf((lat1 + lat2) / 2);
  float cosF = cosf((lat1 + lat2) / 2);
  float sinG = sinf((lat1 - lat2) / 2);
  float cosG = cosf((lat1 - lat2) / 2);
  float sinL = sinf((lng1 - lng2) / 2);
  float cosL = cosf((lng1 - lng2) / 2);

  float s = sinG * sinG * cosL * cosL + cosF * cosF * sinL * sinL;
  float c = cosG * cosG * cosL * cosL + sinF * sinF * sinL * sinL;
  float w = atan2f(sqrtf(s), sqrtf(c));
  float r = sqrtf(s * c) / w;
  float distance = ((2 * w * majorAxisKm) *
                    (1 + flat * ((3 * r - 1) / (2 * c)) * (sinF * sinF * cosG * cosG) -
                     (flat * ((3 * r + 1) / (2 * s)) * cosF * cosF * sinG * sinG)));
  return distance * 1000;  // meters
}

float LatLng::bearingTo(const LatLng &other) const {
  // See http://www.movable-type.co.uk/scripts/latlong.html
  float lat1 = degToRad(mLatitude);
  float lat2 = degToRad(other.latitude());
  float lng1 = degToRad(mLongitude);
  float lng2 = degToRad(other.longitude());

  float deltaLng = lng1 - lng2;

  float term1 = sinf(deltaLng) * cosf(lat2);
  float term2 = cosf(lat1) * sinf(lat2) - sinf(lat1) * cosf(lat2) * cosf(deltaLng);
  return atan2f(term1, term2);
}

std::vector<LatLng> LatLng::GetBoundingBoxForCap(float distance_meters) const {
   assert(distance_meters >= 0);
   
   // angular distance in radians on a great circle
   float radDist = distance_meters / kEarthRadiusMeters;
   float radLat = degToRad(mLatitude);
   float radLon = degToRad(mLongitude);
   float minLat = radLat - radDist;
   float maxLat = radLat + radDist;
   
   float minLon, maxLon;
   if (minLat > kMinLatRadians && maxLat < kMaxLatRadians) {
      float deltaLon = asinf(sinf(radDist) / cosf(radLat));
      minLon = radLon - deltaLon;
      if (minLon < kMinLngRadians) minLon += static_cast<float>(2 * M_PI);
      maxLon = radLon + deltaLon;
      if (maxLon > kMaxLngRadians) maxLon -= static_cast<float>(2 * M_PI);
   } else {
      // a pole is within the distance
      minLat = std::max(minLat, kMinLatRadians);
      maxLat = std::min(maxLat, kMaxLatRadians);
      minLon = kMinLngRadians;
      maxLon = kMaxLngRadians;
   }

   std::vector<LatLng> box;
   box.push_back(LatLng(radToDeg(minLat), radToDeg(minLon)));
   box.push_back(LatLng(radToDeg(maxLat), radToDeg(maxLon)));
   return box;
}

vec3 LatLng::toCartesian() const {
  vec3 res;
  res.x = cos(degToRad(mLatitude)) * cos(degToRad(mLongitude));
  res.y = cos(degToRad(mLatitude)) * sin(degToRad(mLongitude));
  res.z = sin(degToRad(mLatitude));
  // res is normalized
  return res;
}

LatLng::LatLng(vec3 cartesian) {
  // assert catesian is normlized
  mLatitude = radToDeg(asin(cartesian.z));
  mLongitude = radToDeg(atan2(cartesian.y, cartesian.x));
}