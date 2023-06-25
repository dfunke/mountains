#include "sldem_loader.h"
#include "tile.h"
#include "util.h"
#include "easylogging++.h"
#include "file_format.h"

#include <stdio.h>
#include <string>

using std::string;


#include "tile.h"
#include "util.h"
#include "easylogging++.h"
#include "file_format.h"

#include <stdio.h>
#include <string>

using std::string;

Tile *SldemLoader::loadTile(const std::string &directory, float minLat, float minLng) {
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

  FILE *infile = fopen(filename.c_str(), "rb");
  if (infile == nullptr) {
    VLOG(3) << "Failed to open file " << filename;
    return nullptr;
  }
  
  int num_samples = mFormat.inMemorySamplesAcross()* mFormat.inMemorySamplesAcross();
  
  float *inbuf = (float*) malloc(sizeof(float) * num_samples);
  
  Tile *retval = nullptr;
  
  int samples_read = static_cast<int>(fread(inbuf, sizeof(float), num_samples, infile));
  if (samples_read != num_samples) {
    fprintf(stderr, "Couldn't read tile file: %s, got %d samples expecting %d\n",
            filename.c_str(), samples_read, num_samples);
    free(inbuf);
  } else {
    // SRTM data is in big-endian order; convert to Elevation
    retval = new Tile(mFormat.inMemorySamplesAcross(), mFormat.inMemorySamplesAcross(), inbuf, FileFormat::Value::SLDEM);
    //retval->saveAsImage("/home/pc/tmp", minLat, minLng);
  }
  fclose(infile);
  return retval;
}
