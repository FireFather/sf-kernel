#include <algorithm>
#include <fstream>
#include <iomanip>
#include <streambuf>
#include <vector>
#include "evaluate.h"
#include "misc.h"
#include "thread.h"
#include "incbin/incbin.h"
#if !defined(_MSC_VER) && !defined(NNUE_EMBEDDING_OFF)
INCBIN(EmbeddedNNUE,NnueNetDefaultName);
#else
constexpr unsigned char gEmbeddedNNUEData[1]={};
const unsigned char* const gEmbeddedNNUEEnd=&gEmbeddedNNUEData[1];
constexpr unsigned int gEmbeddedNNUESize=1;
#endif
using namespace std;

namespace Nebula{
  namespace Eval{
    string currentNnueNetName;

    void Nnue::init(){
      const string evalFile=NnueNetDefaultName;
      for (const vector<string> dirs={"<internal>","",CommandLine::binaryDirectory}; const string& directory : dirs)
        if (currentNnueNetName!=evalFile){
          if (directory!="<internal>"){
            if (ifstream stream(directory+evalFile,ios::binary); loadEval(evalFile,stream))
              currentNnueNetName=evalFile;
          }
          if (directory=="<internal>"&&evalFile==NnueNetDefaultName){
            class MemoryBuffer : public basic_streambuf<char>{
            public:
              MemoryBuffer(char* p, const size_t n){
                setg(p,p,p+n);
                setp(p,p+n);
              }
            };
            MemoryBuffer buffer(const_cast<char*>(reinterpret_cast<const char*>(gEmbeddedNNUEData)),
              gEmbeddedNNUESize);
            (void)gEmbeddedNNUEEnd;
            if (istream stream(&buffer); loadEval(evalFile,stream))
              currentNnueNetName=evalFile;
          }
        }
    }
  }

  Value Eval::evaluate(const Position& pos, int* complexity){
    const Color stm=pos.stm();
    int nnueComplexity;
    const int scale=1064+106*pos.nonPawnMaterial()/5120;
    Value optimism=pos.thisthread()->optimism[stm];
    const Value nnue=Nnue::evaluate(pos,true,&nnueComplexity);
    nnueComplexity=(104*nnueComplexity+131*abs(nnue))/256;
    if (complexity)
      *complexity=nnueComplexity;
    optimism=optimism*(269+nnueComplexity)/256;
    Value v=(nnue*scale+optimism*(scale-754))/1024;
    v=v*(195-pos.rule50Count())/211;
    v=std::clamp(v,VALUE_TB_LOSS_IN_MAX_PLY+1,VALUE_TB_WIN_IN_MAX_PLY-1);
    return v;
  }
}
