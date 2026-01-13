#pragma once
#include <cstdint>
#include <chrono>
#include <mutex>
#if defined(_WIN64) && defined(_MSC_VER)
#define IS_64BIT
#endif
#if defined(USE_POPCNT) && (defined(__INTEL_COMPILER) || defined(_MSC_VER))
#include <nmmintrin.h>
#endif
#ifdef USE_PEXT
#include <immintrin.h>
#define pext(b, m) _pext_u64(b, m)
#else
#define pext(b, m) 0
#endif
namespace Nebula{
#ifdef USE_POPCNT
  constexpr bool hasPopCnt=true;
#else
  constexpr bool hasPopCnt=false;
#endif
#ifdef USE_PEXT
  constexpr bool hasPext=true;
#else
  constexpr bool hasPext=false;
#endif
#ifdef IS_64BIT
  constexpr bool is64Bit=true;
#else
  constexpr bool is64Bit=false;
#endif
  constexpr int maxMoves=256;
  constexpr int maxPly=246;

  enum Move : int{
    MOVE_NONE=0,
    MOVE_NULL=65
  };

  enum MoveType :uint16_t{
    NORMAL=0,
    PROMOTION=1<<14,
    EN_PASSANT=2<<14,
    CASTLING=3<<14
  };

  enum Color:uint8_t{
    WHITE=0, BLACK=1, COLOR_NB=2
  };

  enum CastlingRights :uint8_t{
    NO_CASTLING=0,
    WHITE_OO=1,
    WHITE_OOO=WHITE_OO<<1,
    BLACK_OO=WHITE_OO<<2,
    BLACK_OOO=WHITE_OO<<3,
    KING_SIDE=WHITE_OO|BLACK_OO,
    QUEEN_SIDE=WHITE_OOO|BLACK_OOO,
    WHITE_CASTLING=WHITE_OO|WHITE_OOO,
    BLACK_CASTLING=BLACK_OO|BLACK_OOO,
    ANY_CASTLING=WHITE_CASTLING|BLACK_CASTLING,
    CASTLING_RIGHT_NB=16
  };

  enum Phase :uint8_t{
    PHASE_ENDGAME=0,
    PHASE_MIDGAME=128,
    MG=0, EG=1, PHASE_NB=2
  };

  enum ScaleFactor :uint8_t{
    SCALE_FACTOR_DRAW=0,
    SCALE_FACTOR_NORMAL=64,
    SCALE_FACTOR_MAX=128,
    SCALE_FACTOR_NONE=255
  };

  enum Bound :uint8_t{
    BOUND_NONE=0,
    BOUND_UPPER=1,
    BOUND_LOWER=2,
    BOUND_EXACT=BOUND_UPPER|BOUND_LOWER
  };

  enum Value : int{
    VALUE_ZERO=0,
    VALUE_DRAW=0,
    VALUE_KNOWN_WIN=10000,
    VALUE_MATE=32000,
    VALUE_INFINITE=32001,
    VALUE_NONE=32002,
    VALUE_TB_WIN_IN_MAX_PLY=VALUE_MATE-2*maxPly,
    VALUE_TB_LOSS_IN_MAX_PLY=-VALUE_TB_WIN_IN_MAX_PLY,
    VALUE_MATE_IN_MAX_PLY=VALUE_MATE-maxPly,
    VALUE_MATED_IN_MAX_PLY=-VALUE_MATE_IN_MAX_PLY,
    PawnValueMg=126, PawnValueEg=208,
    KnightValueMg=781, KnightValueEg=854,
    BishopValueMg=825, BishopValueEg=915,
    RookValueMg=1276, RookValueEg=1380,
    QueenValueMg=2538, QueenValueEg=2682,
    MidgameLimit=15258, EndgameLimit=3915
  };

  enum PieceType : uint8_t{
    NO_PIECE_TYPE=0,
    PAWN=1, KNIGHT=2, BISHOP=3, ROOK=4, QUEEN=5, KING=6,
    ALL_PIECES=7,
    PIECE_TYPE_NB=8
  };

