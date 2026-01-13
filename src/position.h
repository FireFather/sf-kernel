#pragma once
#include <deque>
#include <memory>
#include <string>
#include "bitboard.h"
#include "types.h"
#include "nnue/nnue_accumulator.h"

namespace Nebula{
  struct StateInfo{
    uint64_t pawnKey;
    Value nonPawnMaterial[COLOR_NB];
    int castlingRights;
    int rule50;
    int pliesFromNull;
    Square epSquare;
    uint64_t key;
    uint64_t checkersBB;
    StateInfo* previous;
    uint64_t blockersForKing[COLOR_NB];
    uint64_t pinners[COLOR_NB];
    uint64_t checkSquares[PIECE_TYPE_NB];
    Piece capturedPiece;
    int repetition;
    Eval::Nnue::Accumulator accumulator;
    DirtyPiece dirtyPiece;
  };

  using StateListPtr = std::unique_ptr<std::deque<StateInfo>>;
  class Thread;

  class Position{
  public:
    static void init();
    Position() = default;
    Position(const Position&) = delete;
    Position& operator=(const Position&) = delete;
    Position& set(const std::string& fenStr, bool isChess960, StateInfo* si, Thread* th);
    Position& set(const std::string& code, Color c, StateInfo* si);
    [[nodiscard]] std::string fen() const;
    [[nodiscard]] uint64_t pieces(PieceType pt) const;
    [[nodiscard]] uint64_t pieces(PieceType pt1, PieceType pt2) const;
    [[nodiscard]] uint64_t pieces(Color c) const;
    [[nodiscard]] uint64_t pieces(Color c, PieceType pt) const;
    [[nodiscard]] uint64_t pieces(Color c, PieceType pt1, PieceType pt2) const;
    [[nodiscard]] Piece pieceOn(Square s) const;
    [[nodiscard]] Square epSquare() const;
    [[nodiscard]] bool empty(Square s) const;
    template <PieceType Pt>
    [[nodiscard]] int count(Color c) const;
    template <PieceType Pt>
    [[nodiscard]] int count() const;
    template <PieceType Pt>
    [[nodiscard]] Square square(Color c) const;
    [[nodiscard]] bool isOnSemiopenFile(Color c, Square s) const;
    [[nodiscard]] CastlingRights castlingRights(Color c) const;
    [[nodiscard]] bool canCastle(CastlingRights cr) const;
    [[nodiscard]] bool castlingImpeded(CastlingRights cr) const;
    [[nodiscard]] Square castleRookSquare(CastlingRights cr) const;
    [[nodiscard]] uint64_t checkers() const;
    [[nodiscard]] uint64_t blockersForKing(Color c) const;
    [[nodiscard]] uint64_t checkSquares(PieceType pt) const;
    [[nodiscard]] uint64_t pinners(Color c) const;
    [[nodiscard]] uint64_t attackersTo(Square s) const;
    [[nodiscard]] uint64_t attackersTo(Square s, uint64_t occupied) const;
    uint64_t sliderBlockers(uint64_t sliders, Square s, uint64_t& pinners) const;
    template <PieceType Pt>
    [[nodiscard]] uint64_t attacksBy(Color c) const;
    [[nodiscard]] bool legal(Move m) const;
    [[nodiscard]] bool pseudoLegal(Move m) const;
    [[nodiscard]] bool capture(Move m) const;
    [[nodiscard]] bool givesCheck(Move m) const;
    [[nodiscard]] Piece movedPiece(Move m) const;
    [[nodiscard]] Piece capturedPiece() const;
    [[nodiscard]] bool pawnPassed(Color c, Square s) const;
    [[nodiscard]] bool oppositeBishops() const;
    [[nodiscard]] int pawnsOnSameColorSquares(Color c, Square s) const;
    void doMove(Move m, StateInfo& newSt);
    void doMove(Move m, StateInfo& newSt, bool givesCheck);
    void undoMove(Move m);
    void doNullMove(StateInfo& newSt);
    void undoNullMove();
    [[nodiscard]] bool seeGe(Move m, Value threshold=VALUE_ZERO) const;
    [[nodiscard]] uint64_t key() const;
    [[nodiscard]] uint64_t keyAfter(Move m) const;
    [[nodiscard]] uint64_t pawnKey() const;
    [[nodiscard]] Color stm() const;
    [[nodiscard]] int gameply() const;
    [[nodiscard]] bool isChess960() const;
    [[nodiscard]] Thread* thisthread() const;
    [[nodiscard]] bool isDraw(int ply) const;
    [[nodiscard]] bool hasGameCycle(int ply) const;
    [[nodiscard]] bool hasRepeated() const;
    [[nodiscard]] int rule50Count() const;
    [[nodiscard]] Value nonPawnMaterial(Color c) const;
    [[nodiscard]] Value nonPawnMaterial() const;
    [[nodiscard]] StateInfo* state() const;
    void putPiece(Piece pc, Square s);
    void removePiece(Square s);
  private:
    void setCastlingRight(Color c, Square rfrom);
    void setState(StateInfo* si) const;
    void setCheckInfo(StateInfo* si) const;
    void movePiece(Square from, Square to);
    template <bool Do>
    void doCastling(Color us, Square from, Square& to, Square& rfrom, Square& rto);
    template <bool AfterMove>
    [[nodiscard]] uint64_t adjustKey50(uint64_t k) const;
    Piece board[SQUARE_NB];
    uint64_t byTypeBB[PIECE_TYPE_NB];
    uint64_t byColorBB[COLOR_NB];
    int pieceCount[PIECE_NB];
    int castlingRightsMask[SQUARE_NB];
    Square castlingRookSquare[CASTLING_RIGHT_NB];
    uint64_t castlingPath[CASTLING_RIGHT_NB];
    Thread* thisThread;
    StateInfo* st;
    int gamePly;
    Color sideToMove;
    bool chess960;
  };

