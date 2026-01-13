#pragma once
#include "../nnue_common.h"

namespace Nebula::Eval::Nnue::Layers{
  template <IndexType InDims>
  class ClippedReLu{
  public:
    using InputType = std::int32_t;
    using OutputType = std::uint8_t;
    static constexpr IndexType InputDimensions=InDims;
    static constexpr IndexType OutputDimensions=InputDimensions;
    static constexpr IndexType PaddedOutputDimensions=
      ceilToMultiple<IndexType>(OutputDimensions,32);
    using OutputBuffer = OutputType[PaddedOutputDimensions];

    static constexpr std::uint32_t getHashValue(const std::uint32_t prevHash){
      std::uint32_t value=0x538D24C7u;
      value+=prevHash;
      return value;
    }

    static bool readParameters(std::istream&){ return true; }
    static bool writeParameters(std::ostream&){ return true; }

    const OutputType* propagate(
      const InputType* input, OutputType* output) const{
#ifdef USE_AVX2
      if constexpr (InputDimensions%SimdWidth==0){
        constexpr IndexType NumChunks=InputDimensions/SimdWidth;
        const __m256i Zero=_mm256_setzero_si256();
        const __m256i Offsets=_mm256_set_epi32(7,3,6,2,5,1,4,0);
        const auto in=reinterpret_cast<const __m256i*>(input);
        const auto out=reinterpret_cast<__m256i*>(output);
        for (IndexType i=0; i<NumChunks; ++i){
          const __m256i words0=_mm256_srai_epi16(_mm256_packs_epi32(
            _mm256_load_si256(&in[i*4+0]),
            _mm256_load_si256(&in[i*4+1])),weightScaleBits);
          const __m256i words1=_mm256_srai_epi16(_mm256_packs_epi32(
            _mm256_load_si256(&in[i*4+2]),
            _mm256_load_si256(&in[i*4+3])),weightScaleBits);
          _mm256_store_si256(&out[i],_mm256_permutevar8x32_epi32(_mm256_max_epi8(
              _mm256_packs_epi16(words0,words1),Zero),
            Offsets));
        }
      }
      else{
        constexpr IndexType NumChunks=InputDimensions/(SimdWidth/2);
        const __m128i Zero=_mm_setzero_si128();
        const auto in=reinterpret_cast<const __m128i*>(input);
        const auto out=reinterpret_cast<__m128i*>(output);
        for (IndexType i=0; i<NumChunks; ++i){
          const __m128i words0=_mm_srai_epi16(_mm_packs_epi32(
            _mm_load_si128(&in[i*4+0]),
            _mm_load_si128(&in[i*4+1])),weightScaleBits);
          const __m128i words1=_mm_srai_epi16(_mm_packs_epi32(
            _mm_load_si128(&in[i*4+2]),
            _mm_load_si128(&in[i*4+3])),weightScaleBits);
          const __m128i packedbytes=_mm_packs_epi16(words0,words1);
          _mm_store_si128(&out[i],_mm_max_epi8(packedbytes,Zero));
        }
      }
      constexpr IndexType start=
        InputDimensions%SimdWidth==0
        ?InputDimensions/SimdWidth*SimdWidth
        :InputDimensions/(SimdWidth/2)*(SimdWidth/2);
#elif defined(USE_SSE2)
      constexpr IndexType NumChunks=InputDimensions/SimdWidth;
#ifdef USE_SSE41
      const __m128i Zero=_mm_setzero_si128();
#else
      const __m128i k0x80s=_mm_set1_epi8(-128);
#endif
      const auto in=reinterpret_cast<const __m128i*>(input);
      const auto out=reinterpret_cast<__m128i*>(output);
      for (IndexType i=0; i<NumChunks; ++i){
        const __m128i words0=_mm_srai_epi16(_mm_packs_epi32(
          _mm_load_si128(&in[i*4+0]),
          _mm_load_si128(&in[i*4+1])),weightScaleBits);
        const __m128i words1=_mm_srai_epi16(_mm_packs_epi32(
          _mm_load_si128(&in[i*4+2]),
          _mm_load_si128(&in[i*4+3])),weightScaleBits);
        const __m128i packedbytes=_mm_packs_epi16(words0,words1);
        _mm_store_si128(&out[i],
#ifdef USE_SSE41
      _mm_max_epi8(packedbytes,Zero)
#else
      _mm_subs_epi8(_mm_adds_epi8(packedbytes,k0x80s),k0x80s)
#endif
				);
			}
      constexpr IndexType start=NumChunks*SimdWidth;
#elif defined(USE_MMX)
      constexpr IndexType NumChunks=InputDimensions/SimdWidth;
      const __m64 k0x80s=_mm_set1_pi8(-128);
      const auto in=reinterpret_cast<const __m64*>(input);
      const auto out=reinterpret_cast<__m64*>(output);
      for (IndexType i=0; i<NumChunks; ++i){
        const __m64 words0=_mm_srai_pi16(
          _mm_packs_pi32(in[i*4+0],in[i*4+1]),
          weightScaleBits);
        const __m64 words1=_mm_srai_pi16(
          _mm_packs_pi32(in[i*4+2],in[i*4+3]),
          weightScaleBits);
        const __m64 packedbytes=_mm_packs_pi16(words0,words1);
        out[i]=_mm_subs_pi8(_mm_adds_pi8(packedbytes,k0x80s),k0x80s);
      }
      _mm_empty();
      constexpr IndexType start=NumChunks*SimdWidth;
#elif defined(USE_NEON)
      constexpr IndexType NumChunks=InputDimensions/(SimdWidth/2);
      const int8x8_t Zero={0};
      const auto in=reinterpret_cast<const int32x4_t*>(input);
      const auto out=reinterpret_cast<int8x8_t*>(output);
      for (IndexType i=0; i<NumChunks; ++i){
        int16x8_t shifted;
        const auto pack=reinterpret_cast<int16x4_t*>(&shifted);
        pack[0]=vqshrn_n_s32(in[i*2+0],weightScaleBits);
        pack[1]=vqshrn_n_s32(in[i*2+1],weightScaleBits);
        out[i]=vmax_s8(vqmovn_s16(shifted),Zero);
      }
      constexpr IndexType start=NumChunks*(SimdWidth/2);
#else
      constexpr IndexType start=0;
#endif
      for (IndexType i=start; i<InputDimensions; ++i){
        output[i]=static_cast<OutputType>(
          std::max(0,std::min(127,input[i]>>weightScaleBits)));
      }
      return output;
    }
  };
}
