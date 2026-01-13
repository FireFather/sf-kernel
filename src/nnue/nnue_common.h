#pragma once
#include <cstring>
#include "../misc.h"
#ifdef USE_AVX2
#include <immintrin.h>
#elif defined(USE_SSE41)
#include <smmintrin.h>
#elif defined(USE_SSSE3)
#include <tmmintrin.h>
#elif defined(USE_SSE2)
#include <emmintrin.h>
#elif defined(USE_MMX)
#include <mmintrin.h>
#elif defined(USE_NEON)
#include <arm_neon.h>
#endif
namespace Nebula::Eval::Nnue{
  constexpr std::uint32_t nnueVersion=0x7AF32F20u;
  constexpr int outputScale=16;
  constexpr int weightScaleBits=6;
  constexpr std::size_t cacheLineSize=64;
#ifdef USE_AVX2
  constexpr std::size_t SimdWidth=32;
#elif defined(USE_SSE2)
  constexpr std::size_t SimdWidth=16;
#elif defined(USE_MMX)
  constexpr std::size_t SimdWidth=8;
#elif defined(USE_NEON)
  constexpr std::size_t SimdWidth=16;
#endif
  constexpr std::size_t maxSimdWidth=32;
  using TransformedFeatureType = std::uint8_t;
  using IndexType = std::uint32_t;

  template <typename IntType>
  constexpr IntType ceilToMultiple(IntType n, IntType base){ return (n+base-1)/base*base; }

  template <typename IntType>
  IntType readLittleEndian(std::istream& stream){
    IntType result;
    if (isLittleEndian)
      stream.read(reinterpret_cast<char*>(&result),sizeof(IntType));
    else{
      std::uint8_t u[sizeof(IntType)];
      std::make_unsigned_t<IntType> v=0;
      stream.read(reinterpret_cast<char*>(u),sizeof(IntType));
      for (std::size_t i=0; i<sizeof(IntType); ++i){
        v=static_cast<IntType>((v<<8)|
          static_cast<IntType>(u[sizeof(IntType)-i-1]));
      }
      std::memcpy(&result,&v,sizeof(IntType));
    }
    return result;
  }

  template <typename IntType>
  void writeLittleEndian(std::ostream& stream, IntType value){
    if (isLittleEndian)
      stream.write(reinterpret_cast<const char*>(&value),sizeof(IntType));
    else{
      std::uint8_t u[sizeof(IntType)];
      std::make_unsigned_t<IntType> v=value;
      std::size_t i=0;
      if constexpr (sizeof(IntType)>1){
        for (; i+1<sizeof(IntType); ++i){
          u[i]=static_cast<std::uint8_t>(v);
          v>>=8;
        }
      }
      u[i]=static_cast<std::uint8_t>(v);
      stream.write(reinterpret_cast<char*>(u),sizeof(IntType));
    }
  }

  template <typename IntType>
  void readLittleEndian(std::istream& stream, IntType* out, const std::size_t count){
    if (isLittleEndian)
      stream.read(reinterpret_cast<char*>(out),sizeof(IntType)*count);
    else
      for (std::size_t i=0; i<count; ++i)
        out[i]=readLittleEndian<IntType>(stream);
  }

  template <typename IntType>
  void writeLittleEndian(std::ostream& stream, const IntType* values, const std::size_t count){
    if (isLittleEndian)
      stream.write(reinterpret_cast<const char*>(values),sizeof(IntType)*count);
    else
      for (std::size_t i=0; i<count; ++i)
        writeLittleEndian<IntType>(stream,values[i]);
  }
}
