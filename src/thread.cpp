#include <algorithm>
#include "movegen.h"
#include "search.h"
#include "thread.h"
#include "uci.h"
#include "tt.h"

namespace Nebula{
  ThreadPool threads;
  Thread::Thread(const size_t n) : idx(n), stdThread(&Thread::idleLoop,this){ waitForSearchFinished(); }

  Thread::~Thread(){
    exit=true;
    startSearching();
    stdThread.join();
  }

  void Thread::clear(){
    counterMoves.fill(MOVE_NONE);
    mainHistory.fill(0);
    captureHistory.fill(0);
    previousDepth=0;
    for (const bool inCheck : {false,true})
      for (const StatsType c : {NoCaptures,Captures}){
        for (auto& to : continuationHistory[inCheck][c])
          for (auto& h : to)
            h->fill(-71);
        continuationHistory[inCheck][c][NO_PIECE][0]->fill(Search::counterMovePruneThreshold-1);
      }
  }

  void Thread::startSearching(){
    std::scoped_lock lk(mutex);
    searching=true;
    cv.notify_one();
  }

  void Thread::waitForSearchFinished(){
    std::unique_lock lk(mutex);
    cv.wait(lk,[&]{ return !searching; });
  }

  void Thread::idleLoop(){
    while (true){
      std::unique_lock lk(mutex);
      searching=false;
      cv.notify_one();
      cv.wait(lk,[&]{ return searching; });
      if (exit)
        return;
      lk.unlock();
      search();
    }
  }

  void ThreadPool::set(const size_t requested){
    if (size()>0){
      main()->waitForSearchFinished();
      while (!empty()){
        delete back();
        pop_back();
      }
    }

    if (requested>0){
      push_back(new MainThread(0));
      while (size()<requested)
        push_back(new Thread(static_cast<int>(size())));

      clear();
      tt.resize(std::max<size_t>(1,options["Hash"].asSize()));
      Search::init();
    }
  }

  void ThreadPool::clear() const{
    for (Thread* th : *this)
      th->clear();
    main()->callsCnt=0;
    main()->bestPreviousScore=VALUE_INFINITE;
    main()->bestPreviousAverageScore=VALUE_INFINITE;
    main()->previousTimeReduction=1.0;
  }

  void ThreadPool::startThinking(const Position& pos, StateListPtr& states,
    const Search::LimitsType& limits, const bool ponderMode){
    main()->waitForSearchFinished();
    main()->stopOnPonderhit=stop=false;
    increaseDepth=true;
    main()->ponder=ponderMode;
    Search::limits=limits;
    Search::RootMoves rootMoves;
    for (const auto& m : MoveList<LEGAL>(pos))
      if (limits.searchmoves.empty()
        ||std::count(limits.searchmoves.begin(),limits.searchmoves.end(),m))
        rootMoves.emplace_back(m);
    if (states.get())
      setupStates=std::move(states);
    for (Thread* th : *this){
      th->nodes=0;
      th->nmpMinPly=0;
      th->bestMoveChanges=0;
      th->rootDepth=th->completedDepth=0;
      th->rootMoves=rootMoves;
      th->rootPos.set(pos.fen(),pos.isChess960(),&th->rootState,th);
      th->rootState=setupStates->back();
    }
    main()->startSearching();
  }

  Thread* ThreadPool::getBestThread() const{
    Thread* bestThread=front();
    std::map<Move, int64_t> votes;
    Value minScore=VALUE_NONE;
    for (const Thread* th : *this)
      minScore=std::min(minScore,th->rootMoves[0].score);
    for (Thread* th : *this){
      votes[th->rootMoves[0].pv[0]]+=
        (th->rootMoves[0].score-minScore+14)*th->completedDepth;
      if (abs(bestThread->rootMoves[0].score)>=VALUE_TB_WIN_IN_MAX_PLY){
        if (th->rootMoves[0].score>bestThread->rootMoves[0].score)
          bestThread=th;
      }
      else if (th->rootMoves[0].score>=VALUE_TB_WIN_IN_MAX_PLY
        ||(th->rootMoves[0].score>VALUE_TB_LOSS_IN_MAX_PLY
          &&(votes[th->rootMoves[0].pv[0]]>votes[bestThread->rootMoves[0].pv[0]]
            ||(votes[th->rootMoves[0].pv[0]]==votes[bestThread->rootMoves[0].pv[0]]
              &&th->rootMoves[0].pv.size()>bestThread->rootMoves[0].pv.size()))))
        bestThread=th;
    }
    return bestThread;
  }

  void ThreadPool::startSearching() const{
    for (Thread* th : *this)
      if (th!=front())
        th->startSearching();
  }

  void ThreadPool::waitForSearchFinished() const{
    for (Thread* th : *this)
      if (th!=front())
        th->waitForSearchFinished();
  }
}
