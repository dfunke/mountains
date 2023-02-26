#include "easylogging++.h"
#include "tile.h"
#include "hgt_loader.h"
#include "hgt_writer.h"
#include "file_format.h"
#include "tile_loading_policy.h"
#include "tile_cache.h"


#ifdef PLATFORM_LINUX
#include <unistd.h>
#endif
#ifdef PLATFORM_WINDOWS
#include "getopt-win.h"
#endif

#include "assert.h"
#include <cmath>

INITIALIZE_EASYLOGGINGPP

using std::floor;
using std::ceil;
Tile *incRes(Tile* base, FileFormat oldFormat, FileFormat newFormat) {
    std::size_t num_samples = newFormat.rawSamplesAcross() * newFormat.rawSamplesAcross();
    float ratio = newFormat.rawSamplesAcross() / (1.f* oldFormat.rawSamplesAcross());
    assert(ratio > 1);
    Elevation *samples = (Elevation *) malloc(sizeof(Elevation) * num_samples);
    for (int j = 0; j < newFormat.rawSamplesAcross(); j++) {
        for (int i = 0; i < newFormat.rawSamplesAcross(); i++) {
            int idx = j * newFormat.rawSamplesAcross() + i;

            float x = i / ratio;
            float y = j / ratio;
            int x1 = floor(x);
            int x2 = ceil(x);
            int y1 = floor(y);
            int y2 = ceil(y);
            Elevation e1 = (x - x1) * base->get(x1, y1) + (1-x - x1) * base->get(x2, y1);
            Elevation e2 = (x - x1) * base->get(x1, y2) + (1-x - x1) * base->get(x2, y2);
            samples[idx] = (y - y1) * e1 + (1-y -y1) * e2;
        }
    }
    Tile *erg = new Tile(newFormat.rawSamplesAcross(), newFormat.rawSamplesAcross(), samples, newFormat);
    return erg;
}

int main(int argc, char **argv)
{
    START_EASYLOGGINGPP(argc, argv);

    std::string hgt3Folder = "/home/pc/Data2/SRTM-DEM3";
    std::string saveFolder = "/home/pc/tmp";
    FileFormat fileFormat(FileFormat::Value::HGT3);
    BasicTileLoadingPolicy policy(hgt3Folder.c_str(),fileFormat);
    TileCache *cache = new TileCache(&policy, 0);
    CoordinateSystem *coordinateSystem = fileFormat.coordinateSystemForOrigin(46.f, 9.f);
    Tile *t = cache->loadWithoutCaching(46, 9, *coordinateSystem);
    FileFormat higherFormat(FileFormat::Value::HGT1);
    Tile *higherRes = incRes(t, fileFormat, higherFormat);

    HgtWriter writer(higherFormat);
    writer.writeTile(saveFolder.c_str(), 46, 9, higherRes);
    
    // Compare tiles
    /*
    BasicTileLoadingPolicy policy2(saveFolder.c_str(),fileFormat);
    TileCache *cache2 = new TileCache(&policy2, 0);
    Tile *t2 = cache2->loadWithoutCaching(46, 9, *coordinateSystem);
    for (int i = 0; i < t->width(); i++) {
        for (int j = 0; j < t->height(); j++) {
            Offsets o(i,j);
            if (t->get(o) != t2->get(o)) {
                std::cout << "Diff at: " << i <<", "  << j << " t1: " << t->get(o) << " t2: " << t2->get(o) << std::endl;
                
            }
        }
    }*/
    return 0;
}
