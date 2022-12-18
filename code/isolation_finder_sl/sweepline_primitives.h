#ifndef _SWEEPLINE_PRIMITIVES_H_
#define _SWEEPLINE_PRIMITIVES_H_

#include "../primitives.h"
#include "../tile.h"
#include "../latlng.h"

class SlEvent : public LatLng {
public:
  SlEvent(Elevation elev, bool isPeak, LatLng latLng);
  SlEvent(int lat, int lng) : LatLng(lat, lng) {}
  SlEvent() : LatLng(){}
  void initialize(Elevation elev, bool isPeak, LatLng latLng, Offsets offsets);

  inline Elevation getElev() const {
    return mElev;
  }

  inline bool isPeak() const {
    return mIsPeak;
  }

  inline Offsets getOffsets() const {
    return mOffsets;
  }

  bool equal(const SlEvent &e) const;

private:
  Elevation mElev;
  Offsets mOffsets;
  bool mIsPeak;
};

# endif // _SWEEPLINE_PRIMITIVES_H_
