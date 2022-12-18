#include "isolation_sl_processor.h"
#include "../tile_cache.h"
#include "../isolation_results.h"
#include "isolation_finder_sl.h"
#include "../math_util.h"
#include "tile_cell.h"
#include "../ThreadPool.h"
#include "../lock.h"
#include "cell_memory_manager.h"
#include "concurrent_isolation_results.h"

#include <vector>
#include <unordered_set>
#include <algorithm>

using std::vector;

IsolationSlProcessor::IsolationSlProcessor(TileCache *cache)
{
    mCache = cache;
}

bool compare(IsolationResult const &el, IsolationResult const &er)
{
    // Sort by peak possition
    return (el.peak == er.peak) ? (el.isolationKm > er.isolationKm) : el.peak.latitude() != er.peak.latitude() ? el.peak.latitude() < er.peak.latitude()
                                                                                                       : el.peak.longitude() < er.peak.longitude();
}

template <typename T>
using maxheap = std::priority_queue<T, vector<T>, decltype(&compare)>;

IsolationResults IsolationSlProcessor::findIsolations(int numThreads, float bounds[], float mMinIsolationKm)
{
    FileFormat fileFormat(FileFormat::Value::HGT3);
    int latMax = (int)ceil(bounds[1]);
    int lngMax = (int)ceil(bounds[3]);
    int latMin = (int)floor(bounds[0]);
    int lngMin = (int)floor(bounds[2]);
    mTileTreeHead = new TileCell(latMin, lngMin, latMax - latMin, lngMax - lngMin);
    ThreadPool *threadPool = new ThreadPool(numThreads);
    vector<std::future<void>> voidFutures;
    // Create Buckets and build TileTree
    vector<IsolationFinderSl *> finders;
    //std::cout << "Start building tile-tree" << std::endl;
    vector<IsolationFinderSl *> *pFinders = &finders;
    // This step is verry fast if precalculated maxelev are available, so it can be done by main proccess.
    for (int j = lngMin; j < lngMax; ++j)
    {
        for (int i = latMin; i < latMax; ++i)
        {
            IsolationFinderSl *finder = new IsolationFinderSl(mCache, mTileTreeHead, i, j, fileFormat);
            //mTileTreeHead->insert(i, j, elev, new ConcurrentIsolationResults());
            pFinders->push_back(finder);
        }
    }

    // Fill buckets not parallel for now.
    // Later make addToBucket atomar.
    // voidFutures.clear();
    //std::cout << "Start setup proccess" << std::endl;
    /*
    int findersPerThread = finders.size() / numThreads;
    for (int i = 0; i < numThreads; i++)
    {
        voidFutures.push_back(threadPool->enqueue([=]
                                                  {
            int lastOffset = 0;
            if (i+1 == numThreads) {
                lastOffset = finders.size() % numThreads;
            }
            for (int j = i * findersPerThread; j < (i+1)* findersPerThread + lastOffset; ++j) {
                finders[j]->fillPeakBuckets(mMinIsolationKm);
            }
            return; }));
    }
    */
    for (auto &finder : finders) {
        voidFutures.push_back(threadPool->enqueue([=] { return finder->fillPeakBuckets(mMinIsolationKm); }));
    }
    /*
    for (auto finder : finders)
    {
        voidFutures.push_back(threadPool->enqueue([=]
                                                  { return finder->fillPeakBuckets(mMinIsolationKm); }));
    }*/
    for (auto &&waitFor : voidFutures)
    {
        waitFor.get();
    }
    int i = 0;
    vector<std::future<IsolationResults>> futureResults;
    vector<IsolationResults> results;
    //std::cout << "Start exact calculations" << std::endl;
    maxheap<IsolationResult> q(&compare);
    for (auto finder : finders)
    {

        futureResults.push_back(threadPool->enqueue([=]
                                                    { return finder->run(mMinIsolationKm); }));
    }
    for (auto &res : futureResults)
    {
        IsolationResults newResults = res.get();
        for (IsolationResult &oneResult : newResults.mResults)
        {
            q.emplace(oneResult);
        }
        newResults.mResults.clear();
    }
    //std::cout << "Start merging" << std::endl;
    //  Merge results
    IsolationResults finalResults;
    TileCell newRoot(latMin, lngMin, latMax - latMin, lngMax - lngMin);
    while (!q.empty())
    {
        if (q.top().isolationKm > mMinIsolationKm)
        {
            finalResults.mResults.push_back(q.top());
        }
        IsolationResult minResPos = q.top();
        q.pop();
        while (!q.empty() && q.top().peak == minResPos.peak)
        {
            q.pop();
        }
    }
    delete threadPool;
    delete mTileTreeHead;
    //std::cout << "Sort final results by isolation" << std::endl;
    std::sort(finalResults.mResults.begin(), finalResults.mResults.end(), [](IsolationResult const &lhs, IsolationResult const &rhs)
              { return lhs.isolationKm > rhs.isolationKm; });
    IsolationResults finalFinalResults;
    // merge multiple detected peaks (with nearly same isolation)
    if (finalResults.mResults.size() == 0)
    {
        return finalResults;
    }
    // Filter out dublicates
    //for (std::size_t i = 1; i < finalResults.mResults.size(); ++i)
    //{
    //    // Check if peaks are nearly identical
    //    if (r.peak.distance(finalResults.mResults[i].peak) < 900)
    //    {
    //        // assume identical peak, use one with smaller isolation
    //        if (finalResults.mResults[i].isolationKm < r.isolationKm)
    //        {
    //            r = finalResults.mResults[i];
    //        }
    //    }
    //    else
    //    {
    //        finalFinalResults.mResults.push_back(r);
    //        r = finalResults.mResults[i];
    //    }
    //    // Add last result
    //    if (i == finalResults.mResults.size() - 1)
    //    {
    //        finalFinalResults.mResults.push_back(r);
    //    }
    //}
    return finalResults;
}
