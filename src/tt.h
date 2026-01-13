#pragma once
#include "misc.h"
#include "types.h"

namespace Nebula{
  struct TtEntry{
    [[nodiscard]] Move move() const{ return static_cast<Move>(move16); }
    [[nodiscard]] Value value() const{ return static_cast<Value>(value16); }
    [[nodiscard]] Value eval() const{ return static_cast<Value>(eval16); }
    [[nodiscard]] Depth depth() const{ return static_cast<Depth>(depth8)+DEPTH_OFFSET; }
    [[nodiscard]] bool isPv() const{ return static_cast<bool>(genBound8&0x4); }
    [[nodiscard]] Bound bound() const{ return static_cast<Bound>(genBound8&0x3); }
    void save(uint64_t k, Value v, bool pv, Bound b, Depth d, Move m, Value ev);
  private:
    friend class TranspositionTable;
    uint16_t key16;
    uint8_t depth8;
    uint8_t genBound8;
    uint16_t move16;
    int16_t value16;
    int16_t eval16;
  };

  class TranspositionTable{
    static constexpr int ClusterSize=3;

    struct Cluster{
      TtEntry entry[ClusterSize];
      char padding[2];
    };

    static constexpr unsigned GENERATION_BITS=3;
    static constexpr int GENERATION_DELTA=1<<GENERATION_BITS;
    static constexpr int GENERATION_CYCLE=255+(1<<GENERATION_BITS);
    static constexpr int GENERATION_MASK=0xFF<<GENERATION_BITS&0xFF;
  public:
    ~TranspositionTable(){ alignedLargePagesFree(table); }
    void newSearch(){ generation8+=GENERATION_DELTA; }
    TtEntry* probe(uint64_t key, bool& found) const;
    void resize(size_t mbSize);
    void clear() const;
    [[nodiscard]] TtEntry* firstEntry(const uint64_t key) const{ return &table[mulHi64(key,clusterCount)].entry[0]; }
  private:
    friend struct TtEntry;
    size_t clusterCount;
    Cluster* table;
    uint8_t generation8;
  };

  extern TranspositionTable tt;
}
