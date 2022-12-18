#include "sweepline_primitives.h"
#include "../primitives.h"
#include "../latlng.h"


SlEvent::SlEvent(Elevation elev, bool isPeak, LatLng latLng) : LatLng(latLng) {
  mElev = elev;
  mIsPeak = isPeak;
  mOffsets = Offsets(0,0);
}

void SlEvent::initialize(Elevation elev, bool isPeak, LatLng latLng, Offsets offsets) {
  mElev = elev;
  mIsPeak = isPeak;
  mOffsets = offsets;
  updateLatLng(latLng);
}

bool SlEvent::equal(const SlEvent &e) const {
  return this->latitude() == e.latitude() && this->longitude() == e.longitude() && this->mElev == e.mElev;
}