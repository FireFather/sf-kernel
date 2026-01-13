#pragma once
#include "nnue_common.h"
#include "features/half_ka_v2_hm.h"
#include "layers/affine_transform.h"
#include "layers/clipped_relu.h"
#include "layers/sqr_clipped_relu.h"

namespace Nebula::Eval::Nnue{
  using FeatureSet = Features::HalfKAv2Hm;
  constexpr IndexType transformedFeatureDimensions=1024;
  constexpr IndexType psqtBuckets=8;
  constexpr IndexType layerStacks=8;

  struct Network{
    static constexpr int FC_0_OUTPUTS=15;
    static constexpr int FC_1_OUTPUTS=32;
    Layers::AffineTransform<transformedFeatureDimensions, FC_0_OUTPUTS+1> fc_0{};
    Layers::SqrClippedReLu<FC_0_OUTPUTS+1> ac_sqr_0{};
    Layers::ClippedReLu<FC_0_OUTPUTS+1> ac_0{};
    Layers::AffineTransform<FC_0_OUTPUTS*2, FC_1_OUTPUTS> fc_1{};
    Layers::ClippedReLu<FC_1_OUTPUTS> ac_1{};
    Layers::AffineTransform<FC_1_OUTPUTS, 1> fc_2{};

    static constexpr std::uint32_t getHashValue(){
      std::uint32_t hashValue=0xEC42E90Du;
      hashValue^=transformedFeatureDimensions*2;
      hashValue=decltype(fc_0)::getHashValue(hashValue);
      hashValue=decltype(ac_0)::getHashValue(hashValue);
      hashValue=decltype(fc_1)::getHashValue(hashValue);
      hashValue=decltype(ac_1)::getHashValue(hashValue);
      hashValue=decltype(fc_2)::getHashValue(hashValue);
      return hashValue;
    }

    bool readParameters(std::istream& stream){
      if (!fc_0.readParameters(stream)) return false;
      if (!Layers::ClippedReLu<FC_0_OUTPUTS+1>::readParameters(stream)) return false;
      if (!fc_1.readParameters(stream)) return false;
      if (!Layers::ClippedReLu<FC_1_OUTPUTS>::readParameters(stream)) return false;
      if (!fc_2.readParameters(stream)) return false;
      return true;
    }

    bool writeParameters(std::ostream& stream) const{
      if (!fc_0.writeParameters(stream)) return false;
      if (!Layers::ClippedReLu<FC_0_OUTPUTS+1>::writeParameters(stream)) return false;
      if (!fc_1.writeParameters(stream)) return false;
      if (!Layers::ClippedReLu<FC_1_OUTPUTS>::writeParameters(stream)) return false;
      if (!fc_2.writeParameters(stream)) return false;
      return true;
    }

    std::int32_t propagate(const TransformedFeatureType* transformedFeatures){
      struct alignas(cacheLineSize) Buffer{
        alignas(cacheLineSize) decltype(fc_0)::OutputBuffer fc_0_out;
        alignas(cacheLineSize) decltype(ac_sqr_0)::OutputType ac_sqr_0_out[ceilToMultiple<IndexType>(
          FC_0_OUTPUTS*2,32)];
        alignas(cacheLineSize) decltype(ac_0)::OutputBuffer ac_0_out;
        alignas(cacheLineSize) decltype(fc_1)::OutputBuffer fc_1_out;
        alignas(cacheLineSize) decltype(ac_1)::OutputBuffer ac_1_out;
        alignas(cacheLineSize) decltype(fc_2)::OutputBuffer fc_2_out;
        Buffer(){ std::memset(this,0,sizeof(*this)); }
      };

      alignas(cacheLineSize) thread_local Buffer buffer;

      fc_0.propagate(transformedFeatures,buffer.fc_0_out);
      ac_sqr_0.propagate(buffer.fc_0_out,buffer.ac_sqr_0_out);
      ac_0.propagate(buffer.fc_0_out,buffer.ac_0_out);
      std::memcpy(buffer.ac_sqr_0_out+FC_0_OUTPUTS,buffer.ac_0_out,
        FC_0_OUTPUTS*sizeof(decltype(ac_0)::OutputType));
      fc_1.propagate(buffer.ac_sqr_0_out,buffer.fc_1_out);
      ac_1.propagate(buffer.fc_1_out,buffer.ac_1_out);
      fc_2.propagate(buffer.ac_1_out,buffer.fc_2_out);
      const std::int32_t fwdOut=buffer.fc_0_out[FC_0_OUTPUTS]*(600*outputScale)/(127*(1<<weightScaleBits));
      const std::int32_t outputValue=buffer.fc_2_out[0]+fwdOut;
      return outputValue;
    }
  };
}
