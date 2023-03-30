#ifndef _SPHERICAL_MATH_UTIL_H_
#define _SPHERICAL_MATH_UTIL_H_

#include "../latlng.h"
#include "sweepline_primitives.h"

#include <functional>
#include <assert.h>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

using glm::vec3;

inline float toLongitudeOnOtherSideOfSphere(float n)
{
  return n < 0 ? n + 180 : n - 180;
}

inline float searchDistance(const LatLng *p1, const LatLng p2)
{
  // For now, more intelligent / faster method later
  // return p1->distanceEllipsoid(p2);
  return p1->distance(p2);
}

inline float fastSearchDistance(const Offsets p1, const Offsets p2, float *lngDistanceScale)
{
  int averageY = (p1.y() + p2.y()) / 2;
  float lngScaleFactor = lngDistanceScale[averageY];
  float yDistanceComponent = static_cast<float>((p1.y() - p2.y()) * (p1.y() - p2.y()));
  float deltaX = (p1.x() - p2.x()) * lngScaleFactor;
  return deltaX * deltaX + yDistanceComponent;
}

inline LatLng nearestPointOnGreatCircle(const LatLng start, const LatLng end, const LatLng *point)
{
  assert(start.longitude() == end.longitude());
  assert(start.latitude() < end.latitude());
  // Sollution from https://stackoverflow.com/questions/1299567/how-to-calculate-distance-from-a-point-to-a-line-segment-on-a-sphere
  vec3 cStart = start.toCartesian();
  vec3 cEnd = end.toCartesian();
  vec3 cPoint = point->toCartesian();
  vec3 A =  glm::cross(cStart, cEnd);
  vec3 B = glm::cross(cPoint, A);
  vec3 C = glm::cross(A,B);
  C = glm::normalize(C);
  LatLng nearest(C);
  return nearest;
}

inline float shortestDistanceToGreatCircleSegmet(const LatLng start, const LatLng end, const LatLng *point)
{
  LatLng nearest = nearestPointOnGreatCircle(start, end, point);

  // Check if nearest on line-segment, asserts that line is longitude
  float centerLatitude = (start.latitude() + end.latitude()) / 2.f;
  
  // Because latitude great-circle are devided in 2 pars
  // For this we assume that 0 is at the side of start and end,
  // and then we modify the latitude to be between -180° and 180°
  // respective to this 0 point.
  // check if on other side of the spere
  float nearestLatitude = nearest.latitude();
  // ony same or 180° jump. Work with 1 degree of error
  // 
  if (abs(nearest.longitude() - start.longitude()) > 1 && abs(nearest.longitude() - start.longitude()) < 359) {
    if (nearest.latitude() > 0) {
      nearestLatitude = 90.f + (90.f - nearest.latitude());
    } else {
      nearestLatitude = -90.f - (nearest.latitude() + 90.f);
    }
  }
  // Check if on line segment
  if (nearestLatitude > start.latitude() && nearestLatitude < end.latitude()) {
    //return searchDistance(point, nearest);
    float startDist = searchDistance(point, start);
    float endDist = searchDistance(point, end);
    float onCircleDist = searchDistance(point, nearest);
    return std::min(startDist, std::min(endDist, onCircleDist));
  }

  float centerNearestLatitude = nearest.latitude() - centerLatitude;
  if (centerNearestLatitude > 180) {
    centerNearestLatitude -= 360;
  }
  if (centerNearestLatitude < -180) {
    centerNearestLatitude += 360;
  }
  if (centerNearestLatitude < 0) {
    // start is closest
    return searchDistance(point, start);
  }
  // end is closest
  return searchDistance(point, end);
  /*
  // bigger is closer
  if (nearest.latitude() < start.latitude())
  {
    // start point is shortest distance
    return searchDistance(point, start);
  }
  if (nearest.latitude() > end.latitude())
  {
    // End ist shortest
    return searchDistance(point, end);
  }
  // Nearest is on line segment
  return searchDistance(point, nearest);*/
}

