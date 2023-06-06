/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <type_traits>
#include "./DemographicMetricsGame.h"
#include "fbpcf/frontend/mpcGame.h"

namespace fbpcf::demographic_metrics {

template <int schedulerId>
long unsigned int
DemographicMetricsGame<schedulerId>::demographicMetricsSum(
    const DemographicInfo& aliceDatabase,
    const DemographicInfo& bobDatabase) {
  int alicePartyId = 0;
  int bobPartyId = 1;

  SecDemographicInfo secAliceDatabase(aliceDatabase, alicePartyId);
  SecDemographicInfo secBobDatabase(bobDatabase, bobPartyId);

  // the mpc function defined for the game

  // input validation
  auto secAge = secAliceDatabase.ageShare + secBobDatabase.ageShare;
  
  // create a vector of bools (1 if row is valid, 0 otherwise)
  auto secValid = (secAge < SecUnsignedInt(std::vector<uint32_t>(aliceDatabase.ageShare.size(), 80), 0));

  // calculate the sums on transposed batch vector
  // cannot be done in parrallel 
  std::vector<SecUnsignedIntSingle> secAliceSum = {};
  std::vector<SecUnsignedIntSingle> secBobSum = {};

  // reveal the validity vector, only valid vals will be used in aggregation
  auto validA = secValid.openToParty(alicePartyId).getValue();
  auto validB = secValid.openToParty(bobPartyId).getValue();

  std::vector<uint32_t> validAlice = {};
  std::vector<uint32_t> validBob = {};
  
  // should be symmetric for both parties (all 0 for valid vector of other party)
  for (size_t i = 0; i < validA.size(); ++i) {
    if(validA.at(i)){
      validAlice.push_back(aliceDatabase.ageShare.at(i));
      validBob.push_back(bobDatabase.ageShare.at(i));
    }
  }
  for (size_t i = 0; i < validB.size(); ++i) {
    if(validB.at(i)){
      validAlice.push_back(aliceDatabase.ageShare.at(i));
      validBob.push_back(bobDatabase.ageShare.at(i));
    }
  }

  for (size_t i = 0; i < validAlice.size(); ++i) {
    secAliceSum.push_back(SecUnsignedIntSingle(aliceDatabase.ageShare.at(i), alicePartyId));
  }
  for (size_t i = 0; i < validBob.size(); ++i) {
    secBobSum.push_back(SecUnsignedIntSingle(bobDatabase.ageShare.at(i), bobPartyId));
  }

  uint32_t zero = 0;
  auto secSum = SecUnsignedIntSingle(zero, alicePartyId);
  for (size_t i = 0; i < secBobSum.size(); ++i) {
    secSum = secSum + secBobSum.at(i) + secAliceSum.at(i);
  }
  
  // auto secGender = secAliceDatabase.genderShare ^ secBobDatabase.genderShare;
  // auto secWealth = secAliceDatabase.wealthShare + secBobDatabase.wealthShare;

  auto pubAgeResult = secSum.openToParty(alicePartyId);
  return pubAgeResult.getValue();
}

template <int schedulerId>
DemographicMetricsGame<schedulerId>::SecDemographicInfo::SecDemographicInfo(
  const DemographicInfo& database, int partyId)
    : ageShare(database.ageShare, partyId),
      genderShare(database.genderShare, partyId),
      wealthShare(database.wealthShare, partyId) {}

template <int schedulerId, bool usingBatch = true>
using PubBit =
    typename fbpcf::frontend::MpcGame<schedulerId>::template PubBit<usingBatch>;

template <int schedulerId, bool usingBatch = true>
using SecBit =
    typename fbpcf::frontend::MpcGame<schedulerId>::template SecBit<usingBatch>;


} // namespace fbpcf::demographic_metrics