#include "sweepline_primitives.h"
#include "../primitives.h"
#include "../latlng.h"


SlEvent::SlEvent(Elevation elev, EventType type, LatLng latLng) : LatLng(latLng) {
  mElev = elev;
  mType = type;
  mOffsets = Offsets(0,0);
}

void SlEvent::initialize(Elevation elev, EventType type, LatLng latLng, Offsets offsets) {
  mElev = elev;
  mType = type;
  mOffsets = offsets;
  updateLatLng(latLng);
}

bool SlEvent::equal(const SlEvent &e) const {
  return this->latitude() == e.latitude() && this->longitude() == e.longitude() && this->mElev == e.mElev;
}