inline float shortestDistanceToQuadrillateral(const LatLng *point, const LatLng &topLeft, const LatLng &bottomRight)
{

  // Check if between longitude lines on the same site as quadrilateral
  if (topLeft.longitude() >= point->longitude() && bottomRight.longitude() <= point->longitude())
  {
    // Check if inside Quadrilateral
    if (topLeft.latitude() >= point->latitude() && bottomRight.latitude() <= point->latitude())
    {
      return 0;
    }
    if (topLeft.latitude() < point->latitude())
    {
      // north of quadrilateral
      return searchDistance(point, LatLng(topLeft.latitude(), point->longitude()));
    }
    // south of quadrilateral
    return searchDistance(point, LatLng(bottomRight.latitude(), point->longitude()));
  }
  // Check if between longitude lines on other side of sphere
  float otherSideLongitude = point->longitude() - 180;
  if (otherSideLongitude < -180) {
    otherSideLongitude += 360;
  }
  if (otherSideLongitude > 180) {
    otherSideLongitude -= 360;
  }
  if (topLeft.longitude() >= otherSideLongitude && bottomRight.longitude() <= otherSideLongitude)
  {
    // Calculate distances to both latidue-circle intersections
    float d1 = searchDistance(point, LatLng(point->latitude(), topLeft.longitude()));
    float d2 = searchDistance(point, LatLng(point->latitude(), bottomRight.longitude()));
    // return shortest
    return d1 < d2 ? d1 : d2;
  }
  // Distance with shortest point is on longitude edges of quadrilateral

  float centerLongitude = point->longitude() - ((topLeft.longitude() + bottomRight.longitude()) / 2);
  if (centerLongitude < -180.0f)
  {
    centerLongitude += 360;
  }
  else if (centerLongitude > 180.0f)
  {
    centerLongitude -= 360;
  }
  if (centerLongitude > 0 && centerLongitude < 180)
  {
    return shortestDistanceToGreatCircleSegmet(LatLng(bottomRight.latitude(), topLeft.longitude()), topLeft, point);
  }
  else
  {
    return shortestDistanceToGreatCircleSegmet(bottomRight, LatLng(topLeft.latitude(), bottomRight.longitude()), point);
  }
}

inline float fastShortestDistanceToQuadrillateral(const SlEvent *point, const LatLng &topLeft, const LatLng &bottomRight, float *lngDistanceScale, std::function<Offsets(float lat, float lng)> &toOffsets)
{
  // Check if between longitude lines
  if (topLeft.longitude() > point->longitude() && bottomRight.longitude() < point->longitude())
  {
    // Check if inside Quadrilateral
    if (topLeft.latitude() > point->latitude() && bottomRight.latitude() < point->latitude())
    {
      return 0;
    }
    else if (topLeft.latitude() < point->latitude())
    {
      // north of quadrilateral
      return fastSearchDistance(point->getOffsets(), toOffsets(topLeft.latitude(), point->longitude()), lngDistanceScale);
    }
    // south of quadrilateral
    return fastSearchDistance(point->getOffsets(), toOffsets(bottomRight.latitude(), point->longitude()), lngDistanceScale);
  }
  
  // Distance with shortest point is on longitude edges of quadrilateral
  float centerLongitude = point->longitude() - ((topLeft.longitude() + bottomRight.longitude()) / 2);
  if (centerLongitude < -180.0f)
  {
    centerLongitude += 360;
  }
  else if (centerLongitude > 180.0f)
  {
    centerLongitude -= 360;
  }
  if (centerLongitude > 0 && centerLongitude < 180)
  {
    // check if below latitude
    if (topLeft.latitude() > point->latitude())
    {
      if (bottomRight.latitude() < point->latitude())
      {
        // Is on longitude line
        return fastSearchDistance(point->getOffsets(), toOffsets(point->latitude(), topLeft.longitude()), lngDistanceScale);
      }
      // bottomLeft edge is shortest
      return fastSearchDistance(point->getOffsets(), toOffsets(bottomRight.latitude(), topLeft.longitude()), lngDistanceScale);
    }
    // topLeft point is shortest
    return fastSearchDistance(point->getOffsets(), toOffsets(topLeft.latitude(), topLeft.longitude()), lngDistanceScale);
  }
  else
  {
    if (bottomRight.latitude() < point->latitude())
    {
      // Is on longitude line
      return fastSearchDistance(point->getOffsets(), toOffsets(point->latitude(), bottomRight.longitude()), lngDistanceScale);
    }
    // bottomRight edge is shortest
    return fastSearchDistance(point->getOffsets(), toOffsets(bottomRight.latitude(), bottomRight.longitude()), lngDistanceScale);
  }
  // topRight point is shortest
  return fastSearchDistance(point->getOffsets(), toOffsets(topLeft.latitude(), bottomRight.longitude()), lngDistanceScale);
}

#endif // _SPHERICAL_MATH_UTIL_H_
