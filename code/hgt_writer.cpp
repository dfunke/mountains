#include "hgt_writer.h"
#include "easylogging++.h"

#include <string>

static const int16 HGT_NODATA_ELEVATION = -32768;

static uint16 swapByteOrder16(uint16 us) {
  return (us >> 8) | (us << 8);
}

using std::string;
void HgtWriter::writeTile(const std::string &directory, float minLat,
                          float minLng, Tile *t) {
  char buf[100];
  snprintf(buf, sizeof(buf), "%c%02d%c%03d.hgt", (minLat >= 0) ? 'N' : 'S',
           abs(static_cast<int>(minLat)), (minLng >= 0) ? 'E' : 'W',
           abs(static_cast<int>(minLng)));
  string filename(buf);
  filename = directory + "/" + filename;

  FILE *outfile = fopen(filename.c_str(), "wb");
  if (outfile == nullptr) {
    VLOG(3) << "Failed to create file " << filename;
    return;
  }

  int num_samples = mFormat.rawSamplesAcross() * mFormat.rawSamplesAcross();

  int16 *outbuf = (int16 *)malloc(sizeof(int16) * num_samples);

  for (int i = 0; i < num_samples; ++i) {
    int x = i % t->width();
    int y = i / t->width();
    // std::cout << x << " " << y << " " << y * t->width() + x  << std::endl;
    Elevation elev = t->get(x, y);
    if (elev == Tile::NODATA_ELEVATION) {
      outbuf[i] = swapByteOrder16(HGT_NODATA_ELEVATION);
    } else {
      outbuf[i] = swapByteOrder16(elev);
    }
  }

  std::size_t samples_write =
      fwrite(outbuf, sizeof(int16), num_samples, outfile);
  if (samples_write != num_samples) {
    fprintf(stderr,
            "Couldn't write tile file: %s, got %ld samples expecting %d\n",
            filename.c_str(), samples_write, num_samples);
    free(outbuf);
  }
  free(outbuf);
  fclose(outfile);
}