  enum Piece :uint8_t{
    NO_PIECE=0,
    W_PAWN=PAWN, W_KNIGHT=2, W_BISHOP=3, W_ROOK=4, W_QUEEN=5, W_KING=6,
    B_PAWN=PAWN+8, B_KNIGHT=10, B_BISHOP=11, B_ROOK=12, B_QUEEN=13, B_KING=14,
    PIECE_NB=16
  };

  constexpr Value pieceValue[PHASE_NB][PIECE_NB]={
    {
      VALUE_ZERO,PawnValueMg,KnightValueMg,BishopValueMg,RookValueMg,QueenValueMg,VALUE_ZERO,VALUE_ZERO,
      VALUE_ZERO,PawnValueMg,KnightValueMg,BishopValueMg,RookValueMg,QueenValueMg,VALUE_ZERO,VALUE_ZERO
    },
    {
      VALUE_ZERO,PawnValueEg,KnightValueEg,BishopValueEg,RookValueEg,QueenValueEg,VALUE_ZERO,VALUE_ZERO,
      VALUE_ZERO,PawnValueEg,KnightValueEg,BishopValueEg,RookValueEg,QueenValueEg,VALUE_ZERO,VALUE_ZERO
    }
  };
  using Depth = int;

  enum : int8_t{
    DEPTH_QS_CHECKS=0,
    DEPTH_QS_NO_CHECKS=-1,
    DEPTH_QS_RECAPTURES=-5,
    DEPTH_NONE=-6,
    DEPTH_OFFSET=-7
  };

  enum Square : uint8_t{
    SQ_A1=0, SQ_B1=1, SQ_C1=2, SQ_D1=3, SQ_E1=4, SQ_F1=5, SQ_G1=6, SQ_H1=7,
    SQ_A2=8, SQ_B2=9, SQ_C2=10, SQ_D2=11, SQ_E2=12, SQ_F2=13, SQ_G2=14, SQ_H2=15,
    SQ_A3=16, SQ_B3=17, SQ_C3=18, SQ_D3=19, SQ_E3=20, SQ_F3=21, SQ_G3=22, SQ_H3=23,
    SQ_A4=24, SQ_B4=25, SQ_C4=26, SQ_D4=27, SQ_E4=28, SQ_F4=29, SQ_G4=30, SQ_H4=31,
    SQ_A5=32, SQ_B5=33, SQ_C5=34, SQ_D5=35, SQ_E5=36, SQ_F5=37, SQ_G5=38, SQ_H5=39,
    SQ_A6=40, SQ_B6=41, SQ_C6=42, SQ_D6=43, SQ_E6=44, SQ_F6=45, SQ_G6=46, SQ_H6=47,
    SQ_A7=48, SQ_B7=49, SQ_C7=50, SQ_D7=51, SQ_E7=52, SQ_F7=53, SQ_G7=54, SQ_H7=55,
    SQ_A8=56, SQ_B8=57, SQ_C8=58, SQ_D8=59, SQ_E8=60, SQ_F8=61, SQ_G8=62, SQ_H8=63,
    SQ_NONE=64,
    SQUARE_ZERO=0,
    SQUARE_NB=64
  };

  enum Direction : int{
    NORTH=8,
    EAST=1,
    SOUTH=-NORTH,
    WEST=-EAST,
    NORTH_EAST=NORTH+EAST,
    SOUTH_EAST=SOUTH+EAST,
    SOUTH_WEST=SOUTH+WEST,
    NORTH_WEST=NORTH+WEST
  };

  enum File : int{
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB
  };

  enum Rank : int{
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB
  };

  struct DirtyPiece{
    int dirty_num;
    Piece piece[3];
    Square from[3];
    Square to[3];
  };

  enum Score : int{ SCORE_ZERO };

  constexpr Score calcScore(const int mg, const int eg){
    return static_cast<Score>(static_cast<int>(static_cast<unsigned int>(eg)<<16)+mg);
  }

  inline Value egValue(const Score s){
    union{
      uint16_t u;
      int16_t s;
    } eg={static_cast<uint16_t>(static_cast<unsigned>(s+0x8000)>>16)};
    return static_cast<Value>(eg.s);
  }

  inline Value mgValue(const Score s){
    union{
      uint16_t u;
      int16_t s;
    } mg={static_cast<uint16_t>(static_cast<unsigned>(s))};
    return static_cast<Value>(mg.s);
  }

