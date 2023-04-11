#ifndef _SWEEPLINE_PRIMITIVES_H_
#define _SWEEPLINE_PRIMITIVES_H_

#include "../primitives.h"
#include "../tile.h"
#include "../latlng.h"

  enum EventType {
    ADD,
    PEAK,
    REMOVE
  };

class SlEvent : public LatLng {
public:
  SlEvent(Elevation elev, EventType type, LatLng latLng);
  SlEvent(int lat, int lng) : LatLng(lat, lng) {}
  SlEvent() : LatLng(){}
  void initialize(Elevation elev, EventType type, LatLng latLng, Offsets offsets);

  inline Elevation getElev() const {
    return mElev;
  }

  inline EventType getType() const {
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
  EventType mType;
};

# endif // _SWEEPLINE_PRIMITIVES_H_
