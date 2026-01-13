#pragma once
#include "nnue_common.h"
#include "nnue_architecture.h"
#include <cstring>

namespace Nebula::Eval::Nnue{
  using BiasType = std::int16_t;
  using WeightType = std::int16_t;
  using PsqtWeightType = std::int32_t;
#define VECTOR
#ifdef USE_AVX512
  typedef __m512i vec_t;
  typedef __m256i psqt_vec_t;
#define vec_load(a) _mm512_load_si512(a)
#define vec_store(a,b) _mm512_store_si512(a,b)
#define vec_add_16(a,b) _mm512_add_epi16(a,b)
#define vec_sub_16(a,b) _mm512_sub_epi16(a,b)
#define vec_mul_16(a,b) _mm512_mullo_epi16(a,b)
#define vec_zero() _mm512_setzero_epi32()
#define vec_set_16(a) _mm512_set1_epi16(a)
#define vec_max_16(a,b) _mm512_max_epi16(a,b)
#define vec_min_16(a,b) _mm512_min_epi16(a,b)
  inline vec_t vec_msb_pack_16(vec_t a, vec_t b){
    vec_t compacted=_mm512_packs_epi16(_mm512_srli_epi16(a,7),_mm512_srli_epi16(b,7));
    return _mm512_permutexvar_epi64(_mm512_setr_epi64(0,2,4,6,1,3,5,7),compacted);
  }
#define vec_load_psqt(a) _mm256_load_si256(a)
#define vec_store_psqt(a,b) _mm256_store_si256(a,b)
#define vec_add_psqt_32(a,b) _mm256_add_epi32(a,b)
#define vec_sub_psqt_32(a,b) _mm256_sub_epi32(a,b)
#define vec_zero_psqt() _mm256_setzero_si256()
#define NumRegistersSIMD 32
#define MaxChunkSize 64
#elif USE_AVX2
  typedef __m256i vec_t;
  typedef __m256i psqt_vec_t;
#define vec_load(a) _mm256_load_si256(a)
#define vec_store(a,b) _mm256_store_si256(a,b)
#define vec_add_16(a,b) _mm256_add_epi16(a,b)
#define vec_sub_16(a,b) _mm256_sub_epi16(a,b)
#define vec_mul_16(a,b) _mm256_mullo_epi16(a,b)
#define vec_zero() _mm256_setzero_si256()
#define vec_set_16(a) _mm256_set1_epi16(a)
#define vec_max_16(a,b) _mm256_max_epi16(a,b)
#define vec_min_16(a,b) _mm256_min_epi16(a,b)
  inline vec_t vec_msb_pack_16(vec_t a, vec_t b){
    vec_t compacted=_mm256_packs_epi16(_mm256_srli_epi16(a,7),_mm256_srli_epi16(b,7));
    return _mm256_permute4x64_epi64(compacted,0b11011000);
  }
#define vec_load_psqt(a) _mm256_load_si256(a)
#define vec_store_psqt(a,b) _mm256_store_si256(a,b)
#define vec_add_psqt_32(a,b) _mm256_add_epi32(a,b)
#define vec_sub_psqt_32(a,b) _mm256_sub_epi32(a,b)
#define vec_zero_psqt() _mm256_setzero_si256()
#define NumRegistersSIMD 16
#define MaxChunkSize 32
#elif USE_SSE2
  typedef __m128i vec_t;
  typedef __m128i psqt_vec_t;
#define vec_load(a) (*(a))
#define vec_store(a,b) *(a)=(b)
#define vec_add_16(a,b) _mm_add_epi16(a,b)
#define vec_sub_16(a,b) _mm_sub_epi16(a,b)
#define vec_mul_16(a,b) _mm_mullo_epi16(a,b)
#define vec_zero() _mm_setzero_si128()
#define vec_set_16(a) _mm_set1_epi16(a)
#define vec_max_16(a,b) _mm_max_epi16(a,b)
#define vec_min_16(a,b) _mm_min_epi16(a,b)
#define vec_msb_pack_16(a,b) _mm_packs_epi16(_mm_srli_epi16(a,7),_mm_srli_epi16(b,7))
#define vec_load_psqt(a) (*(a))
#define vec_store_psqt(a,b) *(a)=(b)
#define vec_add_psqt_32(a,b) _mm_add_epi32(a,b)
#define vec_sub_psqt_32(a,b) _mm_sub_epi32(a,b)
#define vec_zero_psqt() _mm_setzero_si128()
#define NumRegistersSIMD (is64Bit ? 16 : 8)
#define MaxChunkSize 16
#elif USE_MMX
  typedef __m64 vec_t;
  typedef __m64 psqt_vec_t;
#define vec_load(a) (*(a))
#define vec_store(a,b) *(a)=(b)
#define vec_add_16(a,b) _mm_add_pi16(a,b)
#define vec_sub_16(a,b) _mm_sub_pi16(a,b)
#define vec_mul_16(a,b) _mm_mullo_pi16(a,b)
#define vec_zero() _mm_setzero_si64()
#define vec_set_16(a) _mm_set1_pi16(a)
  inline vec_t vec_max_16(vec_t a, vec_t b){
    vec_t comparison=_mm_cmpgt_pi16(a,b);
    return _mm_or_si64(_mm_and_si64(comparison,a),_mm_andnot_si64(comparison,b));
  }
  inline vec_t vec_min_16(vec_t a, vec_t b){
    vec_t comparison=_mm_cmpgt_pi16(a,b);
    return _mm_or_si64(_mm_and_si64(comparison,b),_mm_andnot_si64(comparison,a));
  }
#define vec_msb_pack_16(a,b) _mm_packs_pi16(_mm_srli_pi16(a,7),_mm_srli_pi16(b,7))
#define vec_load_psqt(a) (*(a))
#define vec_store_psqt(a,b) *(a)=(b)
#define vec_add_psqt_32(a,b) _mm_add_pi32(a,b)
#define vec_sub_psqt_32(a,b) _mm_sub_pi32(a,b)
#define vec_zero_psqt() _mm_setzero_si64()
#define vec_cleanup() _mm_empty()
#define NumRegistersSIMD 8
#define MaxChunkSize 8
#elif USE_NEON
  typedef int16x8_t vec_t;
  typedef int32x4_t psqt_vec_t;
#define vec_load(a) (*(a))
#define vec_store(a,b) *(a)=(b)
#define vec_add_16(a,b) vaddq_s16(a,b)
#define vec_sub_16(a,b) vsubq_s16(a,b)
#define vec_mul_16(a,b) vmulq_s16(a,b)
#define vec_zero() vec_t{0}
#define vec_set_16(a) vdupq_n_s16(a)
#define vec_max_16(a,b) vmaxq_s16(a,b)
#define vec_min_16(a,b) vminq_s16(a,b)
  inline vec_t vec_msb_pack_16(vec_t a, vec_t b){
    const int8x8_t shifta=vshrn_n_s16(a,7);
    const int8x8_t shiftb=vshrn_n_s16(b,7);
    const int8x16_t compacted=vcombine_s8(shifta,shiftb);
    return *reinterpret_cast<const vec_t*>(&compacted);
  }
#define vec_load_psqt(a) (*(a))
#define vec_store_psqt(a,b) *(a)=(b)
#define vec_add_psqt_32(a,b) vaddq_s32(a,b)
#define vec_sub_psqt_32(a,b) vsubq_s32(a,b)
#define vec_zero_psqt() psqt_vec_t{0}
#define NumRegistersSIMD 16
#define MaxChunkSize 16
#else
#undef VECTOR
#endif
#ifdef VECTOR
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#endif
  template <typename SIMDRegisterType,
    typename LaneType,
    int NumLanes,
    int MaxRegisters>
  static constexpr int BestRegisterCount(){
#define RegisterSize  sizeof(SIMDRegisterType)
#define LaneSize      sizeof(LaneType)
    const int ideal=(NumLanes*LaneSize)/RegisterSize;
    if (ideal<=MaxRegisters)
      return ideal;
    for (int divisor=MaxRegisters; divisor>1; --divisor)
      if (ideal%divisor==0)
        return divisor;
    return 1;
  }
  static constexpr int NumRegs=BestRegisterCount<vec_t, WeightType, transformedFeatureDimensions, NumRegistersSIMD>();
  static constexpr int NumPsqtRegs=BestRegisterCount<psqt_vec_t, PsqtWeightType, psqtBuckets, NumRegistersSIMD>();
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif
  class FeatureTransformer{
    static constexpr IndexType HalfDimensions=transformedFeatureDimensions;
#ifdef VECTOR
    static constexpr IndexType TileHeight=NumRegs*sizeof(vec_t)/2;
    static constexpr IndexType PsqtTileHeight=NumPsqtRegs*sizeof(psqt_vec_t)/4;
#endif
  public:
    using OutputType = TransformedFeatureType;
    static constexpr IndexType InputDimensions=FeatureSet::Dimensions;
    static constexpr IndexType OutputDimensions=HalfDimensions;
    static constexpr std::size_t BufferSize=
      OutputDimensions*sizeof(OutputType);
    FeatureTransformer() = default;
    static constexpr std::uint32_t getHashValue(){ return FeatureSet::HashValue^OutputDimensions*2; }

