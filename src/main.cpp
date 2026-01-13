#include "bitboard.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "uci.h"
using namespace Nebula;

int main(const int argc, char* argv[]){
  CommandLine::init(argc,argv);
  init(options);
  Bitboards::init();
  Position::init();
  const size_t threadCount=std::max<size_t>(1,options["Threads"].asSize());
  threads.set(threadCount);
  Search::clear();
  Eval::Nnue::init();
  Uci::loop(argc,argv);
  threads.set(0);
  return 0;
}
