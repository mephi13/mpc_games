/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <type_traits>
#include "./InputValidationGame.h"
#include "fbpcf/frontend/mpcGame.h"

namespace fbpcf::input_validation {

template <int schedulerId>
std::vector<unsigned long int>
InputValidationGame<schedulerId>::inputValidate(
    const inputData& aliceDatabase,
    const inputData& bobDatabase) {
  int alicePartyId = 0;
  int bobPartyId = 1;

  SecInputData secAliceDatabase(aliceDatabase, alicePartyId);
  SecInputData secBobDatabase(bobDatabase, bobPartyId);

  // the mpc function defined for the game

  auto secInput = secAliceDatabase.inputShare + secBobDatabase.inputShare ? (secAliceDatabase.inputShare + secBobDatabase.inputShare) < SecUnsignedInt(200) : SecUnsignedInt(0);

  auto result = secInput.revealToParty(alicePartyId)
  return result.getValue();
}

template <int schedulerId>
InputValidationGame<schedulerId>::SecInputData::SecInputData(
  const inputData& database, int partyId)
    : inputShare(database.inputShare, partyId) {}
} // namespace fbpcf::demographic_metrics