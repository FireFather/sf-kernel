#include <iomanip>
#include "../evaluate.h"
#include "../position.h"
#include "../misc.h"
#include "../types.h"
#include "evaluate_nnue.h"

namespace Nebula::Eval::Nnue{
  namespace{
    LargePagePtr<FeatureTransformer> featureTransformer;
    AlignedPtr<Network> network[layerStacks];
    std::string fileName;
    std::string netDescription;

    namespace Detail{
      template <typename T>
      void initialize(AlignedPtr<T>& pointer){
        void* mem=stdAlignedAlloc(alignof(T),sizeof(T));
        T* obj=new(mem) T();
        pointer.reset(obj);
      }

      template <typename T>
      void initialize(LargePagePtr<T>& pointer){
        pointer.reset(static_cast<T*>(alignedLargePagesAlloc(sizeof(T))));
        std::memset(pointer.get(),0,sizeof(T));
      }

      template <typename T>
      bool readParameters(std::istream& stream, T& reference){
        auto header=readLittleEndian<std::uint32_t>(stream);
        if (!stream||header!=T::getHashValue()) return false;
        return reference.readParameters(stream);
      }
    }

    void initialize(){
      Detail::initialize(featureTransformer);
      for (auto& i : network)
        Detail::initialize(i);
    }

    bool readHeader(std::istream& stream, std::uint32_t* value, std::string* desc){
      const auto version=readLittleEndian<std::uint32_t>(stream);
      *value=readLittleEndian<std::uint32_t>(stream);
      const auto size=readLittleEndian<std::uint32_t>(stream);
      if (!stream||version!=nnueVersion) return false;
      desc->resize(size);
      stream.read(desc->data(),size);
      return !stream.fail();
    }

    bool readParameters(std::istream& stream){
      std::uint32_t value;
      if (!readHeader(stream,&value,&netDescription)) return false;
      if (value!=hashValue) return false;
      if (!Detail::readParameters(stream,*featureTransformer)) return false;
      for (auto& i : network)
        if (!Detail::readParameters(stream,*i)) return false;
      return stream&&stream.peek()==std::ios::traits_type::eof();
    }
  }

  Value evaluate(const Position& pos, const bool adjusted, int* complexity){
    constexpr uint64_t alignment=cacheLineSize;
    const int delta=24-pos.nonPawnMaterial()/9560;
#ifdef ALIGNAS_ON_STACK_VARIABLES_BROKEN
    TransformedFeatureType transformedFeaturesUnaligned[
      FeatureTransformer::BufferSize+alignment/sizeof(TransformedFeatureType)];
    auto* transformedFeatures=alignPtrUp<alignment>(&transformedFeaturesUnaligned[0]);
#else
    alignas(alignment)
      TransformedFeatureType transformedFeatures[FeatureTransformer::BufferSize];
#endif
    const int bucket=(pos.count<ALL_PIECES>()-1)/4;
    const auto psqt=featureTransformer->transform(pos,transformedFeatures,bucket);
    const auto positional=network[bucket]->propagate(transformedFeatures);
    if (complexity)
      *complexity=abs(psqt-positional)/outputScale;
    if (adjusted)
      return static_cast<Value>(((1024-delta)*psqt+(1024+delta)*positional)/(1024*outputScale));
    return static_cast<Value>((psqt+positional)/outputScale);
  }

  bool loadEval(const std::string& name, std::istream& stream){
    initialize();
    fileName=name;
    return readParameters(stream);
  }
}
