#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iomanip>
#include <sstream>
#include "bitboard.h"
#include "misc.h"
#include "movegen.h"
#include "position.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
using std::string;

namespace Nebula{
  namespace Zobrist{
    namespace{
      uint64_t psq[PIECE_NB][SQUARE_NB];
      uint64_t enpassant[FILE_NB];
      uint64_t castling[CASTLING_RIGHT_NB];
      uint64_t side, noPawns;
    }
  }

  namespace{
    int h1(const uint64_t h){ return static_cast<int>(h&0x1fff); }
    int h2(const uint64_t h){ return static_cast<int>(h>>16&0x1fff); }
    uint64_t cuckoo[8192];
    Move cuckooMove[8192];
    const string pieceToChar(" PNBRQK  pnbrqk");
    constexpr Piece pcs[]={
      W_PAWN,W_KNIGHT,W_BISHOP,W_ROOK,W_QUEEN,W_KING,
      B_PAWN,B_KNIGHT,B_BISHOP,B_ROOK,B_QUEEN,B_KING
    };
  }

  void Position::init(){
    Prng rng(1070372);
    for (const Piece pc : pcs)
      for (Square s=SQ_A1; s<=SQ_H8; ++s)
        Zobrist::psq[pc][s]=rng.rand<uint64_t>();
    for (File f=FILE_A; f<=FILE_H; ++f)
      Zobrist::enpassant[f]=rng.rand<uint64_t>();
    for (int cr=NO_CASTLING; cr<=ANY_CASTLING; ++cr)
      Zobrist::castling[cr]=rng.rand<uint64_t>();
    Zobrist::side=rng.rand<uint64_t>();
    Zobrist::noPawns=rng.rand<uint64_t>();
    std::memset(cuckoo,0,sizeof(cuckoo));
    std::memset(cuckooMove,0,sizeof(cuckooMove));
    int count=0;
    for (const Piece pc : pcs)
      for (Square s1=SQ_A1; s1<=SQ_H8; ++s1)
        for (auto s2=static_cast<Square>(s1+1); s2<=SQ_H8; ++s2)
          if (typeOf(pc)!=PAWN&&attacksBb(typeOf(pc),s1,0)&s2){
            Move move=makeMove(s1,s2);
            uint64_t key=Zobrist::psq[pc][s1]^Zobrist::psq[pc][s2]^Zobrist::side;
            int i=h1(key);
            while (true){
              std::swap(cuckoo[i],key);
              std::swap(cuckooMove[i],move);
              if (move==MOVE_NONE)
                break;
              i=i==h1(key)?h2(key):h1(key);
            }
            count++;
          }
  }

