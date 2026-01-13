#pragma once
#include "misc.h"
#include "search.h"
#include "thread.h"

namespace Nebula{
  class TimeManagement{
  public:
    void init(Search::LimitsType& limits, Color us, int ply);
    [[nodiscard]] TimePoint optimum() const{ return optimumTime; }
    [[nodiscard]] TimePoint maximum() const{ return maximumTime; }

    [[nodiscard]] TimePoint elapsed() const{
      return Search::limits.npmsec?static_cast<TimePoint>(threads.nodesSearched()):now()-startTime;
    }

    int64_t availableNodes;
  private:
    TimePoint startTime=0;
    TimePoint optimumTime=0;
    TimePoint maximumTime=0;
  };

  extern TimeManagement time;
}
