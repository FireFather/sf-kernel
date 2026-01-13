#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <mutex>

namespace Nebula{
  static std::mutex mutexCout;
  template <typename T>
  concept Streamable=requires(std::ostream& os, const T& v){
    { os<<v } -> std::same_as<std::ostream&>;
  };
  template <typename T>
  concept AsyncPrintable=
    !std::is_function_v<std::remove_pointer_t<T>>&&
    requires(std::ostream& os, const T& v){
      { os<<v } -> std::same_as<std::ostream&>;
    };

  struct async{
    std::unique_lock<std::mutex> lk;
    async() : lk(mutexCout){}

    template <AsyncPrintable T>
    async& operator<<(const T& t) &{
      std::cout<<t;
      return *this;
    }

    template <AsyncPrintable T>
    async&& operator<<(const T& t) &&{
      std::cout<<t;
      return std::move(*this);
    }

    async& operator<<(std::ostream& (*fp)(std::ostream&)) &{
      std::cout<<fp;
      return *this;
    }

    async&& operator<<(std::ostream& (*fp)(std::ostream&)) &&{
      std::cout<<fp;
      return std::move(*this);
    }
  };

  std::string engineInfo();
  void* stdAlignedAlloc(size_t alignment, size_t size);
  void stdAlignedFree(void* ptr);
  using TimePoint = std::chrono::milliseconds::rep;

  inline TimePoint now(){
    return std::chrono::duration_cast<std::chrono::milliseconds>
      (std::chrono::steady_clock::now().time_since_epoch()).count();
  }

  template <class Entry, int Size>
  struct HashTable{
    Entry* operator[](const uint64_t key){ return &table[static_cast<uint32_t>(key)&Size-1]; }
  private:
    std::vector<Entry> table=std::vector<Entry>(Size);
  };

  template <uintptr_t Alignment, typename T>
  T* alignPtrUp(T* ptr){
    const uintptr_t ptrint=reinterpret_cast<uintptr_t>(reinterpret_cast<char*>(ptr));
    return reinterpret_cast<T*>(
      reinterpret_cast<char*>((ptrint+(Alignment-1))/Alignment*Alignment));
  }

  static inline constexpr union{
    uint32_t i;
    char c[4];
  } le={0x01020304};

  static inline const bool isLittleEndian=le.c[0]==4;

  class RunningAverage{
  public:
    void set(const int64_t p, const int64_t q){ average=p*PERIOD*RESOLUTION/q; }
    void update(const int64_t v){ average=RESOLUTION*v+(PERIOD-1)*average/PERIOD; }
    [[nodiscard]] bool isGreater(const int64_t a, const int64_t b) const{ return b*average>a*(PERIOD*RESOLUTION); }
    [[nodiscard]] int64_t value() const{ return average/(PERIOD*RESOLUTION); }
  private:
    static constexpr int64_t PERIOD=4096;
    static constexpr int64_t RESOLUTION=1024;
    int64_t average=0;
  };

  template <typename T, std::size_t MaxSize>
  class ValueList{
  public:
    [[nodiscard]] std::size_t size() const{ return size_; }
    void pushBack(const T& value){ values_[size_++]=value; }
    const T* begin() const{ return values_; }
    const T* end() const{ return values_+size_; }
  private:
    T values_[MaxSize];
    std::size_t size_=0;
  };

  inline int64_t sigmoid(const int64_t t, const int64_t x0,
    const int64_t y0,
    const int64_t c,
    const int64_t p,
    const int64_t q){ return y0+p*(t-x0)/(q*(std::abs(t-x0)+c)); }

  class Prng{
    uint64_t s;

    uint64_t rand64(){
      s^=s>>12;
      s^=s<<25;
      s^=s>>27;
      return s*2685821657736338717ULL;
    }
  public:
    explicit Prng(const uint64_t seed) : s(seed){}

    template <typename T>
    T rand(){ return T(rand64()); }

    template <typename T>
    T sparseRand(){ return T(rand64()&rand64()&rand64()); }
  };

  inline uint64_t mulHi64(const uint64_t a, const uint64_t b){
    const uint64_t aL=static_cast<uint32_t>(a), aH=a>>32;
    const uint64_t bL=static_cast<uint32_t>(b), bH=b>>32;
    const uint64_t c1=(aL*bL)>>32;
    const uint64_t c2=aH*bL+c1;
    const uint64_t c3=aL*bH+static_cast<uint32_t>(c2);
    return aH*bH+(c2>>32)+(c3>>32);
  }

  namespace CommandLine{
    void init(int argc, char* argv[]);
    extern std::string binaryDirectory;
    extern std::string workingDirectory;
  }

  void* alignedLargePagesAlloc(size_t size);
  void alignedLargePagesFree(void* mem);
}
