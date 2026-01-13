#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdlib>
#ifdef _WIN32
#include <direct.h>
#endif
#include "misc.h"
#include "thread.h"
using namespace std;

namespace Nebula{
  namespace{
    const string engine="sf-kernel";
    const string version="1.0";
    const string author="the Stockfish developers (see AUTHORS file)";
  }

  string engineInfo(){
    stringstream ss;
    ss<<"id name "<<engine<<" "<<version<<std::endl;
    ss<<"id author "<<author;
    return ss.str();
  }

  void* alignedLargePagesAlloc(const size_t size){ return stdAlignedAlloc(alignof(std::max_align_t),size); }
  void alignedLargePagesFree(void* mem){ stdAlignedFree(mem); }

  void* stdAlignedAlloc(size_t alignment, size_t size){
#ifdef _WIN32
    return _mm_malloc(size,alignment);
#elif defined(POSIXALIGNEDALLOC)
    void* mem;
    return posix_memalign(&mem,alignment,size)?nullptr:mem;
#else
    return std::aligned_alloc(alignment,size);
#endif
  }

  void stdAlignedFree(void* ptr){
#ifdef _WIN32
    _mm_free(ptr);
#else
    free(ptr);
#endif
  }

  namespace CommandLine{
    namespace{
      string argv0;
    }

    string binaryDirectory;
    string workingDirectory;

    void init(const int argc, char* argv[]){
      (void)argc;
      argv0=argv[0];

#ifdef _WIN32
      const string pathSeparator="\\";
#ifdef _MSC_VER
      char* pgmptr=nullptr;
      if (!_get_pgmptr(&pgmptr)&&pgmptr&&*pgmptr)
        argv0=pgmptr;
#endif
#else
      const string pathSeparator="/";
#endif

      char buff[40000];
#ifdef _WIN32
      _getcwd(buff,40000);
#else
      getcwd(buff,40000);
#endif
      workingDirectory=buff;

      binaryDirectory=argv0;
      if (const size_t pos=binaryDirectory.find_last_of("\\/"); pos==string::npos)
        binaryDirectory="."+pathSeparator;
      else
        binaryDirectory.resize(pos+1);

      if (binaryDirectory.starts_with("."+pathSeparator))
        binaryDirectory.replace(0,1,workingDirectory);
    }
  }
}
