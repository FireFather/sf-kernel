#include <cstring>
#include <thread>
#include <vector>
#include "misc.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"

namespace Nebula{
  TranspositionTable tt;

  void TtEntry::save(const uint64_t k, const Value v, const bool pv, const Bound b,
    const Depth d, const Move m, const Value ev){
    if (m||static_cast<uint16_t>(k)!=key16)
      move16=static_cast<uint16_t>(m);

    if (b==BOUND_EXACT
      ||static_cast<uint16_t>(k)!=key16
      ||d-DEPTH_OFFSET+2*pv>depth8-4){
      key16=static_cast<uint16_t>(k);
      depth8=static_cast<uint8_t>(d-DEPTH_OFFSET);
      genBound8=static_cast<uint8_t>(tt.generation8|static_cast<uint8_t>(pv)<<2|b);
      value16=static_cast<int16_t>(v);
      eval16=static_cast<int16_t>(ev);
    }
  }

  void TranspositionTable::resize(const size_t mbSize){
    threads.main()->waitForSearchFinished();

    stdAlignedFree(table);

    clusterCount=mbSize*1024*1024/sizeof(Cluster);
    table=static_cast<Cluster*>(
      stdAlignedAlloc(alignof(Cluster),clusterCount*sizeof(Cluster)));

    clear();
  }

  void TranspositionTable::clear() const{
    const size_t threadsCount=std::max<size_t>(1,options["Threads"].asSize());

    std::vector<std::thread> thrds;
    thrds.reserve(threadsCount);

    const size_t stride=clusterCount/threadsCount;

    for (size_t idx=0; idx<threadsCount; ++idx){
      const size_t start=stride*idx;
      const size_t len=(idx+1<threadsCount)
                       ?stride
                       :clusterCount-start;

      thrds.emplace_back([this, start, len]{ std::memset(&table[start],0,len*sizeof(Cluster)); });
    }

    for (std::thread& th : thrds)
      th.join();
  }

  TtEntry* TranspositionTable::probe(const uint64_t key, bool& found) const{
    TtEntry* const tte=firstEntry(key);
    const auto key16=static_cast<uint16_t>(key);

    for (int i=0; i<ClusterSize; ++i)
      if (tte[i].key16==key16||!tte[i].depth8){
        tte[i].genBound8=static_cast<uint8_t>(
          generation8|(tte[i].genBound8&(GENERATION_DELTA-1)));
        found=static_cast<bool>(tte[i].depth8);
        return &tte[i];
      }

    TtEntry* replace=tte;
    for (int i=1; i<ClusterSize; ++i)
      if (replace->depth8-((GENERATION_CYCLE+generation8-replace->genBound8)&GENERATION_MASK)
        >tte[i].depth8-((GENERATION_CYCLE+generation8-tte[i].genBound8)&GENERATION_MASK))
        replace=&tte[i];

    found=false;
    return replace;
  }
}
