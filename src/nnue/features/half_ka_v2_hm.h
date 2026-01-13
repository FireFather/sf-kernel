#pragma once
#include "../nnue_common.h"
#include "../../evaluate.h"
#include "../../misc.h"

namespace Nebula{
  struct StateInfo;
}

namespace Nebula::Eval::Nnue::Features{
  class HalfKAv2Hm{
    enum :uint16_t{
      PS_NONE=0,
      PS_W_PAWN=0,
      PS_B_PAWN=1*SQUARE_NB,
      PS_W_KNIGHT=2*SQUARE_NB,
      PS_B_KNIGHT=3*SQUARE_NB,
      PS_W_BISHOP=4*SQUARE_NB,
      PS_B_BISHOP=5*SQUARE_NB,
      PS_W_ROOK=6*SQUARE_NB,
      PS_B_ROOK=7*SQUARE_NB,
      PS_W_QUEEN=8*SQUARE_NB,
      PS_B_QUEEN=9*SQUARE_NB,
      PS_KING=10*SQUARE_NB,
      PS_NB=11*SQUARE_NB
    };

    static constexpr IndexType PieceSquareIndex[COLOR_NB][PIECE_NB]={
      {
        PS_NONE,PS_W_PAWN,PS_W_KNIGHT,PS_W_BISHOP,PS_W_ROOK,PS_W_QUEEN,PS_KING,PS_NONE,
        PS_NONE,PS_B_PAWN,PS_B_KNIGHT,PS_B_BISHOP,PS_B_ROOK,PS_B_QUEEN,PS_KING,PS_NONE
      },
      {
        PS_NONE,PS_B_PAWN,PS_B_KNIGHT,PS_B_BISHOP,PS_B_ROOK,PS_B_QUEEN,PS_KING,PS_NONE,
        PS_NONE,PS_W_PAWN,PS_W_KNIGHT,PS_W_BISHOP,PS_W_ROOK,PS_W_QUEEN,PS_KING,PS_NONE
      }
    };
    static Square orient(Color perspective, Square s, Square ksq);
    static IndexType makeIndex(Color perspective, Square s, Piece pc, Square ksq);
  public:
    static constexpr std::uint32_t HashValue=0x7f234cb8u;
    static constexpr IndexType Dimensions=
      static_cast<IndexType>(SQUARE_NB)*static_cast<IndexType>(PS_NB)/2;
    static constexpr int KingBuckets[64]={
      -1,-1,-1,-1,31,30,29,28,
      -1,-1,-1,-1,27,26,25,24,
      -1,-1,-1,-1,23,22,21,20,
      -1,-1,-1,-1,19,18,17,16,
      -1,-1,-1,-1,15,14,13,12,
      -1,-1,-1,-1,11,10,9,8,
      -1,-1,-1,-1,7,6,5,4,
      -1,-1,-1,-1,3,2,1,0
    };
    static constexpr IndexType MaxActiveDimensions=32;
    using IndexList = ValueList<IndexType, MaxActiveDimensions>;
    static void appendActiveIndices(
      const Position& pos,
      Color perspective,
      IndexList& active);
    static void appendChangedIndices(
      Square ksq,
      const DirtyPiece& dp,
      Color perspective,
      IndexList& removed,
      IndexList& added
    );
    static int updateCost(const StateInfo* st);
    static int refreshCost(const Position& pos);
    static bool requiresRefresh(const StateInfo* st, Color perspective);
  };
}
