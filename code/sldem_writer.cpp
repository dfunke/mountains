#include "sldem_writer.h"
#include "easylogging++.h"

#include <string>

using std::string;
void SldemWriter::writeTile(const std::string &directory, float minLat,
                          float minLng, Tile *t) {
  char buf[100];
  snprintf(buf, sizeof(buf), "SLDEM2015_512_%02d%c_%02d%c_%03d_%03d_FLOAT.IMG",
           abs(static_cast<int>(minLat)),
           (minLat >= 0) ? 'N' : 'S',
           abs(static_cast<int>(minLat+30)),
           (minLat+30 > 0) ? 'N' : 'S',
           abs(static_cast<int>(minLng+180)),
           abs(static_cast<int>(minLng+210))
           );
  string filename(buf);
  //string filename = "/home/pc/Data1/Moon/SLDEM2015_256_0N_60N_000_120_FLOAT.IMG";
  if (!directory.empty()) {
    filename = directory + "/" + filename;
  }

  FILE *outfile = fopen(filename.c_str(), "wb");
  if (outfile == nullptr) {
    VLOG(3) << "Failed to create file " << filename;
    return;
  }

  int num_samples = t->width() * t->height();

  float *outbuf = (float*)malloc(sizeof(float) * num_samples);

  for (int i = 0; i < num_samples; ++i) {
    int x = i % t->width();
    int y = i / t->width();
    // std::cout << x << " " << y << " " << y * t->width() + x  << std::endl;
    outbuf[i] = t->get(x,y);
  }

  std::size_t samples_write =
      fwrite(outbuf, sizeof(float), num_samples, outfile);
  if (samples_write != num_samples) {
    fprintf(stderr,
            "Couldn't write tile file: %s, got %ld samples expecting %d\n",
            filename.c_str(), samples_write, num_samples);
    free(outbuf);
  }
  free(outbuf);
  fclose(outfile);
}
