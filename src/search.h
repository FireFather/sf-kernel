#pragma once
#include <vector>
#include "misc.h"
#include "movepick.h"
#include "types.h"

namespace Nebula{
  class Position;

  namespace Search{
    constexpr int counterMovePruneThreshold=0;

    struct Stack{
      Move* pv;
      PieceToHistory* continuationHistory;
      int ply;
      Move currentMove;
      Move excludedMove;
      Move killers[2];
      Value staticEval;
      int statScore;
      int moveCount;
      bool inCheck;
      bool ttPv;
      bool ttHit;
      int doubleExtensions;
      int cutoffCnt;
    };

    struct RootMove{
      explicit RootMove(const Move m) : tbScore(), pv(1,m){}
      bool extractPonderFromTt(Position& pos);
      bool operator==(const Move& m) const{ return pv[0]==m; }

      bool operator<(const RootMove& m) const{
        return m.score!=score
               ?m.score<score
               :m.previousScore<previousScore;
      }

      Value score=-VALUE_INFINITE;
      Value previousScore=-VALUE_INFINITE;
      Value averageScore=-VALUE_INFINITE;
      int tbRank=0;
      Value tbScore;
      std::vector<Move> pv;
    };

    using RootMoves = std::vector<RootMove>;

    struct LimitsType{
      LimitsType() : startTime(0){
        time[WHITE]=time[BLACK]=inc[WHITE]=inc[BLACK]=npmsec=movetime=static_cast<TimePoint>(0);
        movestogo=depth=mate=infinite=0;
        nodes=0;
      }

      [[nodiscard]] bool useTimeManagement() const{ return time[WHITE]||time[BLACK]; }
      std::vector<Move> searchmoves;
      TimePoint time[COLOR_NB], inc[COLOR_NB], npmsec, movetime, startTime;
      int movestogo, depth, mate, infinite;
      int64_t nodes;
    };

    extern LimitsType limits;
    void init();
    void clear();
  }
}
