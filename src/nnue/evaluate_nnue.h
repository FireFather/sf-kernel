#pragma once
#include "nnue_feature_transformer.h"
#include <memory>

namespace Nebula::Eval::Nnue{
  constexpr std::uint32_t hashValue=
    FeatureTransformer::getHashValue()^Network::getHashValue();

  template <typename T>
  struct AlignedDeleter{
    void operator()(T* ptr) const{
      ptr->~T();
      stdAlignedFree(ptr);
    }
  };

  template <typename T>
  struct LargePageDeleter{
    void operator()(T* ptr) const{
      ptr->~T();
      alignedLargePagesFree(ptr);
    }
  };

  template <typename T>
  using AlignedPtr = std::unique_ptr<T, AlignedDeleter<T>>;
  template <typename T>
  using LargePagePtr = std::unique_ptr<T, LargePageDeleter<T>>;
}