  template <typename E>
  constexpr E enumAdd(E e, const int i){ return E(static_cast<int>(e)+i); }

  template <typename E>
  constexpr E enumSub(E e, const int i){ return E(static_cast<int>(e)-i); }

  template <typename E>
  constexpr E enumNeg(E e){ return E(-static_cast<int>(e)); }

  template <typename E>
  constexpr E enumMul(E e, const int i){ return E(static_cast<int>(e)*i); }

  template <typename E>
  constexpr E enumDiv(E e, const int i){ return E(static_cast<int>(e)/i); }

  template <typename E>
  constexpr int enumDiv(E a, E b){ return static_cast<int>(a)/static_cast<int>(b); }

  template <typename E>
  E& enumAddAssign(E& e, int i){ return e=enumAdd(e,i); }

  template <typename E>
  E& enumSubAssign(E& e, int i){ return e=enumSub(e,i); }

  template <typename E>
  E& enumMulAssign(E& e, int i){ return e=enumMul(e,i); }

  template <typename E>
  E& enumDivAssign(E& e, int i){ return e=enumDiv(e,i); }

  template <typename E>
  E& enumInc(E& e){ return e=E(static_cast<int>(e)+1); }

  template <typename E>
  E& enumDec(E& e){ return e=E(static_cast<int>(e)-1); }

  constexpr Color operator~(const Color c){ return static_cast<Color>(c^BLACK); }
  constexpr Direction operator+(const Direction d, const int i){ return enumAdd(d,i); }
  constexpr Direction operator-(const Direction d, const int i){ return enumSub(d,i); }
  constexpr Direction operator-(const Direction d){ return enumNeg(d); }
  inline Direction& operator+=(Direction& d, const int i){ return enumAddAssign(d,i); }
  inline Direction& operator-=(Direction& d, const int i){ return enumSubAssign(d,i); }
  constexpr Direction operator*(const int i, const Direction d){ return enumMul(d,i); }
  constexpr Direction operator*(const Direction d, const int i){ return enumMul(d,i); }
  constexpr Direction operator/(const Direction d, const int i){ return enumDiv(d,i); }
  constexpr int operator/(const Direction a, const Direction b){ return enumDiv(a,b); }
  inline Direction& operator*=(Direction& d, const int i){ return enumMulAssign(d,i); }
  inline Direction& operator/=(Direction& d, const int i){ return enumDivAssign(d,i); }
  inline File& operator++(File& f){ return enumInc(f); }
  inline File& operator--(File& f){ return enumDec(f); }
  inline Rank& operator++(Rank& r){ return enumInc(r); }
  inline Rank& operator--(Rank& r){ return enumDec(r); }
  inline Piece& operator++(Piece& p){ return enumInc(p); }
  inline Piece& operator--(Piece& p){ return enumDec(p); }
  constexpr Piece operator~(const Piece pc){ return static_cast<Piece>(pc^8); }
  inline PieceType& operator++(PieceType& p){ return enumInc(p); }
  inline PieceType& operator--(PieceType& p){ return enumDec(p); }
  constexpr Score operator+(const Score s, const int i){ return enumAdd(s,i); }
  constexpr Score operator-(const Score s, const int i){ return enumSub(s,i); }
  constexpr Score operator-(const Score s){ return enumNeg(s); }
  inline Score& operator+=(Score& s, const int i){ return enumAddAssign(s,i); }
  inline Score& operator-=(Score& s, const int i){ return enumSubAssign(s,i); }
  Score operator*(Score, Score) = delete;
  inline Score operator/(const Score s, const int i){ return calcScore(mgValue(s)/i,egValue(s)/i); }

  inline Score operator*(const Score s, const int i){
    const auto result=static_cast<Score>(static_cast<int>(s)*i);
    return result;
  }

  inline Score operator*(const Score s, const bool b){ return b?s:SCORE_ZERO; }
  inline Square& operator++(Square& s){ return enumInc(s); }
  inline Square& operator--(Square& s){ return enumDec(s); }

  constexpr Square operator+(const Square s, const Direction d){
    return static_cast<Square>(static_cast<int>(s)+static_cast<int>(d));
  }

