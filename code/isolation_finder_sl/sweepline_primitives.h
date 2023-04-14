#ifndef _SWEEPLINE_PRIMITIVES_H_
#define _SWEEPLINE_PRIMITIVES_H_

#include "../primitives.h"
#include "../tile.h"
#include "../latlng.h"

enum SlEventType {
  ADD,
  REMOVE,
  PEAK
};

class SlEvent : public LatLng {
public:
  SlEvent(Elevation elev, SlEventType type, LatLng latLng);
  SlEvent(int lat, int lng) : LatLng(lat, lng) {}
  SlEvent() : LatLng(){}
  void initialize(Elevation elev, SlEventType type, LatLng latLng, Offsets offsets);

  inline Elevation getElev() const {
    return mElev;
  }

  inline SlEventType getType() const {
    return mType;
  }

  inline Offsets getOffsets() const {
    return mOffsets;
  }

  bool equal(const SlEvent &e) const;
  bool operator>(const SlEvent &other) {
    return mElev > other.mElev;
  }
  bool operator<(const SlEvent &other) {
    return mElev < other.mElev;
  }
  bool operator>=(const SlEvent &other) {
    return mElev >= other.mElev;
  }
  bool operator<=(const SlEvent &other) {
    return mElev <= other.mElev;
  }

private:
  Elevation mElev;
  Offsets mOffsets;
  SlEventType mType;
};

# endif // _SWEEPLINE_PRIMITIVES_H_
