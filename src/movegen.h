#pragma once
#include <algorithm>
#include "types.h"
#include "position.h"

namespace Nebula{
  class Position;

  enum GenType :uint8_t{
    CAPTURES,
    QUIETS,
    QUIET_CHECKS,
    EVASIONS,
    NON_EVASIONS,
    LEGAL
  };

  struct ExtMove{
    Move move;
    int value;
    operator Move() const{ return move; }
    void operator=(const Move m){ move=m; }
    operator float() const = delete;
  };

  inline bool operator<(const ExtMove& f, const ExtMove& s){ return f.value<s.value; }
  template <GenType>
  ExtMove* generate(const Position& pos, ExtMove* moveList);

  template <GenType T>
  struct MoveList{
    explicit MoveList(const Position& pos) : moveList{}, last(generate<T>(pos,moveList)){}
    [[nodiscard]] const ExtMove* begin() const{ return moveList; }
    [[nodiscard]] const ExtMove* end() const{ return last; }
    [[nodiscard]] size_t size() const{ return last-moveList; }
    [[nodiscard]] bool contains(const Move move) const{ return std::find(begin(),end(),move)!=end(); }
  private:
    ExtMove moveList[maxMoves], *last;
  };
}

namespace Nebula{
  namespace Movegen{
    template <GenType Type, Direction D>
    ExtMove* makePromotions(ExtMove* moveList, const Square to){
      if (Type==CAPTURES||Type==EVASIONS||Type==NON_EVASIONS)
        *moveList++=make<PROMOTION>(to-D,to,QUEEN);
      if (Type==QUIETS||Type==EVASIONS||Type==NON_EVASIONS){
        *moveList++=make<PROMOTION>(to-D,to,ROOK);
        *moveList++=make<PROMOTION>(to-D,to,BISHOP);
        *moveList++=make<PROMOTION>(to-D,to,KNIGHT);
      }
      return moveList;
    }

    template <Color Us, GenType Type>
    ExtMove* generatePawnMoves(const Position& pos, ExtMove* moveList, const uint64_t target){
      constexpr Color them=~Us;
      constexpr uint64_t tRank7Bb=Us==WHITE?rank7Bb:rank2Bb;
      constexpr uint64_t tRank3Bb=Us==WHITE?rank3Bb:rank6Bb;
      constexpr Direction up=pawnPush(Us);
      constexpr Direction upRight=Us==WHITE?NORTH_EAST:SOUTH_WEST;
      constexpr Direction upLeft=Us==WHITE?NORTH_WEST:SOUTH_EAST;
      const uint64_t emptySquares=~pos.pieces();
      const uint64_t enemies=Type==EVASIONS
                             ?pos.checkers()
                             :pos.pieces(them);
      const uint64_t pawnsOn7=pos.pieces(Us,PAWN)&tRank7Bb;
      const uint64_t pawnsNotOn7=pos.pieces(Us,PAWN)&~tRank7Bb;
      if (Type!=CAPTURES){
        uint64_t b1=shift<up>(pawnsNotOn7)&emptySquares;
        uint64_t b2=shift<up>(b1&tRank3Bb)&emptySquares;
        if (Type==EVASIONS){
          b1&=target;
          b2&=target;
        }
        if (Type==QUIET_CHECKS){
          const Square ksq=pos.square<KING>(them);
          const uint64_t dcCandidatePawns=pos.blockersForKing(them)&~fileBb(ksq);
          b1&=pawnAttacksBb(them,ksq)|shift<up>(dcCandidatePawns);
          b2&=pawnAttacksBb(them,ksq)|shift<up+up>(dcCandidatePawns);
        }
        while (b1){
          const Square to=popLsb(b1);
          *moveList++=makeMove(to-up,to);
        }
        while (b2){
          const Square to=popLsb(b2);
          *moveList++=makeMove(to-up-up,to);
        }
      }
      if (pawnsOn7){
        uint64_t b1=shift<upRight>(pawnsOn7)&enemies;
        uint64_t b2=shift<upLeft>(pawnsOn7)&enemies;
        uint64_t b3=shift<up>(pawnsOn7)&emptySquares;
        if (Type==EVASIONS)
          b3&=target;
        while (b1)
          moveList=makePromotions<Type, upRight>(moveList,popLsb(b1));
        while (b2)
          moveList=makePromotions<Type, upLeft>(moveList,popLsb(b2));
        while (b3)
          moveList=makePromotions<Type, up>(moveList,popLsb(b3));
      }
      if (Type==CAPTURES||Type==EVASIONS||Type==NON_EVASIONS){
        uint64_t b1=shift<upRight>(pawnsNotOn7)&enemies;
        uint64_t b2=shift<upLeft>(pawnsNotOn7)&enemies;
        while (b1){
          const Square to=popLsb(b1);
          *moveList++=makeMove(to-upRight,to);
        }
        while (b2){
          const Square to=popLsb(b2);
          *moveList++=makeMove(to-upLeft,to);
        }
        if (pos.epSquare()!=SQ_NONE){
          if (Type==EVASIONS&&target&pos.epSquare()+up)
            return moveList;
          b1=pawnsNotOn7&pawnAttacksBb(them,pos.epSquare());
          while (b1)
            *moveList++=make<EN_PASSANT>(popLsb(b1),pos.epSquare());
        }
      }
      return moveList;
    }

