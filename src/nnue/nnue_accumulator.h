#pragma once
#include "nnue_architecture.h"

namespace Nebula::Eval::Nnue{
  struct alignas(cacheLineSize) Accumulator{
    std::int16_t accumulation[2][transformedFeatureDimensions];
    std::int32_t psqtAccumulation[2][psqtBuckets];
    bool computed[2];
  };
}