  Position& Position::set(const string& fenStr, const bool isChess960, StateInfo* si, Thread* th){
    unsigned char row, token;
    size_t idx;
    Square sq=SQ_A8;
    std::istringstream ss(fenStr);
    std::memset(this,0,sizeof(Position));
    std::memset(si,0,sizeof(StateInfo));
    st=si;
    ss>>std::noskipws;
    while (ss>>token&&!isspace(token)){
      if (isdigit(token))
        sq+=(token-'0')*EAST;
      else if (token=='/')
        sq+=2*SOUTH;
      else{
        if (const char ch=static_cast<char>(token); (idx=pieceToChar.find(ch))!=string::npos){
          putPiece(static_cast<Piece>(idx),sq);
          ++sq;
        }
      }
    }
    ss>>token;
    sideToMove=token=='w'?WHITE:BLACK;
    ss>>token;
    while (ss>>token&&!isspace(token)){
      Square rsq;
      const Color c=islower(token)?BLACK:WHITE;
      const Piece rook=makePiece(c,ROOK);
      token=static_cast<char>(toupper(token));
      if (token=='K')
        for (rsq=relativeSquare(c,SQ_H1); pieceOn(rsq)!=rook; --rsq){}
      else if (token=='Q')
        for (rsq=relativeSquare(c,SQ_A1); pieceOn(rsq)!=rook; ++rsq){}
      else if (token>='A'&&token<='H')
        rsq=makeSquare(static_cast<File>(token-'A'),relativeRank(c,RANK_1));
      else
        continue;
      setCastlingRight(c,rsq);
    }
    bool enpassant=false;
    if (unsigned char col; ss>>col&&col>='a'&&col<='h'
      &&ss>>row&&row==static_cast<unsigned char>(sideToMove==WHITE?'6':'3')){
      st->epSquare=makeSquare(static_cast<File>(col-'a'),static_cast<Rank>(row-'1'));
      enpassant=pawnAttacksBb(~sideToMove,st->epSquare)&pieces(sideToMove,PAWN)
        &&(pieces(~sideToMove,PAWN)&(st->epSquare+pawnPush(~sideToMove)))
        &&!(pieces()&(st->epSquare|(st->epSquare+pawnPush(sideToMove))));
    }
    if (!enpassant)
      st->epSquare=SQ_NONE;
    ss>>std::skipws>>st->rule50>>gamePly;
    gamePly=std::max(2*(gamePly-1),0)+(sideToMove==BLACK);
    chess960=isChess960;
    thisThread=th;
    setState(st);
    return *this;
  }

  void Position::setCastlingRight(const Color c, const Square rfrom){
    const Square kfrom=square<KING>(c);
    const CastlingRights cr=c&(kfrom<rfrom?KING_SIDE:QUEEN_SIDE);
    st->castlingRights|=cr;
    castlingRightsMask[kfrom]|=cr;
    castlingRightsMask[rfrom]|=cr;
    castlingRookSquare[cr]=rfrom;
    const Square kto=relativeSquare(c,cr&KING_SIDE?SQ_G1:SQ_C1);
    const Square rto=relativeSquare(c,cr&KING_SIDE?SQ_F1:SQ_D1);
    castlingPath[cr]=(getBetweenBb(rfrom,rto)|getBetweenBb(kfrom,kto))
      &~(kfrom|rfrom);
  }

  void Position::setCheckInfo(StateInfo* si) const{
    si->blockersForKing[WHITE]=sliderBlockers(pieces(BLACK),square<KING>(WHITE),si->pinners[BLACK]);
    si->blockersForKing[BLACK]=sliderBlockers(pieces(WHITE),square<KING>(BLACK),si->pinners[WHITE]);
    const Square ksq=square<KING>(~sideToMove);
    si->checkSquares[PAWN]=pawnAttacksBb(~sideToMove,ksq);
    si->checkSquares[KNIGHT]=attacksBb<KNIGHT>(ksq);
    si->checkSquares[BISHOP]=attacksBb<BISHOP>(ksq,pieces());
    si->checkSquares[ROOK]=attacksBb<ROOK>(ksq,pieces());
    si->checkSquares[QUEEN]=si->checkSquares[BISHOP]|si->checkSquares[ROOK];
    si->checkSquares[KING]=0;
  }

  void Position::setState(StateInfo* si) const{
    si->pawnKey=Zobrist::noPawns;
    si->nonPawnMaterial[WHITE]=si->nonPawnMaterial[BLACK]=VALUE_ZERO;
    si->checkersBB=attackersTo(square<KING>(sideToMove))&pieces(~sideToMove);
    setCheckInfo(si);
    for (uint64_t b=pieces(); b;){
      const Square s=popLsb(b);
      const Piece pc=pieceOn(s);
      si->key^=Zobrist::psq[pc][s];
      if (typeOf(pc)==PAWN)
        si->pawnKey^=Zobrist::psq[pc][s];
      else if (typeOf(pc)!=KING)
        si->nonPawnMaterial[colorOf(pc)]+=pieceValue[MG][pc];
    }
    if (si->epSquare!=SQ_NONE)
      si->key^=Zobrist::enpassant[fileOf(si->epSquare)];
    if (sideToMove==BLACK)
      si->key^=Zobrist::side;
    si->key^=Zobrist::castling[si->castlingRights];
  }

