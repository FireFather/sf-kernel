#include "bitboard.h"
#include "movepick.h"

namespace Nebula{
  namespace{
    enum Stages :uint8_t{
      MAIN_TT, CAPTURE_INIT, GOOD_CAPTURE, REFUTATION, QUIET_INIT, QUIET, BAD_CAPTURE,
      EVASION_TT, EVASION_INIT, EVASION,
      PROBCUT_TT, PROBCUT_INIT, PROBCUT,
      QSEARCH_TT, QCAPTURE_INIT, QCAPTURE, QCHECK_INIT, QCHECK
    };

    void partialInsertionSort(ExtMove* begin, const ExtMove* end, const int limit){
      for (ExtMove *sortedEnd=begin, *p=begin+1; p<end; ++p)
        if (p->value>=limit){
          ExtMove tmp=*p, *q;
          *p=*++sortedEnd;
          for (q=sortedEnd; q!=begin&&*(q-1)<tmp; --q)
            *q=*(q-1);
          *q=tmp;
        }
    }
  }

  MovePicker::MovePicker(const Position& p, const Move ttm, const Depth d, const ButterflyHistory* mh,
    const CapturePieceToHistory* cph,
    const PieceToHistory** ch,
    const Move cm,
    const Move* killers)
    : pos(p), mainHistory(mh), captureHistory(cph), continuationHistory(ch),
    ttMove(ttm), refutations{{.move=killers[0],.value=0},{.move=killers[1],.value=0},{.move=cm,.value=0}}, cur(nullptr),
    endMoves(nullptr),
    endBadCaptures(nullptr),
    recaptureSquare(),
    threshold(), depth(d),
    moves{}{
    stage=(pos.checkers()?EVASION_TT:MAIN_TT)+
      !(ttm&&pos.pseudoLegal(ttm));
  }

  MovePicker::MovePicker(const Position& p, const Move ttm, const Depth d, const ButterflyHistory* mh,
    const CapturePieceToHistory* cph,
    const PieceToHistory** ch,
    const Square rs)
    : pos(p), mainHistory(mh), captureHistory(cph), continuationHistory(ch), ttMove(ttm), refutations{}, cur(nullptr),
    endMoves(nullptr),
    endBadCaptures(nullptr),
    recaptureSquare(rs),
    threshold(), depth(d),
    moves{}{
    stage=(pos.checkers()?EVASION_TT:QSEARCH_TT)+
      !(ttm
        &&(pos.checkers()||depth>DEPTH_QS_RECAPTURES||toSq(ttm)==recaptureSquare)
        &&pos.pseudoLegal(ttm));
  }

  MovePicker::MovePicker(const Position& p, const Move ttm, const Value th, const Depth d,
    const CapturePieceToHistory* cph)
    : pos(p), mainHistory(nullptr), captureHistory(cph), continuationHistory(nullptr), ttMove(ttm), refutations{},
    cur(nullptr),
    endMoves(nullptr), endBadCaptures(nullptr),
    recaptureSquare(),
    threshold(th), depth(d),
    moves{}{
    stage=PROBCUT_TT+!(ttm&&pos.capture(ttm)
      &&pos.pseudoLegal(ttm)
      &&pos.seeGe(ttm,threshold));
  }

  template <GenType Type>
  void MovePicker::score() const{
    uint64_t threatened=0, threatenedByPawn=0, threatenedByMinor=0, threatenedByRook=0;
    if constexpr (Type==QUIETS){
      const Color us=pos.stm();
      threatenedByPawn=pos.attacksBy<PAWN>(~us);
      threatenedByMinor=pos.attacksBy<KNIGHT>(~us)|pos.attacksBy<BISHOP>(~us)|threatenedByPawn;
      threatenedByRook=pos.attacksBy<ROOK>(~us)|threatenedByMinor;
      threatened=(pos.pieces(us,QUEEN)&threatenedByRook)
        |(pos.pieces(us,ROOK)&threatenedByMinor)
        |(pos.pieces(us,KNIGHT,BISHOP)&threatenedByPawn);
    }
    else{
      (void)threatened;
      (void)threatenedByPawn;
      (void)threatenedByMinor;
      (void)threatenedByRook;
    }
    for (auto& m : *this)
      if constexpr (Type==CAPTURES)
        m.value=6*static_cast<int>(pieceValue[MG][pos.pieceOn(toSq(m))])
          +(*captureHistory)[pos.movedPiece(m)][toSq(m)][typeOf(pos.pieceOn(toSq(m)))];
      else if constexpr (Type==QUIETS)
        m.value=2*(*mainHistory)[pos.stm()][fromTo(m)]
          +2*(*continuationHistory[0])[pos.movedPiece(m)][toSq(m)]
          +(*continuationHistory[1])[pos.movedPiece(m)][toSq(m)]
          +(*continuationHistory[3])[pos.movedPiece(m)][toSq(m)]
          +(*continuationHistory[5])[pos.movedPiece(m)][toSq(m)]
          +(threatened&fromSq(m)
            ?(typeOf(pos.movedPiece(m))==QUEEN&&!(toSq(m)&threatenedByRook)
              ?50000
              :typeOf(pos.movedPiece(m))==ROOK&&!(toSq(m)&threatenedByMinor)
              ?25000
              :!(toSq(m)&threatenedByPawn)
              ?15000
              :0)
            :0);
      else{
        if (pos.capture(m))
          m.value=pieceValue[MG][pos.pieceOn(toSq(m))]
            -typeOf(pos.movedPiece(m));
        else
          m.value=2*(*mainHistory)[pos.stm()][fromTo(m)]
            +2*(*continuationHistory[0])[pos.movedPiece(m)][toSq(m)]
            -(1<<28);
      }
  }

