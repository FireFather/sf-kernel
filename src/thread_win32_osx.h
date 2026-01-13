#pragma once
#include <thread>
#if defined(__MINGW32__) || defined(__MINGW64__) || defined(USE_PTHREADS)
#include <pthread.h>
namespace Nebula{
  static const size_t TH_STACK_SIZE=8*1024*1024;

  template <class T, class P = std::pair<T*, void(T::*)()>>
  void* start_routine(void* ptr){
    P* p=reinterpret_cast<P*>(ptr);
    (p->first->*(p->second))();
    delete p;
    return NULL;
  }

  class NativeThread{
    pthread_t thread;
  public:
    template <class T, class P = std::pair<T*, void(T::*)()>>
    explicit NativeThread(void (T::*fun)(), T* obj){
      pthread_attr_t attr_storage, *attr=&attr_storage;
      pthread_attr_init(attr);
      pthread_attr_setstacksize(attr,TH_STACK_SIZE);
      pthread_create(&thread,attr,start_routine<T>,new P(obj,fun));
    }

    void join(){ pthread_join(thread,NULL); }
  };
}
#else
namespace Nebula{
  using NativeThread = std::thread;
}
#endif
