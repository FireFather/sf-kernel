#pragma once
#include <array>
#include "movegen.h"
#include "position.h"
#include "types.h"

namespace Nebula{
  template <typename T, int D>
  class StatsEntry{
    T entry;
  public:
    void operator=(const T& v){ entry=v; }
    T* operator&(){ return &entry; }
    T* operator->(){ return &entry; }
    operator const T&() const{ return entry; }

    void operator<<(const int bonus){
      const int delta=bonus-static_cast<int>(entry)*std::abs(bonus)/D;

      entry=static_cast<T>(
        static_cast<int>(entry)+delta
      );
    }
  };

  template <typename T, int D, int Size, int... Sizes>
  struct Statistics : std::array<Statistics<T, D, Sizes...>, Size>{
    using Stats = Statistics;

    void fill(const T& v){
      using Entry = StatsEntry<T, D>;
      Entry* p=reinterpret_cast<Entry*>(this);
      std::fill(p,p+sizeof(*this)/sizeof(Entry),v);
    }
  };

  template <typename T, int D, int Size>
  struct Statistics<T, D, Size> : std::array<StatsEntry<T, D>, Size>{};

  enum StatsParams :uint8_t{ NOT_USED=0 };

  enum StatsType :uint8_t{ NoCaptures, Captures };

  using ButterflyHistory = Statistics<int16_t, 7183, COLOR_NB, static_cast<int>(SQUARE_NB)*static_cast<int>(
    SQUARE_NB)>;
  using CounterMoveHistory = Statistics<Move, NOT_USED, PIECE_NB, SQUARE_NB>;
  using CapturePieceToHistory = Statistics<int16_t, 10692, PIECE_NB, SQUARE_NB, PIECE_TYPE_NB>;
  using PieceToHistory = Statistics<int16_t, 29952, PIECE_NB, SQUARE_NB>;
  using ContinuationHistory = Statistics<PieceToHistory, NOT_USED, PIECE_NB, SQUARE_NB>;

  class MovePicker{
    enum PickType :uint8_t{ Next, Best };
  public:
    MovePicker(const MovePicker&) = delete;
    MovePicker& operator=(const MovePicker&) = delete;
    MovePicker(const Position&, Move, Depth, const ButterflyHistory*,
      const CapturePieceToHistory*,
      const PieceToHistory**,
      Move,
      const Move*);
    MovePicker(const Position&, Move, Depth, const ButterflyHistory*,
      const CapturePieceToHistory*,
      const PieceToHistory**,
      Square);
    MovePicker(const Position&, Move, Value, Depth, const CapturePieceToHistory*);
    Move nextMove(bool skipQuiets=false);
  private:
    template <PickType T, typename Pred>
    Move select(Pred);
    template <GenType>
    void score() const;
    [[nodiscard]] ExtMove* begin() const{ return cur; }
    [[nodiscard]] ExtMove* end() const{ return endMoves; }
    const Position& pos;
    const ButterflyHistory* mainHistory;
    const CapturePieceToHistory* captureHistory;
    const PieceToHistory** continuationHistory;
    Move ttMove;
    ExtMove refutations[3], *cur, *endMoves, *endBadCaptures;
    int stage;
    Square recaptureSquare;
    Value threshold;
    Depth depth;
    ExtMove moves[maxMoves];
  };
}