    bool readParameters(std::istream& stream){
      readLittleEndian<BiasType>(stream,biases,HalfDimensions);
      readLittleEndian<WeightType>(stream,weights,static_cast<size_t>(HalfDimensions)*InputDimensions);
      readLittleEndian<PsqtWeightType>(stream,psqtWeights,static_cast<size_t>(psqtBuckets)*InputDimensions);
      return !stream.fail();
    }

    bool writeParameters(std::ostream& stream) const{
      writeLittleEndian<BiasType>(stream,biases,HalfDimensions);
      writeLittleEndian<WeightType>(stream,weights,static_cast<size_t>(HalfDimensions)*InputDimensions);
      writeLittleEndian<PsqtWeightType>(stream,psqtWeights,static_cast<size_t>(psqtBuckets)*InputDimensions);
      return !stream.fail();
    }

    std::int32_t transform(const Position& pos, OutputType* output, const int bucket) const{
      updateAccumulator(pos,WHITE);
      updateAccumulator(pos,BLACK);
      const Color perspectives[2]={pos.stm(),~pos.stm()};
      const auto& accumulation=pos.state()->accumulator.accumulation;
      const auto& psqtAccumulation=pos.state()->accumulator.psqtAccumulation;
      const auto psqt=(
        psqtAccumulation[perspectives[0]][bucket]
        -psqtAccumulation[perspectives[1]][bucket]
      )/2;
      for (IndexType p=0; p<2; ++p){
        const IndexType offset=HalfDimensions/2*p;
#ifdef VECTOR
        constexpr IndexType OutputChunkSize=MaxChunkSize;
        constexpr IndexType NumOutputChunks=HalfDimensions/2/OutputChunkSize;
        vec_t Zero=vec_zero();
        vec_t One=vec_set_16(127);
        const vec_t* in0=reinterpret_cast<const vec_t*>(&(accumulation[perspectives[p]][0]));
        const vec_t* in1=reinterpret_cast<const vec_t*>(&(accumulation[perspectives[p]][HalfDimensions/2]));
        vec_t* out=reinterpret_cast<vec_t*>(output+offset);
        for (IndexType j=0; j<NumOutputChunks; j+=1){
          const vec_t sum0a=vec_max_16(vec_min_16(in0[j*2+0],One),Zero);
          const vec_t sum0b=vec_max_16(vec_min_16(in0[j*2+1],One),Zero);
          const vec_t sum1a=vec_max_16(vec_min_16(in1[j*2+0],One),Zero);
          const vec_t sum1b=vec_max_16(vec_min_16(in1[j*2+1],One),Zero);
          const vec_t pa=vec_mul_16(sum0a,sum1a);
          const vec_t pb=vec_mul_16(sum0b,sum1b);
          out[j]=vec_msb_pack_16(pa,pb);
        }
#else
        for (IndexType j=0; j<HalfDimensions/2; ++j){
          BiasType sum0=accumulation[static_cast<int>(perspectives[p])][j+0];
          BiasType sum1=accumulation[static_cast<int>(perspectives[p])][j+HalfDimensions/2];
          sum0=std::max<short>(0,std::min<short>(127,sum0));
          sum1=std::max<short>(0,std::min<short>(127,sum1));
          output[offset+j]=static_cast<OutputType>(sum0*sum1/128);
        }
#endif
      }
#ifdef vec_cleanup
      vec_cleanup();
#endif
      return psqt;
    }
  private:
    void updateAccumulator(const Position& pos, const Color perspective) const{
#ifdef VECTOR
      vec_t acc[NumRegs];
      psqt_vec_t psqt[NumPsqtRegs];
#endif
      StateInfo *st=pos.state(), *next=nullptr;
      int gain=FeatureSet::refreshCost(pos);
      while (st->previous&&!st->accumulator.computed[perspective]){
        if (FeatureSet::requiresRefresh(st,perspective)
          ||(gain-=FeatureSet::updateCost(st)+1)<0)
          break;
        next=st;
        st=st->previous;
      }
      if (st->accumulator.computed[perspective]){
        if (next==nullptr)
          return;
        const Square ksq=pos.square<KING>(perspective);
        FeatureSet::IndexList removed[2], added[2];
        FeatureSet::appendChangedIndices(
          ksq,next->dirtyPiece,perspective,removed[0],added[0]);
        for (const StateInfo* st2=pos.state(); st2!=next; st2=st2->previous)
          FeatureSet::appendChangedIndices(
            ksq,st2->dirtyPiece,perspective,removed[1],added[1]);
        next->accumulator.computed[perspective]=true;
        pos.state()->accumulator.computed[perspective]=true;
        StateInfo* statesToUpdate[3]=
          {next,next==pos.state()?nullptr:pos.state(),nullptr};
#ifdef VECTOR
        for (IndexType j=0; j<HalfDimensions/TileHeight; ++j){
          auto accTile=reinterpret_cast<vec_t*>(
            &st->accumulator.accumulation[perspective][j*TileHeight]);
          for (IndexType k=0; k<NumRegs; ++k)
            acc[k]=vec_load(&accTile[k]);
          for (IndexType i=0; statesToUpdate[i]; ++i){
            for (const auto index : removed[i]){
              const IndexType offset=HalfDimensions*index+j*TileHeight;
              auto column=reinterpret_cast<const vec_t*>(&weights[offset]);
              for (IndexType k=0; k<NumRegs; ++k)
                acc[k]=vec_sub_16(acc[k],column[k]);
            }
            for (const auto index : added[i]){
              const IndexType offset=HalfDimensions*index+j*TileHeight;
              auto column=reinterpret_cast<const vec_t*>(&weights[offset]);
              for (IndexType k=0; k<NumRegs; ++k)
                acc[k]=vec_add_16(acc[k],column[k]);
            }
            accTile=reinterpret_cast<vec_t*>(
              &statesToUpdate[i]->accumulator.accumulation[perspective][j*TileHeight]);
            for (IndexType k=0; k<NumRegs; ++k)
              vec_store(&accTile[k],acc[k]);
          }
        }
        for (IndexType j=0; j<psqtBuckets/PsqtTileHeight; ++j){
          auto accTilePsqt=reinterpret_cast<psqt_vec_t*>(
            &st->accumulator.psqtAccumulation[perspective][j*PsqtTileHeight]);
          for (std::size_t k=0; k<NumPsqtRegs; ++k)
            psqt[k]=vec_load_psqt(&accTilePsqt[k]);
          for (IndexType i=0; statesToUpdate[i]; ++i){
            for (const auto index : removed[i]){
              const IndexType offset=psqtBuckets*index+j*PsqtTileHeight;
              auto columnPsqt=reinterpret_cast<const psqt_vec_t*>(&psqtWeights[offset]);
              for (std::size_t k=0; k<NumPsqtRegs; ++k)
                psqt[k]=vec_sub_psqt_32(psqt[k],columnPsqt[k]);
            }
            for (const auto index : added[i]){
              const IndexType offset=psqtBuckets*index+j*PsqtTileHeight;
              auto columnPsqt=reinterpret_cast<const psqt_vec_t*>(&psqtWeights[offset]);
              for (std::size_t k=0; k<NumPsqtRegs; ++k)
                psqt[k]=vec_add_psqt_32(psqt[k],columnPsqt[k]);
            }
            accTilePsqt=reinterpret_cast<psqt_vec_t*>(
              &statesToUpdate[i]->accumulator.psqtAccumulation[perspective][j*PsqtTileHeight]);
            for (std::size_t k=0; k<NumPsqtRegs; ++k)
              vec_store_psqt(&accTilePsqt[k],psqt[k]);
          }
        }
#else
        for (IndexType i=0; statesToUpdate[i]; ++i){
          std::memcpy(statesToUpdate[i]->accumulator.accumulation[perspective],
            st->accumulator.accumulation[perspective],
            HalfDimensions*sizeof(BiasType));
          for (std::size_t k=0; k<psqtBuckets; ++k)
            statesToUpdate[i]->accumulator.psqtAccumulation[perspective][k]=st->accumulator.psqtAccumulation[
              perspective][k];
          st=statesToUpdate[i];
          for (const auto index : removed[i]){
            const IndexType offset=HalfDimensions*index;
            for (IndexType j=0; j<HalfDimensions; ++j)
              st->accumulator.accumulation[perspective][j]=
                static_cast<std::int16_t>(st->accumulator.accumulation[perspective][j]-
                  static_cast<std::int16_t>(weights[offset+j]));

            for (std::size_t k=0; k<psqtBuckets; ++k)
              st->accumulator.psqtAccumulation[perspective][k]-=psqtWeights[static_cast<unsigned long long>(index)*
                psqtBuckets+k];
          }
          for (const auto index : added[i]){
            const IndexType offset=HalfDimensions*index;
            for (IndexType j=0; j<HalfDimensions; ++j)
              st->accumulator.accumulation[perspective][j]=
                static_cast<std::int16_t>(st->accumulator.accumulation[perspective][j]+
                  static_cast<std::int16_t>(weights[offset+j]));

            for (std::size_t k=0; k<psqtBuckets; ++k)
              st->accumulator.psqtAccumulation[perspective][k]+=psqtWeights[static_cast<unsigned long long>(index)*
                psqtBuckets+k];
          }
        }
#endif
      }
      else{
        auto& [accumulation, psqtAccumulation, computed]=pos.state()->accumulator;
        computed[perspective]=true;
        FeatureSet::IndexList active;
        FeatureSet::appendActiveIndices(pos,perspective,active);
#ifdef VECTOR
        for (IndexType j=0; j<HalfDimensions/TileHeight; ++j){
          auto biasesTile=reinterpret_cast<const vec_t*>(
            &biases[j*TileHeight]);
          for (IndexType k=0; k<NumRegs; ++k)
            acc[k]=biasesTile[k];
          for (const auto index : active){
            const IndexType offset=HalfDimensions*index+j*TileHeight;
            auto column=reinterpret_cast<const vec_t*>(&weights[offset]);
            for (unsigned k=0; k<NumRegs; ++k)
              acc[k]=vec_add_16(acc[k],column[k]);
          }
          auto accTile=reinterpret_cast<vec_t*>(
            &accumulation[perspective][j*TileHeight]);
          for (unsigned k=0; k<NumRegs; k++)
            vec_store(&accTile[k],acc[k]);
        }
        for (IndexType j=0; j<psqtBuckets/PsqtTileHeight; ++j){
          for (std::size_t k=0; k<NumPsqtRegs; ++k)
            psqt[k]=vec_zero_psqt();
          for (const auto index : active){
            const IndexType offset=psqtBuckets*index+j*PsqtTileHeight;
            auto columnPsqt=reinterpret_cast<const psqt_vec_t*>(&psqtWeights[offset]);
            for (std::size_t k=0; k<NumPsqtRegs; ++k)
              psqt[k]=vec_add_psqt_32(psqt[k],columnPsqt[k]);
          }
          auto accTilePsqt=reinterpret_cast<psqt_vec_t*>(
            &psqtAccumulation[perspective][j*PsqtTileHeight]);
          for (std::size_t k=0; k<NumPsqtRegs; ++k)
            vec_store_psqt(&accTilePsqt[k],psqt[k]);
        }
#else
        std::memcpy(accumulation[perspective],biases,
          HalfDimensions*sizeof(BiasType));
        for (std::size_t k=0; k<psqtBuckets; ++k)
          psqtAccumulation[perspective][k]=0;
        for (const auto index : active){
          const IndexType offset=HalfDimensions*index;
          for (IndexType j=0; j<HalfDimensions; ++j)
            accumulation[perspective][j]=
              static_cast<std::int16_t>(accumulation[perspective][j]+
                static_cast<std::int16_t>(weights[offset+j]));
          for (std::size_t k=0; k<psqtBuckets; ++k)
            psqtAccumulation[perspective][k]+=psqtWeights[static_cast<unsigned long long>(index)*
              psqtBuckets+k];
        }
#endif
      }
#ifdef USE_MMX
      _mm_empty();
#endif
    }

    alignas(cacheLineSize) BiasType biases[HalfDimensions];
    alignas(cacheLineSize) WeightType weights[HalfDimensions*InputDimensions];
    alignas(cacheLineSize) PsqtWeightType psqtWeights[InputDimensions*psqtBuckets];
  };
}