  constexpr Square operator-(const Square s, const Direction d){
    return static_cast<Square>(static_cast<int>(s)-static_cast<int>(d));
  }

  inline Square& operator+=(Square& s, const Direction d){ return s=s+d; }
  inline Square& operator-=(Square& s, const Direction d){ return s=s-d; }
  constexpr Value operator+(const Value v, const int i){ return enumAdd(v,i); }
  constexpr Value operator-(const Value v, const int i){ return enumSub(v,i); }
  constexpr Value operator-(const Value v){ return enumNeg(v); }
  inline Value& operator+=(Value& v, const int i){ return enumAddAssign(v,i); }
  inline Value& operator-=(Value& v, const int i){ return enumSubAssign(v,i); }
  constexpr Value operator*(const int i, const Value v){ return enumMul(v,i); }
  constexpr Value operator*(const Value v, const int i){ return enumMul(v,i); }
  constexpr Value operator/(const Value v, const int i){ return enumDiv(v,i); }
  constexpr int operator/(const Value a, const Value b){ return enumDiv(a,b); }
  inline Value& operator*=(Value& v, const int i){ return enumMulAssign(v,i); }
  inline Value& operator/=(Value& v, const int i){ return enumDivAssign(v,i); }

  enum SyncCout :uint8_t{ IO_LOCK, IO_UNLOCK };

  std::ostream& operator<<(std::ostream&, SyncCout);
  constexpr Square flipRank(const Square s){ return static_cast<Square>(s^SQ_A8); }
  constexpr Square flipFile(const Square s){ return static_cast<Square>(s^SQ_H1); }

  constexpr CastlingRights operator&(const Color c, const CastlingRights cr){
    return static_cast<CastlingRights>((c==WHITE?WHITE_CASTLING:BLACK_CASTLING)&cr);
  }

  constexpr Direction pawnPush(const Color c){ return c==WHITE?NORTH:SOUTH; }
  constexpr File fileOf(const Square s){ return static_cast<File>(s&7); }
  constexpr MoveType typeOf(const Move m){ return static_cast<MoveType>(m&3<<14); }
  constexpr PieceType typeOf(const Piece pc){ return static_cast<PieceType>(pc&7); }
  constexpr Rank rankOf(const Square s){ return static_cast<Rank>(s>>3); }
  constexpr Move makeMove(const Square from, const Square to){ return static_cast<Move>((from<<6)+to); }
  constexpr Piece makePiece(const Color c, const PieceType pt){ return static_cast<Piece>((c<<3)+pt); }
  constexpr PieceType promotionType(const Move m){ return static_cast<PieceType>((m>>12&3)+KNIGHT); }
  constexpr Rank relativeRank(const Color c, const Rank r){ return static_cast<Rank>(r^c*7); }
  constexpr Rank relativeRank(const Color c, const Square s){ return relativeRank(c,rankOf(s)); }
  constexpr Square fromSq(const Move m){ return static_cast<Square>(m>>6&0x3F); }
  constexpr Square toSq(const Move m){ return static_cast<Square>(m&0x3F); }
  constexpr int fromTo(const Move m){ return m&0xFFF; }
  constexpr Square makeSquare(const File f, const Rank r){ return static_cast<Square>((r<<3)+f); }
  constexpr Square relativeSquare(const Color c, const Square s){ return static_cast<Square>(s^c*56); }
  constexpr uint64_t makeKey(const uint64_t seed){ return seed*6364136223846793005ULL+1442695040888963407ULL; }
  constexpr Value matedIn(const int ply){ return -VALUE_MATE+ply; }
  constexpr Value mateIn(const int ply){ return VALUE_MATE-ply; }
  inline Color colorOf(const Piece pc){ return static_cast<Color>(pc>>3); }
  constexpr bool isOk(const Move m){ return fromSq(m)!=toSq(m); }
  constexpr bool isOk(const Square s){ return s>=SQ_A1&&s<=SQ_H8; }

  template <MoveType T>
  constexpr Move make(const Square from, const Square to, const PieceType pt=KNIGHT){
    return static_cast<Move>(T+((pt-KNIGHT)<<12)+(from<<6)+to);
  }
}
