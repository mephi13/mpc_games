#pragma once

#include "fbpcf/frontend/mpcGame.h"

namespace fbpcf::demographic_metrics {

const int PLAYER0 = 0;
const int PLAYER1 = 1;

const size_t maxStringLength = 15;
const size_t charLength = 8;
const size_t int32Length = 32;

template <int schedulerId, bool usingBatch = true>
using SecBool =
    typename frontend::MpcGame<schedulerId>::template SecBit<usingBatch>;

template <int schedulerId, bool usingBatch = true>
using PubBool =
    typename frontend::MpcGame<schedulerId>::template PubBit<usingBatch>;

template <int schedulerId, bool usingBatch = true>
using PubUnsignedInt = typename frontend::MpcGame<
    schedulerId>::template PubUnsignedInt<int32Length, usingBatch>;

template <int schedulerId, bool usingBatch = true>
using SecUnsignedInt = typename frontend::MpcGame<
    schedulerId>::template SecUnsignedInt<int32Length, usingBatch>;

} // namespace fbpcf::edit_distance