  Position& Position::set(const string& code, const Color c, StateInfo* si){
    string sides[]={
      code.substr(code.find('K',1)),
      code.substr(0,std::min(code.find('v'),code.find('K',1)))
    };
    std::ranges::transform(sides[c],sides[c].begin(),
      [](const unsigned char ch) -> char{ return static_cast<char>(std::tolower(ch)); });

    const string fenStr="8/"+sides[0]+static_cast<char>(8-sides[0].length()+'0')+"/8/8/8/8/"
      +sides[1]+static_cast<char>(8-sides[1].length()+'0')+"/8 w - - 0 10";
    return set(fenStr,false,si,nullptr);
  }

  string Position::fen() const{
    int emptyCnt;
    std::ostringstream ss;
    for (Rank r=RANK_8; r>=RANK_1; --r){
      for (File f=FILE_A; f<=FILE_H; ++f){
        for (emptyCnt=0; f<=FILE_H&&empty(makeSquare(f,r)); ++f)
          ++emptyCnt;
        if (emptyCnt)
          ss<<emptyCnt;
        if (f<=FILE_H)
          ss<<pieceToChar[pieceOn(makeSquare(f,r))];
      }
      if (r>RANK_1)
        ss<<'/';
    }
    ss<<(sideToMove==WHITE?" w ":" b ");
    if (canCastle(WHITE_OO))
      ss<<(chess960?static_cast<char>('A'+fileOf(castleRookSquare(WHITE_OO))):'K');
    if (canCastle(WHITE_OOO))
      ss<<(chess960?static_cast<char>('A'+fileOf(castleRookSquare(WHITE_OOO))):'Q');
    if (canCastle(BLACK_OO))
      ss<<(chess960?static_cast<char>('a'+fileOf(castleRookSquare(BLACK_OO))):'k');
    if (canCastle(BLACK_OOO))
      ss<<(chess960?static_cast<char>('a'+fileOf(castleRookSquare(BLACK_OOO))):'q');
    if (!canCastle(ANY_CASTLING))
      ss<<'-';
    ss<<(epSquare()==SQ_NONE?" - ":" "+Uci::square(epSquare())+" ")
      <<st->rule50<<" "<<1+(gamePly-(sideToMove==BLACK))/2;
    return ss.str();
  }

  uint64_t Position::sliderBlockers(const uint64_t sliders, const Square s, uint64_t& pinners) const{
    uint64_t blockers=0;
    pinners=0;
    uint64_t snipers=((attacksBb<ROOK>(s)&pieces(QUEEN,ROOK))
      |(attacksBb<BISHOP>(s)&pieces(QUEEN,BISHOP)))&sliders;
    const uint64_t occupancy=pieces()^snipers;
    while (snipers){
      const Square sniperSq=popLsb(snipers);
      if (const uint64_t b=getBetweenBb(s,sniperSq)&occupancy; b&&!moreThanOne(b)){
        blockers|=b;
        if (b&pieces(colorOf(pieceOn(s))))
          pinners|=sniperSq;
      }
    }
    return blockers;
  }

  uint64_t Position::attackersTo(const Square s, const uint64_t occupied) const{
    return (pawnAttacksBb(BLACK,s)&pieces(WHITE,PAWN))
      |(pawnAttacksBb(WHITE,s)&pieces(BLACK,PAWN))
      |(attacksBb<KNIGHT>(s)&pieces(KNIGHT))
      |(attacksBb<ROOK>(s,occupied)&pieces(ROOK,QUEEN))
      |(attacksBb<BISHOP>(s,occupied)&pieces(BISHOP,QUEEN))
      |(attacksBb<KING>(s)&pieces(KING));
  }

