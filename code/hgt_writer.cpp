#include "hgt_writer.h"
#include "easylogging++.h"

#include <string>

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
    int16 elev = static_cast<int16>(t->get(i / t->width(), i % t->width()));
    //int16 elevation = swapByteOrder16(inbuf[i]);
    if (elevation == HGT_NODATA_ELEVATION) {
      samples[i] = Tile::NODATA_ELEVATION;
    } else {
      samples[i] = static_cast<Elevation>(elevation);
    }
  }

  Tile *retval = nullptr;

  std::size_t samples_write =
      fwrite(outbuf, sizeof(int16), num_samples, outfile);
  if (samples_read != num_samples) {
    fprintf(stderr,
            "Couldn't read tile file: %s, got %d samples expecting %d\n",
            filename.c_str(), samples_read, num_samples);
    free(inbuf);
  } else {
    Elevation *samples = (Elevation *)malloc(sizeof(Elevation) * num_samples);
    // SRTM data is in big-endian order; convert to Elevation
    retval = new Tile(mFormat.rawSamplesAcross(), mFormat.rawSamplesAcross(),
                      samples, mFormat);
  }

  free(inbuf);
  fclose(infile);

  return retval;
}