  template <MovePicker::PickType T, typename Pred>
  Move MovePicker::select(Pred filter){
    while (cur<endMoves){
      if (T==Best)
        std::swap(*cur,*std::max_element(cur,endMoves));
      if (*cur!=ttMove&&filter())
        return *cur++;
      cur++;
    }
    return MOVE_NONE;
  }

  Move MovePicker::nextMove(const bool skipQuiets){
  top:
    switch (stage){
    case MAIN_TT:
    case EVASION_TT:
    case QSEARCH_TT:
    case PROBCUT_TT:
      ++stage;
      return ttMove;
    case CAPTURE_INIT:
    case PROBCUT_INIT:
    case QCAPTURE_INIT:
      cur=endBadCaptures=moves;
      endMoves=generate<CAPTURES>(pos,cur);
      score<CAPTURES>();
      partialInsertionSort(cur,endMoves,-3000*depth);
      ++stage;
      goto top;
    case GOOD_CAPTURE:
      if (select<Next>([&]{
        if (pos.seeGe(*cur,static_cast<Value>(-69*cur->value/1024)))
          return true;

        *endBadCaptures=*cur;
        ++endBadCaptures;
        return false;
      }))
        return *(cur-1);

      cur=std::begin(refutations);
      endMoves=std::end(refutations);

      if (refutations[0].move==refutations[2].move||
        refutations[1].move==refutations[2].move)
        --endMoves;

      ++stage;
      [[fallthrough]];

    case REFUTATION:
      if (select<Next>([&]{
        return *cur!=MOVE_NONE
          &&!pos.capture(*cur)
          &&pos.pseudoLegal(*cur);
      }))
        return *(cur-1);
      ++stage;
      [[fallthrough]];
    case QUIET_INIT:
      if (!skipQuiets){
        cur=endBadCaptures;
        endMoves=generate<QUIETS>(pos,cur);
        score<QUIETS>();
        partialInsertionSort(cur,endMoves,-3000*depth);
      }
      ++stage;
      [[fallthrough]];
    case QUIET:
      if (!skipQuiets
        &&select<Next>([&]{
          return *cur!=refutations[0].move
            &&*cur!=refutations[1].move
            &&*cur!=refutations[2].move;
        }))
        return *(cur-1);
      cur=moves;
      endMoves=endBadCaptures;
      ++stage;
      [[fallthrough]];
    case BAD_CAPTURE:
      return select<Next>([]{ return true; });
    case EVASION_INIT:
      cur=moves;
      endMoves=generate<EVASIONS>(pos,cur);
      score<EVASIONS>();
      ++stage;
      [[fallthrough]];
    case EVASION:
      return select<Best>([]{ return true; });
    case PROBCUT:
      return select<Next>([&]{ return pos.seeGe(*cur,threshold); });
    case QCAPTURE:
      if (select<Next>([&]{
        return depth>DEPTH_QS_RECAPTURES
          ||toSq(*cur)==recaptureSquare;
      }))
        return *(cur-1);
      if (depth!=DEPTH_QS_CHECKS)
        return MOVE_NONE;
      ++stage;
      [[fallthrough]];
    case QCHECK_INIT:
      cur=moves;
      endMoves=generate<QUIET_CHECKS>(pos,cur);
      ++stage;
      [[fallthrough]];
    case QCHECK:
      return select<Next>([]{ return true; });
    default: ;
    }
    return MOVE_NONE;
  }
}