  bool Position::legal(const Move m) const{
    const Color us=sideToMove;
    const Square from=fromSq(m);
    Square to=toSq(m);
    if (typeOf(m)==EN_PASSANT){
      const Square ksq=square<KING>(us);
      const Square capsq=to-pawnPush(us);
      const uint64_t occupied=(pieces()^from^capsq)|to;
      return !(attacksBb<ROOK>(ksq,occupied)&pieces(~us,QUEEN,ROOK))
        &&!(attacksBb<BISHOP>(ksq,occupied)&pieces(~us,QUEEN,BISHOP));
    }
    if (typeOf(m)==CASTLING){
      to=relativeSquare(us,to>from?SQ_G1:SQ_C1);
      const Direction step=to>from?WEST:EAST;
      for (Square s=to; s!=from; s+=step)
        if (attackersTo(s)&pieces(~us))
          return false;
      return !chess960||!(blockersForKing(us)&toSq(m));
    }
    if (typeOf(pieceOn(from))==KING)
      return !(attackersTo(to,pieces()^from)&pieces(~us));
    return !(blockersForKing(us)&from)
      ||aligned(from,to,square<KING>(us));
  }

  bool Position::pseudoLegal(const Move m) const{
    const Color us=sideToMove;
    const Square from=fromSq(m);
    const Square to=toSq(m);
    const Piece pc=movedPiece(m);
    if (typeOf(m)!=NORMAL)
      return checkers()
             ?MoveList<EVASIONS>(*this).contains(m)
             :MoveList<NON_EVASIONS>(*this).contains(m);
    if (promotionType(m)-KNIGHT!=NO_PIECE_TYPE)
      return false;
    if (pc==NO_PIECE||colorOf(pc)!=us)
      return false;
    if (pieces(us)&to)
      return false;
    if (typeOf(pc)==PAWN){
      if ((rank8Bb|rank1Bb)&to)
        return false;
      if (!(pawnAttacksBb(us,from)&pieces(~us)&to)
        &&!(from+pawnPush(us)==to&&empty(to))
        &&!(from+2*pawnPush(us)==to
          &&relativeRank(us,from)==RANK_2
          &&empty(to)
          &&empty(to-pawnPush(us))))
        return false;
    }
    else if (!(attacksBb(typeOf(pc),from,pieces())&to))
      return false;
    if (checkers()){
      if (typeOf(pc)!=KING){
        if (moreThanOne(checkers()))
          return false;
        if (!(getBetweenBb(square<KING>(us),lsb(checkers()))&to))
          return false;
      }
      else if (attackersTo(to,pieces()^from)&pieces(~us))
        return false;
    }
    return true;
  }

  bool Position::givesCheck(const Move m) const{
    const Square from=fromSq(m);
    const Square to=toSq(m);

    if (checkSquares(typeOf(pieceOn(from)))&to)
      return true;

    if (blockersForKing(~sideToMove)&from
      &&!aligned(from,to,square<KING>(~sideToMove)))
      return true;

    switch (typeOf(m)){
    case NORMAL:
      return false;

    case PROMOTION:
      return attacksBb(promotionType(m),to,pieces()^from)
        &square<KING>(~sideToMove);

    case EN_PASSANT:
      {
        const Square capsq=makeSquare(fileOf(to),rankOf(from));
        const uint64_t b=(pieces()^from^capsq)|to;

        return (attacksBb<ROOK>(square<KING>(~sideToMove),b)&
            pieces(sideToMove,QUEEN,ROOK))
          |(attacksBb<BISHOP>(square<KING>(~sideToMove),b)&
            pieces(sideToMove,QUEEN,BISHOP));
      }

    case CASTLING:
      {
        const Square ksq=square<KING>(~sideToMove);
        const Square rto=relativeSquare(sideToMove,
          to>from?SQ_F1:SQ_D1);

        return (attacksBb<ROOK>(rto)&ksq)
          &&(attacksBb<ROOK>(rto,pieces()^from^to)&ksq);
      }
    }

    UNREACHABLE();
  }