  extern std::ostream& operator<<(std::ostream& os, const Position& pos);
  inline Color Position::stm() const{ return sideToMove; }
  inline Piece Position::pieceOn(const Square s) const{ return board[s]; }
  inline bool Position::empty(const Square s) const{ return pieceOn(s)==NO_PIECE; }
  inline Piece Position::movedPiece(const Move m) const{ return pieceOn(fromSq(m)); }
  inline uint64_t Position::pieces(const PieceType pt=ALL_PIECES) const{ return byTypeBB[pt]; }
  inline uint64_t Position::pieces(const PieceType pt1, const PieceType pt2) const{ return pieces(pt1)|pieces(pt2); }
  inline uint64_t Position::pieces(const Color c) const{ return byColorBB[c]; }
  inline uint64_t Position::pieces(const Color c, const PieceType pt) const{ return pieces(c)&pieces(pt); }

  inline uint64_t Position::pieces(const Color c, const PieceType pt1, const PieceType pt2) const{
    return pieces(c)&(pieces(pt1)|pieces(pt2));
  }

  template <PieceType Pt>
  int Position::count(const Color c) const{ return pieceCount[makePiece(c,Pt)]; }

  template <PieceType Pt>
  int Position::count() const{ return count<Pt>(WHITE)+count<Pt>(BLACK); }

  template <PieceType Pt>
  Square Position::square(const Color c) const{ return lsb(pieces(c,Pt)); }

  inline Square Position::epSquare() const{ return st->epSquare; }
  inline bool Position::isOnSemiopenFile(const Color c, const Square s) const{ return !(pieces(c,PAWN)&fileBb(s)); }
  inline bool Position::canCastle(const CastlingRights cr) const{ return st->castlingRights&cr; }

  inline CastlingRights Position::castlingRights(const Color c) const{
    return c&static_cast<CastlingRights>(st->castlingRights);
  }

  inline bool Position::castlingImpeded(const CastlingRights cr) const{ return pieces()&castlingPath[cr]; }
  inline Square Position::castleRookSquare(const CastlingRights cr) const{ return castlingRookSquare[cr]; }
  inline uint64_t Position::attackersTo(const Square s) const{ return attackersTo(s,pieces()); }

