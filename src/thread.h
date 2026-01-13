#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include "movepick.h"
#include "position.h"
#include "search.h"
#include "thread_win32_osx.h"

namespace Nebula{
  class Thread{
    std::mutex mutex;
    std::condition_variable cv;
    size_t idx;
    bool exit=false, searching=true;
    NativeThread stdThread;
  public:
    explicit Thread(size_t);
    virtual ~Thread();
    virtual void search();
    void clear();
    void idleLoop();
    void startSearching();
    void waitForSearchFinished();
    size_t id() const{ return idx; }
    size_t pvIdx, pvLast;
    RunningAverage complexityAverage;
    std::atomic<uint64_t> nodes, bestMoveChanges;
    int nmpMinPly;
    Color nmpColor;
    Value bestValue, optimism[COLOR_NB];
    Position rootPos;
    StateInfo rootState;
    Search::RootMoves rootMoves;
    Depth rootDepth, completedDepth, previousDepth;
    Value rootDelta;
    CounterMoveHistory counterMoves;
    ButterflyHistory mainHistory;
    CapturePieceToHistory captureHistory;
    ContinuationHistory continuationHistory[2][2];
    Score trend;
  };

  struct MainThread final : Thread{
    using Thread::Thread;
    void search() override;
    void checkTime();
    double previousTimeReduction;
    Value bestPreviousScore;
    Value bestPreviousAverageScore;
    Value iterValue[4];
    int callsCnt;
    bool stopOnPonderhit;
    std::atomic_bool ponder;
  };

  struct ThreadPool : std::vector<Thread*>{
    void startThinking(const Position&, StateListPtr&, const Search::LimitsType&, bool=false);
    void clear() const;
    void set(size_t);
    MainThread* main() const{ return dynamic_cast<MainThread*>(front()); }
    uint64_t nodesSearched() const{ return accumulate(&Thread::nodes); }
    Thread* getBestThread() const;
    void startSearching() const;
    void waitForSearchFinished() const;
    std::atomic_bool stop, increaseDepth;
  private:
    StateListPtr setupStates;

    uint64_t accumulate(std::atomic<uint64_t> Thread::* member) const{
      uint64_t sum=0;
      for (const Thread* th : *this)
        sum+=(th->*member).load(std::memory_order_relaxed);
      return sum;
    }
  };

  extern ThreadPool threads;
}
