#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include "movegen.h"
#include "bench.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "uci.h"
using namespace std;

namespace Nebula{
  extern vector<string> setupBench();

  namespace{
    auto startFen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    void position(Position& pos, istringstream& is, StateListPtr& states){
      Move m;
      string token, fen;
      is>>token;
      if (token=="startpos"){
        fen=startFen;
        is>>token;
      }
      else if (token=="fen")
        while (is>>token&&token!="moves")
          fen+=token+" ";
      else
        return;
      states=std::make_unique<std::deque<StateInfo>>(1);
      pos.set(fen,false,&states->back(),threads.main());
      while (is>>token&&(m=Uci::toMove(pos,token))!=MOVE_NONE){
        states->emplace_back();
        pos.doMove(m,states->back());
      }
    }

    void setoption(istringstream& is){
      string token, name, value;
      is>>token;
      while (is>>token&&token!="value")
        name+=(name.empty()?"":" ")+token;
      while (is>>token)
        value+=(value.empty()?"":" ")+token;
      if (options.contains(name))
        options[name]=value;
    }

    void go(const Position& pos, istringstream& is, StateListPtr& states){
      Search::LimitsType limits;
      string token;
      bool ponderMode=false;
      limits.startTime=now();
      while (is>>token)
        if (token=="searchmoves")
          while (is>>token)
            limits.searchmoves.push_back(Uci::toMove(pos,token));
        else if (token=="wtime") is>>limits.time[WHITE];
        else if (token=="btime") is>>limits.time[BLACK];
        else if (token=="winc") is>>limits.inc[WHITE];
        else if (token=="binc") is>>limits.inc[BLACK];
        else if (token=="movestogo") is>>limits.movestogo;
        else if (token=="depth") is>>limits.depth;
        else if (token=="nodes") is>>limits.nodes;
        else if (token=="movetime") is>>limits.movetime;
        else if (token=="mate") is>>limits.mate;
        else if (token=="infinite") limits.infinite=1;
        else if (token=="ponder") ponderMode=true;
      threads.startThinking(pos,states,limits,ponderMode);
    }

    void bench(const Position& pos, StateListPtr& states){
      string token;
      uint64_t nodes=0, cnt=1;
      vector<string> list=setupBench();
      const uint64_t num=ranges::count_if(list,[](const string& s){ return s.starts_with("go "); });
      TimePoint elapsed=now();
      for (const auto& cmd : list){
        istringstream is(cmd);
        is>>skipws>>token;
        if (token=="go"){
          cout<<"\nPosition: "<<cnt++<<'/'<<num<<endl;
          if (token=="go"){
            go(pos,is,states);
            threads.main()->waitForSearchFinished();
            nodes+=threads.nodesSearched();
          }
        }
      }
      elapsed=now()-elapsed+1;
      cout<<"\nTime (ms) : "<<elapsed
        <<"\nNodes     : "<<nodes
        <<"\nNPS       : "<<1000*nodes/elapsed<<endl;
    }
  }

  void Uci::loop(const int argc, char* argv[]){
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    Position pos;
    string token, cmd;
    StateListPtr states(new std::deque<StateInfo>(1));
    pos.set(startFen,false,&states->back(),threads.main());
    for (int i=1; i<argc; ++i)
      cmd+=std::string(argv[i])+" ";
    do{
      if (argc==1&&!getline(cin,cmd))
        cmd="quit";
      istringstream is(cmd);
      token.clear();
      is>>skipws>>token;
      if (token=="quit"
        ||token=="stop")
        threads.stop=true;
      else if (token=="ponderhit")
        threads.main()->ponder=false;
      else if (token=="uci")
        async()<<engineInfo()
          <<options<<"\nuciok"<<std::endl;
      else if (token=="setoption") setoption(is);
      else if (token=="go") go(pos,is,states);
      else if (token=="position") position(pos,is,states);
      else if (token=="ucinewgame") Search::clear();
      else if (token=="isready") async()<<"readyok"<<std::endl;
      else if (token=="bench") bench(pos,states);
      else if (token=="perft"){
        int d=1;
        is>>d;
        d=std::max(d,1);
        perft(pos,d);
      }
    }
    while (token!="quit"&&argc==1);
  }

  string Uci::value(const Value v){
    stringstream ss;
    if (abs(v)<VALUE_MATE_IN_MAX_PLY)
      ss<<"cp "<<v*100/PawnValueEg;
    else
      ss<<"mate "<<(v>0?VALUE_MATE-v+1:-VALUE_MATE-v)/2;
    return ss.str();
  }

  std::string Uci::square(const Square s){
    return std::string{static_cast<char>('a'+fileOf(s)),static_cast<char>('1'+rankOf(s))};
  }

  string Uci::move(const Move m, const bool chess960){
    const Square from=fromSq(m);
    Square to=toSq(m);
    if (m==MOVE_NONE)
      return "";
    if (m==MOVE_NULL)
      return "";
    if (typeOf(m)==CASTLING&&!chess960)
      to=makeSquare(to>from?FILE_G:FILE_C,rankOf(from));
    string move=square(from)+square(to);
    if (typeOf(m)==PROMOTION)
      move+=" pnbrqk"[promotionType(m)];
    return move;
  }

  Move Uci::toMove(const Position& pos, string& str){
    if (str.length()==5)
      str[4]=static_cast<char>(tolower(str[4]));
    for (const auto& m : MoveList<LEGAL>(pos))
      if (str==move(m,pos.isChess960()))
        return m;
    return MOVE_NONE;
  }
}