  template <PieceType Pt>
  uint64_t Position::attacksBy(const Color c) const{
    if constexpr (Pt==PAWN)
      return c==WHITE
             ?pawnAttacksBb<WHITE>(pieces(WHITE,PAWN))
             :pawnAttacksBb<BLACK>(pieces(BLACK,PAWN));
    else{
      uint64_t threats=0;
      uint64_t attackers=pieces(c,Pt);
      while (attackers)
        threats|=attacksBb<Pt>(popLsb(attackers),pieces());
      return threats;
    }
  }

  inline uint64_t Position::checkers() const{ return st->checkersBB; }
  inline uint64_t Position::blockersForKing(const Color c) const{ return st->blockersForKing[c]; }
  inline uint64_t Position::pinners(const Color c) const{ return st->pinners[c]; }
  inline uint64_t Position::checkSquares(const PieceType pt) const{ return st->checkSquares[pt]; }

  inline bool Position::pawnPassed(const Color c, const Square s) const{
    return !(pieces(~c,PAWN)&passedPawnSpan(c,s));
  }

  inline int Position::pawnsOnSameColorSquares(const Color c, const Square s) const{
    return popcnt(pieces(c,PAWN)&(darkSquares&s?darkSquares:~darkSquares));
  }

  inline uint64_t Position::key() const{ return adjustKey50<false>(st->key); }

  template <bool AfterMove>
  uint64_t Position::adjustKey50(const uint64_t k) const{
    return st->rule50<14-AfterMove
           ?k
           :k^makeKey((st->rule50-(14-AfterMove))/8);
  }

  inline uint64_t Position::pawnKey() const{ return st->pawnKey; }
  inline Value Position::nonPawnMaterial(const Color c) const{ return st->nonPawnMaterial[c]; }
  inline Value Position::nonPawnMaterial() const{ return nonPawnMaterial(WHITE)+nonPawnMaterial(BLACK); }
  inline int Position::gameply() const{ return gamePly; }
  inline int Position::rule50Count() const{ return st->rule50; }

  inline bool Position::oppositeBishops() const{
    return count<BISHOP>(WHITE)==1
      &&count<BISHOP>(BLACK)==1
      &&oppositeColors(square<BISHOP>(WHITE),square<BISHOP>(BLACK));
  }

  inline bool Position::isChess960() const{ return chess960; }

  inline bool Position::capture(const Move m) const{
    return (!empty(toSq(m))&&typeOf(m)!=CASTLING)||typeOf(m)==EN_PASSANT;
  }

  inline Piece Position::capturedPiece() const{ return st->capturedPiece; }
  inline Thread* Position::thisthread() const{ return thisThread; }

  inline void Position::putPiece(const Piece pc, const Square s){
    board[s]=pc;
    byTypeBB[ALL_PIECES]|=byTypeBB[typeOf(pc)]|=s;
    byColorBB[colorOf(pc)]|=s;
    pieceCount[pc]++;
    pieceCount[makePiece(colorOf(pc),ALL_PIECES)]++;
  }

  inline void Position::removePiece(const Square s){
    const Piece pc=board[s];
    byTypeBB[ALL_PIECES]^=s;
    byTypeBB[typeOf(pc)]^=s;
    byColorBB[colorOf(pc)]^=s;
    board[s]=NO_PIECE;
    pieceCount[pc]--;
    pieceCount[makePiece(colorOf(pc),ALL_PIECES)]--;
  }

  inline void Position::movePiece(const Square from, const Square to){
    const Piece pc=board[from];
    const uint64_t fromTo=from|to;
    byTypeBB[ALL_PIECES]^=fromTo;
    byTypeBB[typeOf(pc)]^=fromTo;
    byColorBB[colorOf(pc)]^=fromTo;
    board[from]=NO_PIECE;
    board[to]=pc;
  }

  inline void Position::doMove(const Move m, StateInfo& newSt){ doMove(m,newSt,givesCheck(m)); }
  inline StateInfo* Position::state() const{ return st; }
}
