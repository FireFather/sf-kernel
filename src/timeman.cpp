#include <algorithm>
#include <cmath>
#include "search.h"
#include "timeman.h"
#include "uci.h"

namespace Nebula{
  TimeManagement time;

  void TimeManagement::init(Search::LimitsType& limits, const Color us, const int ply){
    constexpr TimePoint moveOverhead=10;
    constexpr TimePoint slowMover=100;
    double optScale, maxScale;
    if constexpr (constexpr TimePoint npmsec=0){
      if (!availableNodes)
        availableNodes=npmsec*limits.time[us];
      limits.time[us]=availableNodes;
      limits.inc[us]*=npmsec;
      limits.npmsec=npmsec;
    }
    startTime=limits.startTime;
    const int mtg=limits.movestogo?std::min(limits.movestogo,50):50;
    TimePoint timeLeft=std::max(static_cast<TimePoint>(1),
      limits.time[us]+limits.inc[us]*(mtg-1)-moveOverhead*(2+mtg));
    const double optExtra=std::clamp(
      1.0+12.0*static_cast<double>(limits.inc[us])/static_cast<double>(limits.time[us]),1.0,1.12);
    timeLeft=slowMover*timeLeft/100;
    if (limits.movestogo==0){
      optScale=std::min(0.0084+std::pow(ply+3.0,0.5)*0.0042,
          0.2*static_cast<double>(limits.time[us])/static_cast<double>(timeLeft))
        *optExtra;
      maxScale=std::min(7.0,4.0+ply/12.0);
    }
    else{
      optScale=std::min((0.88+ply/116.4)/mtg,
        0.88*static_cast<double>(limits.time[us])/static_cast<double>(timeLeft));
      maxScale=std::min(6.3,1.5+0.11*mtg);
    }
    optimumTime=static_cast<TimePoint>(optScale*static_cast<double>(timeLeft));
    maximumTime=static_cast<TimePoint>(std::min(0.8*static_cast<double>(limits.time[us])-moveOverhead,
      maxScale*static_cast<double>(optimumTime)));
    if (options["Ponder"].asBool())
      optimumTime+=optimumTime/4;
  }
}