  void Position::doMove(const Move m, StateInfo& newSt, const bool givesCheck){
    thisThread->nodes.fetch_add(1,std::memory_order_relaxed);
    uint64_t k=st->key^Zobrist::side;
    std::memcpy(&newSt,st, offsetof(StateInfo,key));
    newSt.previous=st;
    st=&newSt;
    ++gamePly;
    ++st->rule50;
    ++st->pliesFromNull;
    st->accumulator.computed[WHITE]=false;
    st->accumulator.computed[BLACK]=false;
    auto& dp=st->dirtyPiece;
    dp.dirty_num=1;
    const Color us=sideToMove;
    const Color them=~us;
    const Square from=fromSq(m);
    Square to=toSq(m);
    const Piece pc=pieceOn(from);
    Piece captured=typeOf(m)==EN_PASSANT?makePiece(them,PAWN):pieceOn(to);
    if (typeOf(m)==CASTLING){
      Square rfrom, rto;
      doCastling<true>(us,from,to,rfrom,rto);
      k^=Zobrist::psq[captured][rfrom]^Zobrist::psq[captured][rto];
      captured=NO_PIECE;
    }
    if (captured){
      Square capsq=to;
      if (typeOf(captured)==PAWN){
        if (typeOf(m)==EN_PASSANT){ capsq-=pawnPush(us); }
        st->pawnKey^=Zobrist::psq[captured][capsq];
      }
      else
        st->nonPawnMaterial[them]-=pieceValue[MG][captured];

      dp.dirty_num=2;
      dp.piece[1]=captured;
      dp.from[1]=capsq;
      dp.to[1]=SQ_NONE;

      removePiece(capsq);
      if (typeOf(m)==EN_PASSANT)
        board[capsq]=NO_PIECE;
      k^=Zobrist::psq[captured][capsq];
      st->rule50=0;
    }
    k^=Zobrist::psq[pc][from]^Zobrist::psq[pc][to];
    if (st->epSquare!=SQ_NONE){
      k^=Zobrist::enpassant[fileOf(st->epSquare)];
      st->epSquare=SQ_NONE;
    }
    if (st->castlingRights&&castlingRightsMask[from]|castlingRightsMask[to]){
      k^=Zobrist::castling[st->castlingRights];
      st->castlingRights&=~(castlingRightsMask[from]|castlingRightsMask[to]);
      k^=Zobrist::castling[st->castlingRights];
    }
    if (typeOf(m)!=CASTLING){
      dp.piece[0]=pc;
      dp.from[0]=from;
      dp.to[0]=to;

      movePiece(from,to);
    }
    if (typeOf(pc)==PAWN){
      if ((static_cast<int>(to)^static_cast<int>(from))==16
        &&pawnAttacksBb(us,to-pawnPush(us))&pieces(them,PAWN)){
        st->epSquare=to-pawnPush(us);
        k^=Zobrist::enpassant[fileOf(st->epSquare)];
      }
      else if (typeOf(m)==PROMOTION){
        const Piece promotion=makePiece(us,promotionType(m));
        removePiece(to);
        putPiece(promotion,to);

        dp.to[0]=SQ_NONE;
        dp.piece[dp.dirty_num]=promotion;
        dp.from[dp.dirty_num]=SQ_NONE;
        dp.to[dp.dirty_num]=to;
        dp.dirty_num++;

        k^=Zobrist::psq[pc][to]^Zobrist::psq[promotion][to];
        st->pawnKey^=Zobrist::psq[pc][to];
        st->nonPawnMaterial[us]+=pieceValue[MG][promotion];
      }
      st->pawnKey^=Zobrist::psq[pc][from]^Zobrist::psq[pc][to];
      st->rule50=0;
    }
    st->capturedPiece=captured;
    st->key=k;
    st->checkersBB=givesCheck?attackersTo(square<KING>(them))&pieces(us):0;
    sideToMove=~sideToMove;
    setCheckInfo(st);
    st->repetition=0;
    if (const int end=std::min(st->rule50,st->pliesFromNull); end>=4){
      const StateInfo* stp=st->previous->previous;
      for (int i=4; i<=end; i+=2){
        stp=stp->previous->previous;
        if (stp->key==st->key){
          st->repetition=stp->repetition?-i:i;
          break;
        }
      }
    }
  }

