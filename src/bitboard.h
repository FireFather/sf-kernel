#pragma once
#ifdef _MSC_VER
#include <bit>
#endif
#include "types.h"
#ifdef _MSC_VER
#define UNREACHABLE() __assume(0)
#elif defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE() __builtin_unreachable()
#else
#define UNREACHABLE() do {} while(0)
#endif
namespace Nebula{
#ifdef _MSC_VER
  inline Square popcnt(const uint64_t bb){ return static_cast<Square>(std::popcount(bb)); }
  inline Square lsb(const uint64_t bb){ return static_cast<Square>(std::countr_zero(bb)); }
  inline Square msb(const uint64_t bb){ return static_cast<Square>(std::countl_zero(bb)); }
  inline void prefetch(void* address){ _mm_prefetch(static_cast<char*>(address), _MM_HINT_T0); }
#elif defined(__GNUC__)
  inline Square popcnt(uint64_t bb){ return static_cast<Square>(__builtin_popcountll(bb)); }
  inline Square lsb(uint64_t bb){ return static_cast<Square>(__builtin_ctzll(bb)); }
  inline Square msb(uint64_t bb){ return static_cast<Square>(63^__builtin_clzll(bb)); }
  inline void prefetch(void* address){ __builtin_prefetch(address); }
#endif
  inline uint64_t leastSignificantSquareBb(const uint64_t b){
    return b&static_cast<uint64_t>(-static_cast<int64_t>(b));
  }

  inline Square popLsb(uint64_t& b){
    const auto s=lsb(b);
    b&=b-1;
    return s;
  }

  inline Square frontmostSq(const Color c, const uint64_t b){ return c==WHITE?msb(b):lsb(b); }

  namespace Bitboards{
    void init();
  }

  constexpr uint64_t allSquares=~static_cast<uint64_t>(0);
  constexpr uint64_t darkSquares=0xAA55AA55AA55AA55ULL;
  constexpr uint64_t fileAbb=0x0101010101010101ULL;
  constexpr uint64_t fileBbb=fileAbb<<1;
  constexpr uint64_t fileCbb=fileAbb<<2;
  constexpr uint64_t fileDbb=fileAbb<<3;
  constexpr uint64_t fileEbb=fileAbb<<4;
  constexpr uint64_t fileFbb=fileAbb<<5;
  constexpr uint64_t fileGbb=fileAbb<<6;
  constexpr uint64_t fileHbb=fileAbb<<7;
  constexpr uint64_t rank1Bb=0xFF;
  constexpr uint64_t rank2Bb=rank1Bb<<(8*1);
  constexpr uint64_t rank3Bb=rank1Bb<<(8*2);
  constexpr uint64_t rank4Bb=rank1Bb<<(8*3);
  constexpr uint64_t rank5Bb=rank1Bb<<(8*4);
  constexpr uint64_t rank6Bb=rank1Bb<<(8*5);
  constexpr uint64_t rank7Bb=rank1Bb<<(8*6);
  constexpr uint64_t rank8Bb=rank1Bb<<(8*7);
  constexpr uint64_t queenSide=fileAbb|fileBbb|fileCbb|fileDbb;
  constexpr uint64_t centerFiles=fileCbb|fileDbb|fileEbb|fileFbb;
  constexpr uint64_t kingSide=fileEbb|fileFbb|fileGbb|fileHbb;
  constexpr uint64_t center=(fileDbb|fileEbb)&(rank4Bb|rank5Bb);
  constexpr uint64_t kingFlank[FILE_NB]={
    queenSide^fileDbb,queenSide,queenSide,
    centerFiles,centerFiles,
    kingSide,kingSide,kingSide^fileEbb
  };
  extern uint8_t popCnt16[1<<16];
  extern uint8_t squareDistance[SQUARE_NB][SQUARE_NB];
  extern uint64_t squareBb[SQUARE_NB];
  extern uint64_t betweenBb[SQUARE_NB][SQUARE_NB];
  extern uint64_t lineBb[SQUARE_NB][SQUARE_NB];
  extern uint64_t pseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
  extern uint64_t pawnAttacks[COLOR_NB][SQUARE_NB];

  struct Magic{
    uint64_t mask;
    uint64_t magic;
    uint64_t* attacks;
    unsigned shift;

    [[nodiscard]] unsigned index(const uint64_t occupied) const{
      if (hasPext)
        return pext(occupied,mask);
      if (is64Bit)
        return static_cast<unsigned>(((occupied&mask)*magic)>>shift);
      unsigned lo=static_cast<unsigned>(occupied)&static_cast<unsigned>(mask);
      unsigned hi=static_cast<unsigned>(occupied>>32)&static_cast<unsigned>(mask>>32);
      return (lo*static_cast<unsigned>(magic)^hi*static_cast<unsigned>(magic>>32))>>shift;
    }
  };

  extern Magic rookMagics[SQUARE_NB];
  extern Magic bishopMagics[SQUARE_NB];
  inline uint64_t getSquareBb(const Square s){ return squareBb[s]; }
  inline uint64_t operator&(const uint64_t b, const Square s){ return b&getSquareBb(s); }
  inline uint64_t operator|(const uint64_t b, const Square s){ return b|getSquareBb(s); }
  inline uint64_t operator^(const uint64_t b, const Square s){ return b^getSquareBb(s); }
  inline uint64_t& operator|=(uint64_t& b, const Square s){ return b|=getSquareBb(s); }
  inline uint64_t& operator^=(uint64_t& b, const Square s){ return b^=getSquareBb(s); }
  inline uint64_t operator&(const Square s, const uint64_t b){ return b&s; }
  inline uint64_t operator|(const Square s, const uint64_t b){ return b|s; }
  inline uint64_t operator^(const Square s, const uint64_t b){ return b^s; }
  inline uint64_t operator|(const Square s1, const Square s2){ return getSquareBb(s1)|s2; }
  constexpr bool moreThanOne(const uint64_t b){ return b&(b-1); }

