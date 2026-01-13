#pragma once
#include "../nnue_common.h"

namespace Nebula::Eval::Nnue::Layers{
  template <IndexType InDims>
  class SqrClippedReLu{
  public:
    using InputType = std::int32_t;
    using OutputType = std::uint8_t;
    static constexpr IndexType InputDimensions=InDims;
    static constexpr IndexType OutputDimensions=InputDimensions;
    static constexpr IndexType PaddedOutputDimensions=
      ceilToMultiple<IndexType>(OutputDimensions,32);
    using OutputBuffer = OutputType[PaddedOutputDimensions];

    static constexpr std::uint32_t getHashValue(const std::uint32_t prevHash){
      std::uint32_t hashValue=0x538D24C7u;
      hashValue+=prevHash;
      return hashValue;
    }

    static bool readParameters(std::istream&){ return true; }
    static bool writeParameters(std::ostream&){ return true; }

    const OutputType* propagate(
      const InputType* input, OutputType* output) const{
#ifdef USE_SSE2
      constexpr IndexType NumChunks=InputDimensions/16;
#ifdef USE_SSE41
      const __m128i Zero=_mm_setzero_si128();
#else
      const __m128i k0x80s=_mm_set1_epi8(-128);
#endif
      const auto in=reinterpret_cast<const __m128i*>(input);
      const auto out=reinterpret_cast<__m128i*>(output);
      for (IndexType i=0; i<NumChunks; ++i){
        __m128i words0=_mm_packs_epi32(
          _mm_load_si128(&in[i*4+0]),
          _mm_load_si128(&in[i*4+1]));
        __m128i words1=_mm_packs_epi32(
          _mm_load_si128(&in[i*4+2]),
          _mm_load_si128(&in[i*4+3]));
        words0=_mm_srli_epi16(_mm_mulhi_epi16(words0,words0),3);
        words1=_mm_srli_epi16(_mm_mulhi_epi16(words1,words1),3);
        const __m128i packedbytes=_mm_packs_epi16(words0,words1);
        _mm_store_si128(&out[i],
#ifdef USE_SSE41
      _mm_max_epi8(packedbytes,Zero)
#else
      _mm_subs_epi8(_mm_adds_epi8(packedbytes,k0x80s),k0x80s)
#endif
				);
			}
      constexpr IndexType start=NumChunks*16;
#else
      constexpr IndexType start=0;
#endif
      for (IndexType i=start; i<InputDimensions; ++i){
        output[i]=static_cast<OutputType>(
          std::max(0ll,std::min(
            127ll,((static_cast<long long>(input[i])*input[i])>>(2*weightScaleBits))/128)));
      }
      return output;
    }
  };
}