  void Position::undoMove(const Move m){
    sideToMove=~sideToMove;
    const Color us=sideToMove;
    const Square from=fromSq(m);
    Square to=toSq(m);
    if (typeOf(m)==PROMOTION){
      removePiece(to);
      const Piece pc=makePiece(us,PAWN);
      putPiece(pc,to);
    }
    if (typeOf(m)==CASTLING){
      Square rfrom, rto;
      doCastling<false>(us,from,to,rfrom,rto);
    }
    else{
      movePiece(to,from);
      if (st->capturedPiece){
        Square capsq=to;
        if (typeOf(m)==EN_PASSANT){ capsq-=pawnPush(us); }
        putPiece(st->capturedPiece,capsq);
      }
    }
    st=st->previous;
    --gamePly;
  }

  template <bool Do>
  void Position::doCastling(const Color us, const Square from, Square& to, Square& rfrom, Square& rto){
    const bool kingside=to>from;
    rfrom=to;
    rto=relativeSquare(us,kingside?SQ_F1:SQ_D1);
    to=relativeSquare(us,kingside?SQ_G1:SQ_C1);
    if (Do){
      auto& dp=st->dirtyPiece;
      dp.piece[0]=makePiece(us,KING);
      dp.from[0]=from;
      dp.to[0]=to;
      dp.piece[1]=makePiece(us,ROOK);
      dp.from[1]=rfrom;
      dp.to[1]=rto;
      dp.dirty_num=2;
    }
    removePiece(Do?from:to);
    removePiece(Do?rfrom:rto);
    board[Do?from:to]=board[Do?rfrom:rto]=NO_PIECE;
    putPiece(makePiece(us,KING),Do?to:from);
    putPiece(makePiece(us,ROOK),Do?rto:rfrom);
  }

  void Position::doNullMove(StateInfo& newSt){
    std::memcpy(&newSt,st, offsetof(StateInfo,accumulator));
    newSt.previous=st;
    st=&newSt;
    st->dirtyPiece.dirty_num=0;
    st->dirtyPiece.piece[0]=NO_PIECE;
    st->accumulator.computed[WHITE]=false;
    st->accumulator.computed[BLACK]=false;
    if (st->epSquare!=SQ_NONE){
      st->key^=Zobrist::enpassant[fileOf(st->epSquare)];
      st->epSquare=SQ_NONE;
    }
    st->key^=Zobrist::side;
    ++st->rule50;
    prefetch(tt.firstEntry(key()));
    st->pliesFromNull=0;
    sideToMove=~sideToMove;
    setCheckInfo(st);
    st->repetition=0;
  }

  void Position::undoNullMove(){
    st=st->previous;
    sideToMove=~sideToMove;
  }

  uint64_t Position::keyAfter(const Move m) const{
    const Square from=fromSq(m);
    const Square to=toSq(m);
    const Piece pc=pieceOn(from);
    const Piece captured=pieceOn(to);
    uint64_t k=st->key^Zobrist::side;
    if (captured)
      k^=Zobrist::psq[captured][to];
    k^=Zobrist::psq[pc][to]^Zobrist::psq[pc][from];
    return captured||typeOf(pc)==PAWN
           ?k
           :adjustKey50<true>(k);
  }