  constexpr bool oppositeColors(const Square s1, const Square s2){
    return (static_cast<int>(s1)+static_cast<int>(rankOf(s1))+
      static_cast<int>(s2)+static_cast<int>(rankOf(s2)))&1;
  }

  constexpr uint64_t rankBb(const Rank r){ return rank1Bb<<(8*r); }
  constexpr uint64_t rankBb(const Square s){ return rankBb(rankOf(s)); }
  constexpr uint64_t fileBb(const File f){ return fileAbb<<f; }
  constexpr uint64_t fileBb(const Square s){ return fileBb(fileOf(s)); }

  template <Direction D>
  constexpr uint64_t shift(const uint64_t b){
    return D==NORTH
           ?b<<8
           :D==SOUTH
           ?b>>8
           :D==NORTH+NORTH
           ?b<<16
           :D==SOUTH+SOUTH
           ?b>>16
           :D==EAST
           ?(b&~fileHbb)<<1
           :D==WEST
           ?(b&~fileAbb)>>1
           :D==NORTH_EAST
           ?(b&~fileHbb)<<9
           :D==NORTH_WEST
           ?(b&~fileAbb)<<7
           :D==SOUTH_EAST
           ?(b&~fileHbb)>>7
           :D==SOUTH_WEST
           ?(b&~fileAbb)>>9
           :0;
  }

  template <Color C>
  constexpr uint64_t pawnAttacksBb(const uint64_t b){
    return C==WHITE
           ?shift<NORTH_WEST>(b)|shift<NORTH_EAST>(b)
           :shift<SOUTH_WEST>(b)|shift<SOUTH_EAST>(b);
  }

  inline uint64_t pawnAttacksBb(const Color c, const Square s){ return pawnAttacks[c][s]; }

  template <Color C>
  constexpr uint64_t pawnDoubleAttacksBb(const uint64_t b){
    return C==WHITE
           ?shift<NORTH_WEST>(b)&shift<NORTH_EAST>(b)
           :shift<SOUTH_WEST>(b)&shift<SOUTH_EAST>(b);
  }

  constexpr uint64_t adjacentFilesBb(const Square s){ return shift<EAST>(fileBb(s))|shift<WEST>(fileBb(s)); }
  inline uint64_t getLineBb(const Square s1, const Square s2){ return lineBb[s1][s2]; }
  inline uint64_t getBetweenBb(const Square s1, const Square s2){ return betweenBb[s1][s2]; }

  constexpr uint64_t forwardRanksBb(const Color c, const Square s){
    return c==WHITE
           ?~rank1Bb<<8*relativeRank(WHITE,s)
           :~rank8Bb>>8*relativeRank(BLACK,s);
  }

  constexpr uint64_t forwardFileBb(const Color c, const Square s){ return forwardRanksBb(c,s)&fileBb(s); }
  constexpr uint64_t pawnAttackSpan(const Color c, const Square s){ return forwardRanksBb(c,s)&adjacentFilesBb(s); }
  constexpr uint64_t passedPawnSpan(const Color c, const Square s){ return pawnAttackSpan(c,s)|forwardFileBb(c,s); }
  inline bool aligned(const Square s1, const Square s2, const Square s3){ return getLineBb(s1,s2)&s3; }
  template <typename T1 = Square>
  int distance(Square x, Square y);

  template <>
  inline int distance<File>(const Square x, const Square y){ return std::abs(fileOf(x)-fileOf(y)); }

  template <>
  inline int distance<Rank>(const Square x, const Square y){ return std::abs(rankOf(x)-rankOf(y)); }

  template <>
  inline int distance<Square>(const Square x, const Square y){ return squareDistance[x][y]; }

  inline int edgeDistance(const File f){ return std::min(f,static_cast<File>(FILE_H-f)); }
  inline int edgeDistance(const Rank r){ return std::min(r,static_cast<Rank>(RANK_8-r)); }

  template <PieceType Pt>
  uint64_t attacksBb(const Square s){ return pseudoAttacks[Pt][s]; }

  template <PieceType Pt>
  uint64_t attacksBb(const Square s, const uint64_t occupied){
    if constexpr (Pt==BISHOP)
      return bishopMagics[s].attacks[bishopMagics[s].index(occupied)];
    else if constexpr (Pt==ROOK)
      return rookMagics[s].attacks[rookMagics[s].index(occupied)];
    else if constexpr (Pt==QUEEN)
      return attacksBb<BISHOP>(s,occupied)|attacksBb<ROOK>(s,occupied);
    else
      return pseudoAttacks[Pt][s];
  }

  inline uint64_t attacksBb(const PieceType pt, const Square s, const uint64_t occupied){
    switch (pt){
    case BISHOP: return attacksBb<BISHOP>(s,occupied);
    case ROOK: return attacksBb<ROOK>(s,occupied);
    case QUEEN: return attacksBb<BISHOP>(s,occupied)|attacksBb<ROOK>(s,occupied);
    case PAWN:
    case KNIGHT:
    case KING:
      return pseudoAttacks[pt][s];
    case NO_PIECE_TYPE:
    case ALL_PIECES:
    case PIECE_TYPE_NB:
      return 0;
    }
    UNREACHABLE();
  }
}
