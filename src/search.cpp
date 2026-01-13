#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include "evaluate.h"
#include "misc.h"
#include "movegen.h"
#include "movepick.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "timeman.h"
#include "tt.h"
#include "uci.h"

namespace Nebula{
  namespace Search{
    LimitsType limits;
  }

  using std::string;
  using Eval::evaluate;
  using namespace Search;

  namespace{
    enum NodeType :uint8_t{ NonPV, PV, Root };

    Value futilityMargin(const Depth d, const bool improving){ return static_cast<Value>(168*(d-improving)); }
    int reductions[maxMoves];

    Depth reduction(const bool i, const Depth d, const int mn, const Value delta, const Value rootDelta){
      const int r=reductions[d]*reductions[mn];
      return (r+1463-static_cast<int>(delta)*1024/static_cast<int>(rootDelta))/1024+(!i&&r>1010);
    }

    constexpr int futilityMoveCount(const bool improving, const Depth depth){
      return improving
             ?3+depth*depth
             :(3+depth*depth)/2;
    }

    int statBonus(const Depth d){ return std::min((8*d+240)*d-276,1907); }
    Value valueDraw(const Thread* thisThread){ return VALUE_DRAW-1+static_cast<Value>(thisThread->nodes&0x2); }
    template <NodeType nodeType>
    Value search(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode);
    template <NodeType nodeType>
    Value qsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth=0);
    Value valueToTt(Value v, int ply);
    Value valueFromTt(Value v, int ply, int r50C);
    void updatePv(Move* pv, Move move, const Move* childPv);
    void updateContinuationHistories(const Stack* ss, Piece pc, Square to, int bonus);
    void updateQuietStats(const Position& pos, Stack* ss, Move move, int bonus);
    void updateAllStats(const Position& pos, Stack* ss, Move bestMove, Value bestValue, Value beta, Square prevSq,
      const Move* quietsSearched, int quietCount, const Move* capturesSearched, int captureCount,
      Depth depth);
  }

  void Search::init(){
    for (int i=1; i<maxMoves; ++i)
      reductions[i]=static_cast<int>((20.81+std::log(threads.size())/2)*std::log(i));
  }

  void Search::clear(){
    threads.main()->waitForSearchFinished();
    time.availableNodes=0;
    tt.clear();
    threads.clear();
  }

  void MainThread::search(){
    const Color us=rootPos.stm();
    time.init(limits,us,rootPos.gameply());
    tt.newSearch();
    if (rootMoves.empty()){ rootMoves.emplace_back(MOVE_NONE); }
    else{
      threads.startSearching();
      Thread::search();
    }
    while (!threads.stop&&(ponder||limits.infinite)){}
    threads.stop=true;
    threads.waitForSearchFinished();
    if (limits.npmsec)
      time.availableNodes+=static_cast<int64_t>(limits.inc[us]-threads.nodesSearched());
    Thread* bestThread=this;
    if (options["MultiPV"].asInt()==1
      &&!limits.depth
      &&rootMoves[0].pv[0]!=MOVE_NONE)
      bestThread=threads.getBestThread();
    bestPreviousScore=bestThread->rootMoves[0].score;
    bestPreviousAverageScore=bestThread->rootMoves[0].averageScore;
    for (Thread* th : threads)
      th->previousDepth=bestThread->completedDepth;
    if (bestThread!=this)
      async()<<Uci::pv(bestThread->rootPos,bestThread->completedDepth)<<std::endl;
    async()<<"bestmove "<<Uci::move(bestThread->rootMoves[0].pv[0],rootPos.isChess960());
    if (bestThread->rootMoves[0].pv.size()>1||bestThread->rootMoves[0].extractPonderFromTt(rootPos))
      async()<<" ponder "<<Uci::move(bestThread->rootMoves[0].pv[1],rootPos.isChess960());
    async()<<std::endl;
  }

  void Thread::search(){
    Stack stack[maxPly+10], *ss=stack+7;
    Move pv[maxPly+1];
    Value alpha, delta;
    Move lastBestMove=MOVE_NONE;
    Depth lastBestMoveDepth=0;
    MainThread* mainThread=this==threads.main()?threads.main():nullptr;
    double timeReduction=1, totBestMoveChanges=0;
    const Color us=rootPos.stm();
    int iterIdx=0;
    std::memset(ss-7,0,10*sizeof(Stack));
    for (int i=7; i>0; i--)
      (ss-i)->continuationHistory=&this->continuationHistory[0][0][NO_PIECE][0];
    for (int i=0; i<=maxPly+2; ++i)
      (ss+i)->ply=i;
    ss->pv=pv;
    bestValue=delta=alpha=-VALUE_INFINITE;
    Value beta=VALUE_INFINITE;
    if (mainThread){
      if (mainThread->bestPreviousScore==VALUE_INFINITE)
        for (auto& i : mainThread->iterValue)
          i=VALUE_ZERO;
      else
        for (auto& i : mainThread->iterValue)
          i=mainThread->bestPreviousScore;
    }
    size_t multiPv=std::max<size_t>(1,options["MultiPV"].asSize());
    multiPv=std::min(multiPv,rootMoves.size());
    complexityAverage.set(174,1);
    trend=SCORE_ZERO;
    optimism[us]=static_cast<Value>(39);
    optimism[~us]=-optimism[us];
    int searchAgainCounter=0;
    while (true) {
      ++rootDepth;
      if (rootDepth >= maxPly)
        break;
      if (threads.stop)
        break;
      if (limits.depth && mainThread && rootDepth > limits.depth)
        break;
      if (mainThread)
        totBestMoveChanges/=2;
      for (RootMove& rm : rootMoves)
        rm.previousScore=rm.score;
      size_t pvFirst=0;
      pvLast=0;
      if (!threads.increaseDepth)
        searchAgainCounter++;
      for (pvIdx=0; pvIdx<multiPv&&!threads.stop; ++pvIdx){
        if (pvIdx==pvLast){
          pvFirst=pvLast;
          for (pvLast++; pvLast<rootMoves.size(); pvLast++)
            if (rootMoves[pvLast].tbRank!=rootMoves[pvFirst].tbRank)
              break;
        }
        if (rootDepth>=4){
          const Value prev=rootMoves[pvIdx].averageScore;
          delta=static_cast<Value>(16)+static_cast<int>(prev)*prev/19178;
          alpha=std::max(prev-delta,-VALUE_INFINITE);
          beta=std::min(prev+delta,VALUE_INFINITE);
          const int tr=static_cast<int>(sigmoid(prev,3,8,90,125,1));
          trend=us==WHITE
                ?calcScore(tr,tr/2)
                :-calcScore(tr,tr/2);
          int opt=static_cast<int>(sigmoid(prev,8,17,144,13966,183));
          optimism[us]=static_cast<Value>(opt);
          optimism[~us]=-optimism[us];
        }
        int failedHighCnt=0;
        while (true){
          const Depth adjustedDepth=std::max(1,rootDepth-failedHighCnt-3*(searchAgainCounter+1)/4);
          bestValue=Nebula::search<Root>(rootPos,ss,alpha,beta,adjustedDepth,false);
          const auto it1=rootMoves.begin()+static_cast<std::ptrdiff_t>(pvIdx);
          const auto it2=rootMoves.begin()+static_cast<std::ptrdiff_t>(pvLast);
          std::stable_sort(it1,it2);

          if (threads.stop)
            break;
          if (mainThread
            &&multiPv==1
            &&(bestValue<=alpha||bestValue>=beta)
            &&time.elapsed()>3000)
            async()<<Uci::pv(rootPos,rootDepth)<<std::endl;
          if (bestValue<=alpha){
            beta=(alpha+beta)/2;
            alpha=std::max(bestValue-delta,-VALUE_INFINITE);
            failedHighCnt=0;
            if (mainThread)
              mainThread->stopOnPonderhit=false;
          }
          else if (bestValue>=beta){
            beta=std::min(bestValue+delta,VALUE_INFINITE);
            ++failedHighCnt;
          }
          else
            break;
          delta+=delta/4+2;
        }
        const auto it3=rootMoves.begin()+static_cast<std::ptrdiff_t>(pvFirst);
        const auto it4=rootMoves.begin()+static_cast<std::ptrdiff_t>(pvIdx+1);
        std::stable_sort(it3,it4);

        if (mainThread
          &&(threads.stop||pvIdx+1==multiPv||time.elapsed()>3000))
          async()<<Uci::pv(rootPos,rootDepth)<<std::endl;
      }
      if (!threads.stop)
        completedDepth=rootDepth;
      if (rootMoves[0].pv[0]!=lastBestMove){
        lastBestMove=rootMoves[0].pv[0];
        lastBestMoveDepth=rootDepth;
      }
      if (limits.mate
        &&bestValue>=VALUE_MATE_IN_MAX_PLY
        &&VALUE_MATE-bestValue<=2*limits.mate)
        threads.stop=true;
      if (!mainThread)
        continue;
      for (Thread* th : threads){
        totBestMoveChanges+=static_cast<double>(th->bestMoveChanges);
        th->bestMoveChanges=0;
      }
      if (limits.useTimeManagement()
        &&!threads.stop
        &&!mainThread->stopOnPonderhit){
        double fallingEval=(69+12*(mainThread->bestPreviousAverageScore-bestValue)
          +6*(mainThread->iterValue[iterIdx]-bestValue))/781.4;
        fallingEval=std::clamp(fallingEval,0.5,1.5);
        timeReduction=lastBestMoveDepth+10<completedDepth?1.63:0.73;
        const double reduction=(1.56+mainThread->previousTimeReduction)/(2.20*timeReduction);
        const auto bestMoveInstability=1+1.7*totBestMoveChanges/static_cast<double>(threads.size());
        const int complexity=static_cast<int>(mainThread->complexityAverage.value());
        const double complexPosition=std::min(1.0+(complexity-277)/1819.1,1.5);
        auto totalTime=static_cast<double>(time.optimum())*fallingEval*reduction*bestMoveInstability*
          complexPosition;
        if (rootMoves.size()==1)
          totalTime=std::min(500.0,totalTime);
        if (static_cast<double>(time.elapsed())>totalTime){
          if (mainThread->ponder)
            mainThread->stopOnPonderhit=true;
          else
            threads.stop=true;
        }
        else if (threads.increaseDepth
          &&!mainThread->ponder
          &&static_cast<double>(time.elapsed())>totalTime*0.43)
          threads.increaseDepth=false;
        else
          threads.increaseDepth=true;
      }
      mainThread->iterValue[iterIdx]=bestValue;
      iterIdx=(iterIdx+1)&3;
    }
    if (!mainThread)
      return;
    mainThread->previousTimeReduction=timeReduction;
  }

  namespace{
    template <NodeType nodeType>
    Value search(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode){
      constexpr bool pvNode=nodeType!=NonPV;
      constexpr bool rootNode=nodeType==Root;
      const Depth maxNextDepth=rootNode?depth:depth+1;
      if (!rootNode
        &&pos.rule50Count()>=3
        &&alpha<VALUE_DRAW
        &&pos.hasGameCycle(ss->ply)){
        alpha=valueDraw(pos.thisthread());
        if (alpha>=beta)
          return alpha;
      }
      if (depth<=0)
        return qsearch<pvNode?PV:NonPV>(pos,ss,alpha,beta);
      Move pv[maxPly+1], capturesSearched[32], quietsSearched[64];
      StateInfo st;
      TtEntry* tte;
      uint64_t posKey;
      Move ttMove, move, excludedMove, bestMove;
      Depth extension, newDepth;
      Value bestValue, value, ttValue, eval, maxValue, probCutBeta;
      bool givesCheck, improving, priorCapture, singularQuietLmr;
      bool capture, moveCountPruning, ttCapture;
      Piece movedPiece;
      int moveCount, captureCount, quietCount, improvement, complexity;
      Thread* thisThread=pos.thisthread();
      ss->inCheck=pos.checkers();
      priorCapture=pos.capturedPiece();
      Color us=pos.stm();
      moveCount=captureCount=quietCount=ss->moveCount=0;
      bestValue=-VALUE_INFINITE;
      maxValue=VALUE_INFINITE;
      if (thisThread==threads.main())
        dynamic_cast<MainThread*>(thisThread)->checkTime();
      if (!rootNode){
        if (threads.stop.load(std::memory_order_relaxed)
          ||pos.isDraw(ss->ply)
          ||ss->ply>=maxPly)
          return ss->ply>=maxPly&&!ss->inCheck
                 ?evaluate(pos)
                 :valueDraw(pos.thisthread());
        alpha=std::max(matedIn(ss->ply),alpha);
        beta=std::min(mateIn(ss->ply+1),beta);
        if (alpha>=beta)
          return alpha;
      }
      else
        thisThread->rootDelta=beta-alpha;
      (ss+1)->ttPv=false;
      (ss+1)->excludedMove=bestMove=MOVE_NONE;
      (ss+2)->killers[0]=(ss+2)->killers[1]=MOVE_NONE;
      (ss+2)->cutoffCnt=0;
      ss->doubleExtensions=(ss-1)->doubleExtensions;
      Square prevSq=toSq((ss-1)->currentMove);
      if (!rootNode)
        (ss+2)->statScore=0;
      excludedMove=ss->excludedMove;
      posKey=excludedMove==MOVE_NONE?pos.key():pos.key()^makeKey(excludedMove);
      tte=tt.probe(posKey,ss->ttHit);
      ttValue=ss->ttHit?valueFromTt(tte->value(),ss->ply,pos.rule50Count()):VALUE_NONE;
      ttMove=rootNode
             ?thisThread->rootMoves[thisThread->pvIdx].pv[0]
             :ss->ttHit
             ?tte->move()
             :MOVE_NONE;
      ttCapture=ttMove&&pos.capture(ttMove);
      if (!excludedMove)
        ss->ttPv=pvNode||(ss->ttHit&&tte->isPv());
      if (!pvNode
        &&ss->ttHit
        &&tte->depth()>depth-(tte->bound()==BOUND_EXACT)
        &&ttValue!=VALUE_NONE
        &&tte->bound()&(ttValue>=beta?BOUND_LOWER:BOUND_UPPER)){
        if (ttMove){
          if (ttValue>=beta){
            if (!ttCapture)
              updateQuietStats(pos,ss,ttMove,statBonus(depth));
            if ((ss-1)->moveCount<=2&&!priorCapture)
              updateContinuationHistories(ss-1,pos.pieceOn(prevSq),prevSq,-statBonus(depth+1));
          }
          else if (!ttCapture){
            int penalty=-statBonus(depth);
            thisThread->mainHistory[us][fromTo(ttMove)]<<penalty;
            updateContinuationHistories(ss,pos.movedPiece(ttMove),toSq(ttMove),penalty);
          }
        }
        if (pos.rule50Count()<90)
          return ttValue;
      }
      CapturePieceToHistory& captureHistory=thisThread->captureHistory;
      if (ss->inCheck){
        ss->staticEval=eval=VALUE_NONE;
        improving=false;
        improvement=0;
        complexity=0;
        goto moves_loop;
      }
      if (ss->ttHit){
        ss->staticEval=eval=tte->eval();
        if (eval==VALUE_NONE)
          ss->staticEval=eval=evaluate(pos,&complexity);
        else
          complexity=abs(ss->staticEval);
        if (ttValue!=VALUE_NONE
          &&tte->bound()&(ttValue>eval?BOUND_LOWER:BOUND_UPPER))
          eval=ttValue;
      }
      else{
        ss->staticEval=eval=evaluate(pos,&complexity);
        if (!excludedMove)
          tte->save(posKey,VALUE_NONE,ss->ttPv,BOUND_NONE,DEPTH_NONE,MOVE_NONE,eval);
      }
      thisThread->complexityAverage.update(complexity);
      if (isOk((ss-1)->currentMove)&&!(ss-1)->inCheck&&!priorCapture){
        int bonus=std::clamp(-16*static_cast<int>((ss-1)->staticEval+ss->staticEval),-2000,2000);
        thisThread->mainHistory[~us][fromTo((ss-1)->currentMove)]<<bonus;
      }
      improvement=(ss-2)->staticEval!=VALUE_NONE
                  ?ss->staticEval-(ss-2)->staticEval
                  :(ss-4)->staticEval!=VALUE_NONE
                  ?ss->staticEval-(ss-4)->staticEval
                  :175;
      improving=improvement>0;
      if (depth<=7
        &&eval<alpha-348-258*depth*depth){
        value=qsearch<NonPV>(pos,ss,alpha-1,alpha);
        if (value<alpha)
          return value;
      }
      if (!ss->ttPv
        &&depth<8
        &&eval-futilityMargin(depth,improving)-(ss-1)->statScore/256>=beta
        &&eval>=beta
        &&eval<26305)
        return eval;
      if (!pvNode
        &&(ss-1)->currentMove!=MOVE_NULL
        &&(ss-1)->statScore<14695
        &&eval>=beta
        &&eval>=ss->staticEval
        &&ss->staticEval>=beta-15*depth-improvement/15+201+complexity/24
        &&!excludedMove
        &&pos.nonPawnMaterial(us)
        &&(ss->ply>=thisThread->nmpMinPly||us!=thisThread->nmpColor)){
        Depth R=std::min(static_cast<int>(eval-beta)/147,5)+depth/3+4-(complexity>650);
        ss->currentMove=MOVE_NULL;
        ss->continuationHistory=&thisThread->continuationHistory[0][0][NO_PIECE][0];
        pos.doNullMove(st);
        Value nullValue=-search<NonPV>(pos,ss+1,-beta,-beta+1,depth-R,!cutNode);
        pos.undoNullMove();
        if (nullValue>=beta){
          if (nullValue>=VALUE_TB_WIN_IN_MAX_PLY)
            nullValue=beta;
          if (thisThread->nmpMinPly||(abs(beta)<VALUE_KNOWN_WIN&&depth<14))
            return nullValue;
          thisThread->nmpMinPly=ss->ply+3*(depth-R)/4;
          thisThread->nmpColor=us;
          Value v=search<NonPV>(pos,ss,beta-1,beta,depth-R,false);
          thisThread->nmpMinPly=0;
          if (v>=beta)
            return nullValue;
        }
      }
      probCutBeta=beta+179-46*improving;
      if (!pvNode
        &&depth>4
        &&abs(beta)<VALUE_TB_WIN_IN_MAX_PLY
        &&!(ss->ttHit
          &&tte->depth()>=depth-3
          &&ttValue!=VALUE_NONE
          &&ttValue<probCutBeta)){
        MovePicker mp(pos,ttMove,probCutBeta-ss->staticEval,depth-3,&captureHistory);
        while ((move=mp.nextMove())!=MOVE_NONE)
          if (move!=excludedMove&&pos.legal(move)){
            ss->currentMove=move;
            ss->continuationHistory=&thisThread->continuationHistory[ss->inCheck]
              [true]
              [pos.movedPiece(move)]
              [toSq(move)];
            pos.doMove(move,st);
            value=-qsearch<NonPV>(pos,ss+1,-probCutBeta,-probCutBeta+1);
            if (value>=probCutBeta)
              value=-search<NonPV>(pos,ss+1,-probCutBeta,-probCutBeta+1,depth-4,!cutNode);
            pos.undoMove(move);
            if (value>=probCutBeta){
              tte->save(posKey,valueToTt(value,ss->ply),ss->ttPv,BOUND_LOWER,depth-3,move,ss->staticEval);
              return value;
            }
          }
      }
      if (pvNode
        &&!ttMove)
        depth-=3;
      if (depth<=0)
        return qsearch<PV>(pos,ss,alpha,beta);
      if (cutNode
        &&depth>=8
        &&!ttMove)
        depth--;
    moves_loop:
      probCutBeta=beta+481;
      if (ss->inCheck
        &&!pvNode
        &&depth>=2
        &&ttCapture
        &&tte->bound()&BOUND_LOWER
        &&tte->depth()>=depth-3
        &&ttValue>=probCutBeta
        &&abs(ttValue)<=VALUE_KNOWN_WIN
        &&abs(beta)<=VALUE_KNOWN_WIN
      )
        return probCutBeta;
      const PieceToHistory* contHist[]={
        (ss-1)->continuationHistory,(ss-2)->continuationHistory,
        nullptr,(ss-4)->continuationHistory,nullptr,(ss-6)->continuationHistory
      };
      Move countermove=thisThread->counterMoves[pos.pieceOn(prevSq)][prevSq];
      MovePicker mp(pos,ttMove,depth,&thisThread->mainHistory,
        &captureHistory,
        contHist,
        countermove,
        ss->killers);
      value=bestValue;
      moveCountPruning=singularQuietLmr=false;
      bool likelyFailLow=pvNode
        &&ttMove
        &&tte->bound()&BOUND_UPPER
        &&tte->depth()>=depth;
      while ((move=mp.nextMove(moveCountPruning))!=MOVE_NONE){
        if (move==excludedMove)
          continue;
        if (rootNode){
          const auto it1=thisThread->rootMoves.begin()
            +static_cast<std::ptrdiff_t>(thisThread->pvIdx);
          const auto it2=thisThread->rootMoves.begin()
            +static_cast<std::ptrdiff_t>(thisThread->pvLast);

          if (!std::count(it1,it2,move))
            continue;
        }

        if (!rootNode&&!pos.legal(move))
          continue;
        ss->moveCount=++moveCount;
        if (pvNode)
          (ss+1)->pv=nullptr;
        extension=0;
        capture=pos.capture(move);
        movedPiece=pos.movedPiece(move);
        givesCheck=pos.givesCheck(move);
        newDepth=depth-1;
        Value delta=beta-alpha;
        if (!rootNode
          &&pos.nonPawnMaterial(us)
          &&bestValue>VALUE_TB_LOSS_IN_MAX_PLY){
          moveCountPruning=moveCount>=futilityMoveCount(improving,depth);
          int lmrDepth=std::max(newDepth-reduction(improving,depth,moveCount,delta,thisThread->rootDelta),0);
          if (capture
            ||givesCheck){
            if (!pos.empty(toSq(move))
              &&!givesCheck
              &&!pvNode
              &&lmrDepth<6
              &&!ss->inCheck
              &&ss->staticEval+281+179*lmrDepth+pieceValue[EG][pos.pieceOn(toSq(move))]
              +captureHistory[movedPiece][toSq(move)][typeOf(pos.pieceOn(toSq(move)))]/6<alpha)
              continue;
            if (!pos.seeGe(move,static_cast<Value>(-203)*depth))
              continue;
          }
          else{
            int history=(*contHist[0])[movedPiece][toSq(move)]
              +(*contHist[1])[movedPiece][toSq(move)]
              +(*contHist[3])[movedPiece][toSq(move)];
            if (lmrDepth<5
              &&history<-3875*(depth-1))
              continue;
            history+=2*thisThread->mainHistory[us][fromTo(move)];
            if (!ss->inCheck
              &&lmrDepth<11
              &&ss->staticEval+122+138*lmrDepth+history/60<=alpha)
              continue;
            if (!pos.seeGe(move,static_cast<Value>(-25*lmrDepth*lmrDepth-20*lmrDepth)))
              continue;
          }
        }
        if (ss->ply<thisThread->rootDepth*2){
          if (!rootNode
            &&depth>=4-(thisThread->previousDepth>27)+2*(pvNode&&tte->isPv())
            &&move==ttMove
            &&!excludedMove
            &&abs(ttValue)<VALUE_KNOWN_WIN
            &&tte->bound()&BOUND_LOWER
            &&tte->depth()>=depth-3){
            Value singularBeta=ttValue-3*depth;
            Depth singularDepth=(depth-1)/2;
            ss->excludedMove=move;
            value=search<NonPV>(pos,ss,singularBeta-1,singularBeta,singularDepth,cutNode);
            ss->excludedMove=MOVE_NONE;
            if (value<singularBeta){
              extension=1;
              singularQuietLmr=!ttCapture;
              if (!pvNode
                &&value<singularBeta-26
                &&ss->doubleExtensions<=8)
                extension=2;
            }
            else if (singularBeta>=beta)
              return singularBeta;
            else if (ttValue>=beta)
              extension=-2;
            else if (ttValue<=alpha&&ttValue<=value)
              extension=-1;
          }
          else if (givesCheck
            &&depth>9
            &&abs(ss->staticEval)>71)
            extension=1;
          else if (pvNode
            &&move==ttMove
            &&move==ss->killers[0]
            &&(*contHist[0])[movedPiece][toSq(move)]>=5491)
            extension=1;
        }
        newDepth+=extension;
        ss->doubleExtensions=(ss-1)->doubleExtensions+(extension==2);
        prefetch(tt.firstEntry(pos.keyAfter(move)));
        ss->currentMove=move;
        ss->continuationHistory=&thisThread->continuationHistory[ss->inCheck]
          [capture]
          [movedPiece]
          [toSq(move)];
        pos.doMove(move,st,givesCheck);
        if (depth>=2
          &&moveCount>1+(pvNode&&ss->ply<=1)
          &&(!ss->ttPv
            ||!capture
            ||(cutNode&&(ss-1)->moveCount>1))){
          Depth r=reduction(improving,depth,moveCount,delta,thisThread->rootDelta);
          if (ss->ttPv
            &&!likelyFailLow)
            r-=2;
          if ((ss-1)->moveCount>7)
            r--;
          if (cutNode)
            r+=2;
          if (ttCapture)
            r++;
          if (pvNode)
            r-=1+15/(3+depth);
          if (singularQuietLmr)
            r--;
          if ((ss+1)->cutoffCnt>3&&!pvNode)
            r++;
          ss->statScore=2*thisThread->mainHistory[us][fromTo(move)]
            +(*contHist[0])[movedPiece][toSq(move)]
            +(*contHist[1])[movedPiece][toSq(move)]
            +(*contHist[3])[movedPiece][toSq(move)]
            -4334;
          r-=ss->statScore/15914;
          Depth d=std::clamp(newDepth-r,1,newDepth+1);
          value=-search<NonPV>(pos,ss+1,-(alpha+1),-alpha,d,true);
          if (value>alpha&&d<newDepth){
            const bool doDeeperSearch=value>alpha+78+11*(newDepth-d);
            value=-search<NonPV>(pos,ss+1,-(alpha+1),-alpha,newDepth+doDeeperSearch,!cutNode);
            int bonus=value>alpha
                      ?statBonus(newDepth)
                      :-statBonus(newDepth);
            if (capture)
              bonus/=6;
            updateContinuationHistories(ss,movedPiece,toSq(move),bonus);
          }
        }
        else if (!pvNode||moveCount>1){ value=-search<NonPV>(pos,ss+1,-(alpha+1),-alpha,newDepth,!cutNode); }
        if (pvNode&&(moveCount==1||(value>alpha&&(rootNode||value<beta)))){
          (ss+1)->pv=pv;
          (ss+1)->pv[0]=MOVE_NONE;
          value=-search<PV>(pos,ss+1,-beta,-alpha,
            std::min(maxNextDepth,newDepth),false);
        }
        pos.undoMove(move);
        if (threads.stop.load(std::memory_order_relaxed))
          return VALUE_ZERO;
        if (rootNode){
          RootMove& rm=*std::find(thisThread->rootMoves.begin(),
            thisThread->rootMoves.end(),move);
          rm.averageScore=rm.averageScore!=-VALUE_INFINITE?(2*value+rm.averageScore)/3:value;
          if (moveCount==1||value>alpha){
            rm.score=value;
            rm.pv.resize(1);
            for (Move* m=(ss+1)->pv; *m!=MOVE_NONE; ++m)
              rm.pv.push_back(*m);
            if (moveCount>1
              &&!thisThread->pvIdx)
              ++thisThread->bestMoveChanges;
          }
          else
            rm.score=-VALUE_INFINITE;
        }
        if (value>bestValue){
          bestValue=value;
          if (value>alpha){
            bestMove=move;
            if (pvNode&&!rootNode)
              updatePv(ss->pv,move,(ss+1)->pv);
            if (pvNode&&value<beta){
              alpha=value;
              if (depth>2
                &&depth<7
                &&beta<VALUE_KNOWN_WIN
                &&alpha>-VALUE_KNOWN_WIN)
                depth-=1;
            }
            else{
              ss->cutoffCnt++;
              break;
            }
          }
        }
        else
          ss->cutoffCnt=0;
        if (move!=bestMove){
          if (capture&&captureCount<32)
            capturesSearched[captureCount++]=move;
          else if (!capture&&quietCount<64)
            quietsSearched[quietCount++]=move;
        }
      }
      if (!moveCount)
        bestValue=excludedMove
                  ?alpha
                  :ss->inCheck
                  ?matedIn(ss->ply)
                  :VALUE_DRAW;
      else if (bestMove)
        updateAllStats(pos,ss,bestMove,bestValue,beta,prevSq,
          quietsSearched,quietCount,capturesSearched,captureCount,depth);
      else if ((depth>=4||pvNode)
        &&!priorCapture){
        bool extraBonus=pvNode
          ||cutNode
          ||bestValue<alpha-70*depth;
        updateContinuationHistories(ss-1,pos.pieceOn(prevSq),prevSq,statBonus(depth)*(1+extraBonus));
      }
      if (pvNode)
        bestValue=std::min(bestValue,maxValue);
      if (bestValue<=alpha)
        ss->ttPv=ss->ttPv||((ss-1)->ttPv&&depth>3);
      if (!excludedMove&&!(rootNode&&thisThread->pvIdx))
        tte->save(posKey,valueToTt(bestValue,ss->ply),ss->ttPv,
          bestValue>=beta?BOUND_LOWER:pvNode&&bestMove?BOUND_EXACT:BOUND_UPPER,
          depth,bestMove,ss->staticEval);
      return bestValue;
    }

    template <NodeType nodeType>
    Value qsearch(Position& pos, Stack* ss, Value alpha, const Value beta, const Depth depth){
      constexpr bool pvNode=nodeType==PV;
      Move pv[maxPly+1];
      StateInfo st;
      Move move;
      Value bestValue, futilityBase;
      if (pvNode){
        (ss+1)->pv=pv;
        ss->pv[0]=MOVE_NONE;
      }
      Thread* thisThread=pos.thisthread();
      Move bestMove=MOVE_NONE;
      ss->inCheck=pos.checkers();
      int moveCount=0;
      if (pos.isDraw(ss->ply)
        ||ss->ply>=maxPly)
        return ss->ply>=maxPly&&!ss->inCheck?evaluate(pos):VALUE_DRAW;
      const Depth ttDepth=ss->inCheck||depth>=DEPTH_QS_CHECKS
                          ?DEPTH_QS_CHECKS
                          :DEPTH_QS_NO_CHECKS;
      const uint64_t posKey=pos.key();
      TtEntry* tte=tt.probe(posKey,ss->ttHit);
      const Value ttValue=ss->ttHit?valueFromTt(tte->value(),ss->ply,pos.rule50Count()):VALUE_NONE;
      const Move ttMove=ss->ttHit?tte->move():MOVE_NONE;
      const bool pvHit=ss->ttHit&&tte->isPv();
      if (!pvNode
        &&ss->ttHit
        &&tte->depth()>=ttDepth
        &&ttValue!=VALUE_NONE
        &&tte->bound()&(ttValue>=beta?BOUND_LOWER:BOUND_UPPER))
        return ttValue;
      if (ss->inCheck){
        ss->staticEval=VALUE_NONE;
        bestValue=futilityBase=-VALUE_INFINITE;
      }
      else{
        if (ss->ttHit){
          if ((ss->staticEval=bestValue=tte->eval())==VALUE_NONE)
            ss->staticEval=bestValue=evaluate(pos);
          if (ttValue!=VALUE_NONE
            &&tte->bound()&(ttValue>bestValue?BOUND_LOWER:BOUND_UPPER))
            bestValue=ttValue;
        }
        else
          ss->staticEval=bestValue=
            (ss-1)->currentMove!=MOVE_NULL
            ?evaluate(pos)
            :-(ss-1)->staticEval;
        if (bestValue>=beta){
          if (!ss->ttHit)
            tte->save(posKey,valueToTt(bestValue,ss->ply),false,BOUND_LOWER,
              DEPTH_NONE,MOVE_NONE,ss->staticEval);
          return bestValue;
        }
        if (pvNode&&bestValue>alpha)
          alpha=bestValue;
        futilityBase=bestValue+118;
      }
      const PieceToHistory* contHist[]={
        (ss-1)->continuationHistory,(ss-2)->continuationHistory,
        nullptr,(ss-4)->continuationHistory,
        nullptr,(ss-6)->continuationHistory
      };
      const Square prevSq=toSq((ss-1)->currentMove);
      MovePicker mp(pos,ttMove,depth,&thisThread->mainHistory,
        &thisThread->captureHistory,
        contHist,
        prevSq);
      int quietCheckEvasions=0;
      while ((move=mp.nextMove())!=MOVE_NONE){
        if (!pos.legal(move))
          continue;
        const bool givesCheck=pos.givesCheck(move);
        const bool capture=pos.capture(move);
        moveCount++;
        if (bestValue>VALUE_TB_LOSS_IN_MAX_PLY
          &&!givesCheck
          &&toSq(move)!=prevSq
          &&futilityBase>-VALUE_KNOWN_WIN
          &&typeOf(move)!=PROMOTION){
          if (moveCount>2)
            continue;
          if (Value futilityValue=futilityBase+pieceValue[EG][pos.pieceOn(toSq(move))]; futilityValue<=alpha){
            bestValue=std::max(bestValue,futilityValue);
            continue;
          }
          if (futilityBase<=alpha&&!pos.seeGe(move,VALUE_ZERO+1)){
            bestValue=std::max(bestValue,futilityBase);
            continue;
          }
        }
        if (bestValue>VALUE_TB_LOSS_IN_MAX_PLY
          &&!pos.seeGe(move))
          continue;
        prefetch(tt.firstEntry(pos.keyAfter(move)));
        ss->currentMove=move;
        ss->continuationHistory=&thisThread->continuationHistory[ss->inCheck]
          [capture]
          [pos.movedPiece(move)]
          [toSq(move)];
        if (!capture
          &&bestValue>VALUE_TB_LOSS_IN_MAX_PLY
          &&(*contHist[0])[pos.movedPiece(move)][toSq(move)]<counterMovePruneThreshold
          &&(*contHist[1])[pos.movedPiece(move)][toSq(move)]<counterMovePruneThreshold)
          continue;
        if (bestValue>VALUE_TB_LOSS_IN_MAX_PLY
          &&quietCheckEvasions>1
          &&!capture
          &&ss->inCheck)
          continue;
        quietCheckEvasions+=!capture&&ss->inCheck;
        pos.doMove(move,st,givesCheck);
        const Value value=-qsearch<nodeType>(pos,ss+1,-beta,-alpha,depth-1);
        pos.undoMove(move);
        if (value>bestValue){
          bestValue=value;
          if (value>alpha){
            bestMove=move;
            if (pvNode)
              updatePv(ss->pv,move,(ss+1)->pv);
            if (pvNode&&value<beta)
              alpha=value;
            else
              break;
          }
        }
      }
      if (ss->inCheck&&bestValue==-VALUE_INFINITE){ return matedIn(ss->ply); }
      tte->save(posKey,valueToTt(bestValue,ss->ply),pvHit,
        bestValue>=beta?BOUND_LOWER:BOUND_UPPER,
        ttDepth,bestMove,ss->staticEval);
      return bestValue;
    }

    Value valueToTt(const Value v, const int ply){
      return v>=VALUE_TB_WIN_IN_MAX_PLY
             ?v+ply
             :v<=VALUE_TB_LOSS_IN_MAX_PLY
             ?v-ply
             :v;
    }

    Value valueFromTt(const Value v, const int ply, const int r50C){
      if (v==VALUE_NONE)
        return VALUE_NONE;
      if (v>=VALUE_TB_WIN_IN_MAX_PLY){
        if (v>=VALUE_MATE_IN_MAX_PLY&&VALUE_MATE-v>99-r50C)
          return VALUE_MATE_IN_MAX_PLY-1;
        return v-ply;
      }
      if (v<=VALUE_TB_LOSS_IN_MAX_PLY){
        if (v<=VALUE_MATED_IN_MAX_PLY&&VALUE_MATE+v>99-r50C)
          return VALUE_MATED_IN_MAX_PLY+1;
        return v+ply;
      }
      return v;
    }

    void updatePv(Move* pv, const Move move, const Move* childPv){
      for (*pv++=move; childPv&&*childPv!=MOVE_NONE;)
        *pv++=*childPv++;
      *pv=MOVE_NONE;
    }

    void updateAllStats(const Position& pos, Stack* ss, const Move bestMove, const Value bestValue, const Value beta,
      const Square prevSq,
      const Move* quietsSearched, const int quietCount, const Move* capturesSearched,
      const int captureCount, const Depth depth){
      const Color us=pos.stm();
      Thread* thisThread=pos.thisthread();
      CapturePieceToHistory& captureHistory=thisThread->captureHistory;
      Piece movedPce=pos.movedPiece(bestMove);
      PieceType captured=typeOf(pos.pieceOn(toSq(bestMove)));
      const int bonus1=statBonus(depth+1);
      if (!pos.capture(bestMove)){
        const int bonus2=bestValue>beta+PawnValueMg
                         ?bonus1
                         :statBonus(depth);
        updateQuietStats(pos,ss,bestMove,bonus2);
        for (int i=0; i<quietCount; ++i){
          thisThread->mainHistory[us][fromTo(quietsSearched[i])]<<-bonus2;
          updateContinuationHistories(ss,pos.movedPiece(quietsSearched[i]),toSq(quietsSearched[i]),-bonus2);
        }
      }
      else
        captureHistory[movedPce][toSq(bestMove)][captured]<<bonus1;
      if (((ss-1)->moveCount==1+(ss-1)->ttHit||(ss-1)->currentMove==(ss-1)->killers[0])
        &&!pos.capturedPiece())
        updateContinuationHistories(ss-1,pos.pieceOn(prevSq),prevSq,-bonus1);
      for (int i=0; i<captureCount; ++i){
        movedPce=pos.movedPiece(capturesSearched[i]);
        captured=typeOf(pos.pieceOn(toSq(capturesSearched[i])));
        captureHistory[movedPce][toSq(capturesSearched[i])][captured]<<-bonus1;
      }
    }

    void updateContinuationHistories(const Stack* ss, const Piece pc, const Square to, const int bonus){
      for (const int i : {1,2,4,6}){
        if (ss->inCheck&&i>2)
          break;
        if (isOk((ss-i)->currentMove))
          (*(ss-i)->continuationHistory)[pc][to]<<bonus;
      }
    }

    void updateQuietStats(const Position& pos, Stack* ss, const Move move, const int bonus){
      if (ss->killers[0]!=move){
        ss->killers[1]=ss->killers[0];
        ss->killers[0]=move;
      }
      const Color us=pos.stm();
      Thread* thisThread=pos.thisthread();
      thisThread->mainHistory[us][fromTo(move)]<<bonus;
      updateContinuationHistories(ss,pos.movedPiece(move),toSq(move),bonus);
      if (isOk((ss-1)->currentMove)){
        const Square prevSq=toSq((ss-1)->currentMove);
        thisThread->counterMoves[pos.pieceOn(prevSq)][prevSq]=move;
      }
    }
  }

  void MainThread::checkTime(){
    if (--callsCnt>0)
      return;
    callsCnt=limits.nodes?std::min(1024,static_cast<int>(limits.nodes/1024)):1024;
    static TimePoint lastInfoTime=now();
    const TimePoint elapsed=time.elapsed();
    if (const TimePoint tick=limits.startTime+elapsed; tick-lastInfoTime>=1000)
      lastInfoTime=tick;
    if (ponder)
      return;
    if ((limits.useTimeManagement()&&(elapsed>time.maximum()-10||stopOnPonderhit))
      ||(limits.movetime&&elapsed>=limits.movetime)
      ||(limits.nodes&&threads.nodesSearched()>=static_cast<uint64_t>(limits.nodes)))
      threads.stop=true;
  }

  string Uci::pv(const Position& pos, const Depth depth){
    std::stringstream ss;
    const TimePoint elapsed=time.elapsed()+1;
    const RootMoves& rootMoves=pos.thisthread()->rootMoves;
    const size_t multiPv=std::min(std::max<size_t>(1,options["MultiPV"].asSize()),
      rootMoves.size());
    const uint64_t nodesSearched=threads.nodesSearched();
    for (size_t i=0; i<multiPv; ++i){
      const bool updated=rootMoves[i].score!=-VALUE_INFINITE;
      if (depth==1&&!updated&&i>0)
        continue;
      const Depth d=updated?depth:std::max(1,depth-1);
      Value v=updated?rootMoves[i].score:rootMoves[i].previousScore;
      if (v==-VALUE_INFINITE)
        v=VALUE_ZERO;
      if (ss.rdbuf()->in_avail())
        ss<<"\n";
      ss<<"info"
        <<" depth "<<d
        <<" multipv "<<i+1
        <<" nodes "<<nodesSearched
        <<" time "<<elapsed
        <<" nps "<<nodesSearched*1000/elapsed
        <<" score "<<value(v)
        <<" pv";
      for (const Move m : rootMoves[i].pv)
        ss<<" "<<move(m,pos.isChess960());
    }
    return ss.str();
  }

  bool RootMove::extractPonderFromTt(Position& pos){
    StateInfo st;
    bool ttHit;
    if (pv[0]==MOVE_NONE)
      return false;
    pos.doMove(pv[0],st);
    const TtEntry* tte=tt.probe(pos.key(),ttHit);
    if (ttHit){
      if (const Move m=tte->move(); MoveList<LEGAL>(pos).contains(m))
        pv.push_back(m);
    }
    pos.undoMove(pv[0]);
    return pv.size()>1;
  }
}
