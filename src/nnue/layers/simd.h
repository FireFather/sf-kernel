#pragma once
#ifdef USE_AVX2
# include <immintrin.h>
#elif defined(USE_SSE41)
# include <smmintrin.h>
#elif defined(USE_SSSE3)
# include <tmmintrin.h>
#elif defined(USE_SSE2)
# include <emmintrin.h>
#elif defined(USE_MMX)
# include <mmintrin.h>
#elif defined(USE_NEON)
# include <arm_neon.h>
#endif
#if (defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER))
#define USE_INLINE_ASM
#endif
#ifdef USE_AVXVNNI
#define VNNI_PREFIX "%{vex%} "
#else
#define VNNI_PREFIX ""
#endif
namespace Nebula::Simd{
#ifdef USE_AVX512
  [[maybe_unused]] static int m512_hadd(__m512i sum, int bias){ return _mm512_reduce_add_epi32(sum)+bias; }
  [[maybe_unused]] static __m512i m512_hadd128x16_interleave(
    __m512i sum0, __m512i sum1, __m512i sum2, __m512i sum3){
    __m512i sum01a=_mm512_unpacklo_epi32(sum0,sum1);
    __m512i sum01b=_mm512_unpackhi_epi32(sum0,sum1);

    __m512i sum23a=_mm512_unpacklo_epi32(sum2,sum3);
    __m512i sum23b=_mm512_unpackhi_epi32(sum2,sum3);

    __m512i sum01=_mm512_add_epi32(sum01a,sum01b);
    __m512i sum23=_mm512_add_epi32(sum23a,sum23b);

    __m512i sum0123a=_mm512_unpacklo_epi64(sum01,sum23);
    __m512i sum0123b=_mm512_unpackhi_epi64(sum01,sum23);

    return _mm512_add_epi32(sum0123a,sum0123b);
  }
  [[maybe_unused]] static __m128i m512_haddx4(
    __m512i sum0, __m512i sum1, __m512i sum2, __m512i sum3,
    __m128i bias){
    __m512i sum=m512_hadd128x16_interleave(sum0,sum1,sum2,sum3);

    __m256i sum256lo=_mm512_castsi512_si256(sum);
    __m256i sum256hi=_mm512_extracti64x4_epi64(sum,1);

    sum256lo=_mm256_add_epi32(sum256lo,sum256hi);

    __m128i sum128lo=_mm256_castsi256_si128(sum256lo);
    __m128i sum128hi=_mm256_extracti128_si256(sum256lo,1);

    return _mm_add_epi32(_mm_add_epi32(sum128lo,sum128hi),bias);
  }
  [[maybe_unused]] static void m512_add_dpbusd_epi32(
    __m512i& acc,
    __m512i a,
    __m512i b){

# if defined (USE_VNNI)
#   if defined (USE_INLINE_ASM)
  asm(
        "vpdpbusd %[b], %[a], %[acc]\n\t"
        : [acc]"+v"(acc)
    :  [a]"v"(a), [b]"vm"(b)
      );
#   else
  acc= _mm512_dpbusd_epi32(acc, a, b);
#   endif
# else
#   if defined (USE_INLINE_ASM)
  __m512i tmp=_mm512_maddubs_epi16(a,b);
      asm(
          "vpmaddwd    %[tmp], %[ones], %[tmp]\n\t"
          "vpaddd      %[acc], %[tmp], %[acc]\n\t"
          : [acc]"+v"(acc), [tmp]"+&v"(tmp)
    :  [ones]"v"(_mm512_set1_epi16(1))
      );
#   else
  __m512i product0=_mm512_maddubs_epi16(a,b);
  product0= _mm512_madd_epi16(product0, _mm512_set1_epi16 (1));
  acc= _mm512_add_epi32(acc, product0);
#   endif
# endif
  }