  bool Position::seeGe(const Move m, const Value threshold) const{
    if (typeOf(m)!=NORMAL)
      return VALUE_ZERO>=threshold;
    const Square from=fromSq(m), to=toSq(m);
    int swap=pieceValue[MG][pieceOn(to)]-threshold;
    if (swap<0)
      return false;
    swap=pieceValue[MG][pieceOn(from)]-swap;
    if (swap<=0)
      return true;
    uint64_t occupied=pieces()^from^to;
    Color stm=sideToMove;
    uint64_t attackers=attackersTo(to,occupied);
    uint64_t bb;
    int res=1;
    while (true){
      stm=~stm;
      attackers&=occupied;
      uint64_t stmAttackers=attackers&pieces(stm);
      if (!stmAttackers)
        break;
      if (pinners(~stm)&occupied){
        stmAttackers&=~blockersForKing(stm);
        if (!stmAttackers)
          break;
      }
      res^=1;
      if ((bb=stmAttackers&pieces(PAWN))){
        if ((swap=PawnValueMg-swap)<res)
          break;
        occupied^=leastSignificantSquareBb(bb);
        attackers|=attacksBb<BISHOP>(to,occupied)&pieces(BISHOP,QUEEN);
      }
      else if ((bb=stmAttackers&pieces(KNIGHT))){
        if ((swap=KnightValueMg-swap)<res)
          break;
        occupied^=leastSignificantSquareBb(bb);
      }
      else if ((bb=stmAttackers&pieces(BISHOP))){
        if ((swap=BishopValueMg-swap)<res)
          break;
        occupied^=leastSignificantSquareBb(bb);
        attackers|=attacksBb<BISHOP>(to,occupied)&pieces(BISHOP,QUEEN);
      }
      else if ((bb=stmAttackers&pieces(ROOK))){
        if ((swap=RookValueMg-swap)<res)
          break;
        occupied^=leastSignificantSquareBb(bb);
        attackers|=attacksBb<ROOK>(to,occupied)&pieces(ROOK,QUEEN);
      }
      else if ((bb=stmAttackers&pieces(QUEEN))){
        if ((swap=QueenValueMg-swap)<res)
          break;
        occupied^=leastSignificantSquareBb(bb);
        attackers|=(attacksBb<BISHOP>(to,occupied)&pieces(BISHOP,QUEEN))
          |(attacksBb<ROOK>(to,occupied)&pieces(ROOK,QUEEN));
      }
      else
        return attackers&~pieces(stm)?res^1:res;
    }
    return static_cast<bool>(res);
  }

  bool Position::isDraw(const int ply) const{
    if (st->rule50>99&&(!checkers()||MoveList<LEGAL>(*this).size()))
      return true;
    return st->repetition&&st->repetition<ply;
  }

  bool Position::hasRepeated() const{
    const StateInfo* stc=st;
    int end=std::min(st->rule50,st->pliesFromNull);
    while (end-->=4){
      if (stc->repetition)
        return true;
      stc=stc->previous;
    }
    return false;
  }

  bool Position::hasGameCycle(const int ply) const{
    const int end=std::min(st->rule50,st->pliesFromNull);
    if (end<3)
      return false;

    const uint64_t originalKey=st->key;
    const StateInfo* stp=st->previous;

    for (int i=3; i<=end; i+=2){
      stp=stp->previous->previous;

      const uint64_t moveKey=originalKey^stp->key;

      int j=h1(moveKey);
      bool found=(cuckoo[j]==moveKey);

      if (!found){
        j=h2(moveKey);
        found=(cuckoo[j]==moveKey);
      }

      if (!found)
        continue;

      const Move move=cuckooMove[j];
      Square s1=fromSq(move);
      Square s2=toSq(move);

      if (((getBetweenBb(s1,s2)^s2)&pieces())!=0)
        continue;

      if (ply>i)
        return true;

      if (const Square sq=empty(s1)?s2:s1; colorOf(pieceOn(sq))!=stm())
        continue;

      if (stp->repetition)
        return true;
    }

    return false;
  }
}