    template <Color Us, PieceType Pt, bool Checks>
    ExtMove* generateMoves(const Position& pos, ExtMove* moveList, uint64_t target){
      uint64_t bb=pos.pieces(Us,Pt);
      while (bb){
        Square from=popLsb(bb);
        uint64_t b=attacksBb<Pt>(from,pos.pieces())&target;
        if (Checks&&(Pt==QUEEN||!(pos.blockersForKing(~Us)&from)))
          b&=pos.checkSquares(Pt);
        while (b)
          *moveList++=makeMove(from,popLsb(b));
      }
      return moveList;
    }

    template <Color Us, GenType Type>
    ExtMove* generateAll(const Position& pos, ExtMove* moveList){
      constexpr bool checks=Type==QUIET_CHECKS;
      const Square ksq=pos.square<KING>(Us);
      uint64_t target;
      if (Type!=EVASIONS||!moreThanOne(pos.checkers())){
        target=Type==EVASIONS
               ?getBetweenBb(ksq,lsb(pos.checkers()))
               :Type==NON_EVASIONS
               ?~pos.pieces(Us)
               :Type==CAPTURES
               ?pos.pieces(~Us)
               :~pos.pieces();
        moveList=generatePawnMoves<Us, Type>(pos,moveList,target);
        moveList=generateMoves<Us, KNIGHT, checks>(pos,moveList,target);
        moveList=generateMoves<Us, BISHOP, checks>(pos,moveList,target);
        moveList=generateMoves<Us, ROOK, checks>(pos,moveList,target);
        moveList=generateMoves<Us, QUEEN, checks>(pos,moveList,target);
      }
      if (!checks||pos.blockersForKing(~Us)&ksq){
        uint64_t b=attacksBb<KING>(ksq)&(Type==EVASIONS?~pos.pieces(Us):target);
        if (checks)
          b&=~attacksBb<QUEEN>(pos.square<KING>(~Us));
        while (b)
          *moveList++=makeMove(ksq,popLsb(b));
        if ((Type==QUIETS||Type==NON_EVASIONS)&&pos.canCastle(Us&ANY_CASTLING))
          for (const CastlingRights cr : {Us&KING_SIDE,Us&QUEEN_SIDE})
            if (!pos.castlingImpeded(cr)&&pos.canCastle(cr))
              *moveList++=make<CASTLING>(ksq,pos.castleRookSquare(cr));
      }
      return moveList;
    }
  }

  template <GenType Type>
  ExtMove* generate(const Position& pos, ExtMove* moveList){
    const Color us=pos.stm();
    return us==WHITE
           ?Movegen::generateAll<WHITE, Type>(pos,moveList)
           :Movegen::generateAll<BLACK, Type>(pos,moveList);
  }

  template <>
  inline ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList){
    const Color us=pos.stm();
    const uint64_t pinned=pos.blockersForKing(us)&pos.pieces(us);
    const Square ksq=pos.square<KING>(us);
    ExtMove* cur=moveList;
    moveList=pos.checkers()
             ?generate<EVASIONS>(pos,moveList)
             :generate<NON_EVASIONS>(pos,moveList);
    while (cur!=moveList)
      if (((pinned&&pinned&fromSq(*cur))||fromSq(*cur)==ksq||typeOf(*cur)==EN_PASSANT)
        &&!pos.legal(*cur))
        *cur=(--moveList)->move;
      else
        ++cur;
    return moveList;
  }
}