  [[maybe_unused]] static void m512_add_dpbusd_epi32x2(
    __m512i& acc,
    __m512i a0, __m512i b0,
    __m512i a1, __m512i b1){

# if defined (USE_VNNI)
#   if defined (USE_INLINE_ASM)
  asm(
        "vpdpbusd %[b0], %[a0], %[acc]\n\t"
        "vpdpbusd %[b1], %[a1], %[acc]\n\t"
        : [acc]"+v"(acc)
    :  [a0]"v"(a0), [b0]"vm"(b0), [a1]"v"(a1), [b1]"vm"(b1)
      );
#   else
  acc= _mm512_dpbusd_epi32(acc, a0, b0);
  acc= _mm512_dpbusd_epi32(acc, a1, b1);
#   endif
# else
#   if defined (USE_INLINE_ASM)
  __m512i tmp0=_mm512_maddubs_epi16(a0,b0);
  __m512i tmp1=_mm512_maddubs_epi16(a1,b1);
      asm(
          "vpaddsw     %[tmp0], %[tmp1], %[tmp0]\n\t"
          "vpmaddwd    %[tmp0], %[ones], %[tmp0]\n\t"
          "vpaddd      %[acc], %[tmp0], %[acc]\n\t"
          : [acc]"+v"(acc), [tmp0]"+&v"(tmp0)
    :  [tmp1]"v"(tmp1), [ones]"v"(_mm512_set1_epi16(1))
      );
#   else
  __m512i product0=_mm512_maddubs_epi16(a0,b0);
  __m512i product1=_mm512_maddubs_epi16(a1,b1);
  product0= _mm512_adds_epi16(product0, product1);
  product0= _mm512_madd_epi16(product0, _mm512_set1_epi16 (1));
  acc= _mm512_add_epi32(acc, product0);
#   endif
# endif
  }
#endif
#ifdef USE_AVX2
  [[maybe_unused]] static int m256_hadd(__m256i sum, int bias){
    __m128i sum128=_mm_add_epi32(_mm256_castsi256_si128(sum),_mm256_extracti128_si256(sum,1));
    sum128=_mm_add_epi32(sum128,_mm_shuffle_epi32(sum128,_MM_PERM_BADC));
    sum128=_mm_add_epi32(sum128,_mm_shuffle_epi32(sum128,_MM_PERM_CDAB));
    return _mm_cvtsi128_si32(sum128)+bias;
  }
  [[maybe_unused]] static __m128i m256_haddx4(
    __m256i sum0, __m256i sum1, __m256i sum2, __m256i sum3,
    __m128i bias){
    sum0=_mm256_hadd_epi32(sum0,sum1);
    sum2=_mm256_hadd_epi32(sum2,sum3);

    sum0=_mm256_hadd_epi32(sum0,sum2);

    __m128i sum128lo=_mm256_castsi256_si128(sum0);
    __m128i sum128hi=_mm256_extracti128_si256(sum0,1);

    return _mm_add_epi32(_mm_add_epi32(sum128lo,sum128hi),bias);
  }
  [[maybe_unused]] static void m256_add_dpbusd_epi32(
    __m256i& acc,
    __m256i a,
    __m256i b){

# if defined (USE_VNNI)
#   if defined (USE_INLINE_ASM)
  asm(
    VNNI_PREFIX"vpdpbusd %[b], %[a], %[acc]\n\t"
        : [acc]"+v"(acc)
    :  [a]"v"(a), [b]"vm"(b)
      );
#   else
  acc= _mm256_dpbusd_epi32(acc, a, b);
#   endif
# else
#   if defined (USE_INLINE_ASM)
  __m256i tmp=_mm256_maddubs_epi16(a,b);
      asm(
          "vpmaddwd    %[tmp], %[ones], %[tmp]\n\t"
          "vpaddd      %[acc], %[tmp], %[acc]\n\t"
          : [acc]"+v"(acc), [tmp]"+&v"(tmp)
    :  [ones]"v"(_mm256_set1_epi16(1))
      );
#   else
  __m256i product0=_mm256_maddubs_epi16(a,b);
  product0= _mm256_madd_epi16(product0, _mm256_set1_epi16 (1));
  acc= _mm256_add_epi32(acc, product0);
#   endif
# endif
  }

  [[maybe_unused]] static void m256_add_dpbusd_epi32x2(
    __m256i& acc,
    __m256i a0, __m256i b0,
    __m256i a1, __m256i b1){

# if defined (USE_VNNI)
#   if defined (USE_INLINE_ASM)
  asm(
    VNNI_PREFIX"vpdpbusd %[b0], %[a0], %[acc]\n\t"
  VNNI_PREFIX"vpdpbusd %[b1], %[a1], %[acc]\n\t"
        : [acc]"+v"(acc)
    :  [a0]"v"(a0), [b0]"vm"(b0), [a1]"v"(a1), [b1]"vm"(b1)
      );
#   else
  acc= _mm256_dpbusd_epi32(acc, a0, b0);
  acc= _mm256_dpbusd_epi32(acc, a1, b1);
#   endif
# else
#   if defined (USE_INLINE_ASM)
  __m256i tmp0=_mm256_maddubs_epi16(a0,b0);
  __m256i tmp1=_mm256_maddubs_epi16(a1,b1);
      asm(
          "vpaddsw     %[tmp0], %[tmp1], %[tmp0]\n\t"
          "vpmaddwd    %[tmp0], %[ones], %[tmp0]\n\t"
          "vpaddd      %[acc], %[tmp0], %[acc]\n\t"
          : [acc]"+v"(acc), [tmp0]"+&v"(tmp0)
    :  [tmp1]"v"(tmp1), [ones]"v"(_mm256_set1_epi16(1))
      );
#   else
  __m256i product0=_mm256_maddubs_epi16(a0,b0);
  __m256i product1=_mm256_maddubs_epi16(a1,b1);
  product0= _mm256_adds_epi16(product0, product1);
  product0= _mm256_madd_epi16(product0, _mm256_set1_epi16 (1));
  acc= _mm256_add_epi32(acc, product0);
#   endif
# endif
  }
#endif
#ifdef USE_SSSE3
  [[maybe_unused]] static int m128_hadd(__m128i sum, int bias){
    sum=_mm_add_epi32(sum,_mm_shuffle_epi32(sum,0x4E));
    sum=_mm_add_epi32(sum,_mm_shuffle_epi32(sum,0xB1));
    return _mm_cvtsi128_si32(sum)+bias;
  }
  [[maybe_unused]] static __m128i m128_haddx4(
    __m128i sum0, __m128i sum1, __m128i sum2, __m128i sum3,
    __m128i bias){
    sum0=_mm_hadd_epi32(sum0,sum1);
    sum2=_mm_hadd_epi32(sum2,sum3);
    sum0=_mm_hadd_epi32(sum0,sum2);
    return _mm_add_epi32(sum0,bias);
  }
  [[maybe_unused]] static void m128_add_dpbusd_epi32(
    __m128i& acc,
    __m128i a,
    __m128i b){

#   if defined (USE_INLINE_ASM)
  __m128i tmp=_mm_maddubs_epi16(a,b);
      asm(
          "pmaddwd    %[ones], %[tmp]\n\t"
          "paddd      %[tmp], %[acc]\n\t"
          : [acc]"+v"(acc), [tmp]"+&v"(tmp)
    :  [ones]"v"(_mm_set1_epi16(1))
      );
#   else
  __m128i product0=_mm_maddubs_epi16(a,b);
  product0= _mm_madd_epi16(product0, _mm_set1_epi16 (1));
  acc= _mm_add_epi32(acc, product0);
#   endif
  }

  [[maybe_unused]] static void m128_add_dpbusd_epi32x2(
    __m128i& acc,
    __m128i a0, __m128i b0,
    __m128i a1, __m128i b1){

#   if defined (USE_INLINE_ASM)
  __m128i tmp0=_mm_maddubs_epi16(a0,b0);
  __m128i tmp1=_mm_maddubs_epi16(a1,b1);
      asm(
          "paddsw     %[tmp1], %[tmp0]\n\t"
          "pmaddwd    %[ones], %[tmp0]\n\t"
          "paddd      %[tmp0], %[acc]\n\t"
          : [acc]"+v"(acc), [tmp0]"+&v"(tmp0)
    :  [tmp1]"v"(tmp1), [ones]"v"(_mm_set1_epi16(1))
      );
#   else
  __m128i product0=_mm_maddubs_epi16(a0,b0);
  __m128i product1=_mm_maddubs_epi16(a1,b1);
  product0= _mm_adds_epi16(product0, product1);
  product0= _mm_madd_epi16(product0, _mm_set1_epi16 (1));
  acc= _mm_add_epi32(acc, product0);
#   endif
  }
#endif
#ifdef USE_NEON
  [[maybe_unused]] static int neon_m128_reduce_add_epi32(int32x4_t s){

#   if USE_NEON >= 8
  return vaddvq_s32 (s);
#   else
  return s [0]+ s [1]+ s [2]+ s [3];
#   endif
  }

  [[maybe_unused]] static int neon_m128_hadd(int32x4_t sum, int bias){ return neon_m128_reduce_add_epi32(sum)+bias; }
  [[maybe_unused]] static int32x4_t neon_m128_haddx4(
    int32x4_t sum0, int32x4_t sum1, int32x4_t sum2, int32x4_t sum3,
    int32x4_t bias){
    int32x4_t hsums{
      neon_m128_reduce_add_epi32(sum0),
      neon_m128_reduce_add_epi32(sum1),
      neon_m128_reduce_add_epi32(sum2),
      neon_m128_reduce_add_epi32(sum3)
    };
    return vaddq_s32(hsums,bias);
  }
  [[maybe_unused]] static void neon_m128_add_dpbusd_epi32x2(
    int32x4_t& acc,
    int8x8_t a0, int8x8_t b0,
    int8x8_t a1, int8x8_t b1){
    int16x8_t product=vmull_s8(a0,b0);
    product=vmlal_s8(product,a1,b1);
    acc=vpadalq_s16(acc,product);
  }
#endif
}
