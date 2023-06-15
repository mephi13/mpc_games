#pragma once

#include <type_traits>
#include "./DemographicMetricsGame.h"
#include "fbpcf/frontend/mpcGame.h"
#include <folly/Random.h>

namespace fbpcf::demographic_metrics {

template <int schedulerId>
float
DemographicMetricsGame<schedulerId>::demographicMetricsAverage(
    const DemographicInfo& aliceDatabase,
    const DemographicInfo& bobDatabase) {
  int alicePartyId = 0;
  int bobPartyId = 1;

  // the mpc function defined for the game

  // calculate the sums on transposed batch vector
  // cannot be done in parrallel 
  std::vector<SecUnsignedIntSingle> secAliceSum = {};
  std::vector<SecUnsignedIntSingle> secBobSum = {};

  for (size_t i = 0; i < aliceDatabase.ageShare.size(); ++i) {
    secAliceSum.push_back(SecUnsignedIntSingle(aliceDatabase.ageShare.at(i), alicePartyId));
  }
  for (size_t i = 0; i < bobDatabase.ageShare.size(); ++i) {
    secBobSum.push_back(SecUnsignedIntSingle(bobDatabase.ageShare.at(i), bobPartyId));
  }

  uint32_t zero = 0;
  auto secSum = SecUnsignedIntSingle(zero, alicePartyId);
  for (size_t i = 0; i < secBobSum.size(); ++i) {
    secSum = secSum + secBobSum.at(i) + secAliceSum.at(i);
  }
  
  auto pubAgeResult = secSum.openToParty(alicePartyId);
  XLOG(INFO) << "secSum: " << pubAgeResult.getValue();

  return pubAgeResult.getValue()/float(aliceDatabase.ageShare.size());
}

template <int schedulerId>
float
DemographicMetricsGame<schedulerId>::demographicMetricsAverageSecretShared(
    const DemographicInfo& aliceDatabase,
    const DemographicInfo& bobDatabase) {
  int alicePartyId = 0;
  int bobPartyId = 1;

  SecDemographicInfo secAliceDatabase(aliceDatabase, alicePartyId);
  SecDemographicInfo secBobDatabase(bobDatabase, bobPartyId);

  return aggregateBatch(secAliceDatabase.ageShare + secBobDatabase.ageShare)/float(aliceDatabase.ageShare.size());
}

template <int schedulerId>
typename DemographicMetricsGame<schedulerId>::SecUnsignedInt
DemographicMetricsGame<schedulerId>::mul(
    const SecUnsignedInt& self,
    const SecUnsignedInt& other) {

  auto zero = SecUnsignedInt(std::vector<uint32_t>(self.getBatchSize(), 0), 0);

  SecUnsignedInt rst = zero;
  SecUnsignedInt multiplicand = SecUnsignedInt(self);

  for (int8_t i = 0; i < 32; i++) {
    // 32 additions + 32 mux operations for multiplication
    rst = rst + zero.mux(other[i], multiplicand);

    // we need to shift left the multiplicand
    for (int8_t j = 31; j > 0; j--) {
      multiplicand[j] = multiplicand[j-1];
    }
    multiplicand[0] = fbpcf::frontend::Bit<true, schedulerId, true>(std::vector<bool>(self.getBatchSize(), 0), 0); // clear the last bit
  }

  return rst;
}

template <int schedulerId>
float
DemographicMetricsGame<schedulerId>::demographicMetricsVariance(
    const DemographicInfo& aliceDatabase,
    const DemographicInfo& bobDatabase,
    float mean 
    ) {
  int alicePartyId = 0;
  int bobPartyId = 1;

  SecDemographicInfo secAliceDatabase(aliceDatabase, alicePartyId);
  SecDemographicInfo secBobDatabase(bobDatabase, bobPartyId);

  auto secSum = secAliceDatabase.ageShare + secBobDatabase.ageShare;

  auto secDiff = secSum - SecUnsignedInt(std::vector<uint32_t>(secSum.getBatchSize(), mean), alicePartyId);
  auto secRes = mul(secDiff, secDiff);

  auto pubResSum = aggregateBatch(secRes);

  auto varianceEstimation = pubResSum/float(aliceDatabase.ageShare.size() - 1); // unbiased estimator

  XLOG(INFO) << "varianceEstimation: " << varianceEstimation;
  return varianceEstimation;
}

template <int schedulerId>
int DemographicMetricsGame<schedulerId>::demographicMetricsValidate(
    DemographicInfo& aliceDatabase,
    DemographicInfo& bobDatabase) {
  int alicePartyId = 0;
  int bobPartyId = 1;

  SecDemographicInfo secAliceDatabase(aliceDatabase, alicePartyId);
  SecDemographicInfo secBobDatabase(bobDatabase, bobPartyId);

  // New vector to store the valid entries
  std::vector<uint32_t> validAgeAlice = {};
  std::vector<uint32_t> validAgeBob = {};

  std::vector<bool> validGenderAlice = {};
  std::vector<bool> validGenderBob = {};

  std::vector<uint32_t> validWealthAlice = {};
  std::vector<uint32_t> validWealthBob = {};

  // input validation
  auto secAge = secAliceDatabase.ageShare + secBobDatabase.ageShare;

  // create a vector of bools (1 if row is valid, 0 otherwise)
  auto secValid = (secAge < SecUnsignedInt(std::vector<uint32_t>(aliceDatabase.ageShare.size(), 200), 0));
  // reveal the validity vector, only valid vals will be used in aggregation
  auto validA = secValid.openToParty(alicePartyId).getValue();
  auto validB = secValid.openToParty(bobPartyId).getValue();
  
  // should be symmetric for both parties (all 0 for valid vector of other party)
  for (size_t i = 0; i < validA.size(); ++i) {
    if(validA.at(i)){
      validAgeAlice.push_back(aliceDatabase.ageShare.at(i));
      validAgeBob.push_back(bobDatabase.ageShare.at(i));
      validGenderAlice.push_back(aliceDatabase.genderShare.at(i));
      validGenderBob.push_back(bobDatabase.genderShare.at(i));
      validWealthAlice.push_back(aliceDatabase.wealthShare.at(i));
      validWealthBob.push_back(bobDatabase.wealthShare.at(i));
    }
  }
  // we don't know which party are we, so we do it for both
  for (size_t i = 0; i < validB.size(); ++i) {
    if(validB.at(i)){
      validAgeAlice.push_back(aliceDatabase.ageShare.at(i));
      validAgeBob.push_back(bobDatabase.ageShare.at(i));
      validGenderAlice.push_back(aliceDatabase.genderShare.at(i));
      validGenderBob.push_back(bobDatabase.genderShare.at(i));
      validWealthAlice.push_back(aliceDatabase.wealthShare.at(i));
      validWealthBob.push_back(bobDatabase.wealthShare.at(i));
    }
  }

  aliceDatabase.ageShare = validAgeAlice;
  aliceDatabase.genderShare = validGenderAlice;
  aliceDatabase.wealthShare = validWealthAlice;

  bobDatabase.ageShare = validAgeBob;
  bobDatabase.genderShare = validGenderBob;
  bobDatabase.wealthShare = validWealthBob;

  // Return valid database size
  return validAgeAlice.size();
}

template<int schedulerId> 
long unsigned int DemographicMetricsGame<schedulerId>::aggregateBatch(
    const SecUnsignedInt& inputBatch){
  int alicePartyId = 0;
  int bobPartyId = 1;

  // create mask for bob
  auto masks = vector<uint32_t>();

  for(size_t i = 0; i < inputBatch.getBatchSize(); ++i){
    // basically doing calculations mod 2^32, so the mask 
    // is taken at random from this space
    masks.push_back(folly::Random::secureRand32());
  }
  auto secMasks = SecUnsignedInt(masks, bobPartyId);
  auto pubInputShares = (inputBatch - secMasks).openToParty(alicePartyId).getValue();

  // calculate the sum of masked shares
  uint32_t shareSum = 0;
  for (size_t i = 0; i < pubInputShares.size(); ++i) {
    shareSum += pubInputShares.at(i);
  }
  XLOG(DBG) << "shareSum: " << shareSum;

  uint32_t masksSum = 0;
  for (size_t i = 0; i < masks.size(); ++i) {
    masksSum += masks.at(i);
  }
  XLOG(DBG) << "masksSum: " << masksSum;

  // make the mask sum public
  auto maskSumPublic = SecUnsignedIntSingle(masksSum, bobPartyId).openToParty(alicePartyId).getValue();

  // calculate the sum
  uint32_t sum = shareSum + maskSumPublic;
  XLOG(DBG) << "sum: " << sum;
  return sum;
}

template<int schedulerId> 
std::vector<long unsigned int>
DemographicMetricsGame<schedulerId>::demographicMetricsHistogram(
    const DemographicInfo& aliceDatabase,
    const DemographicInfo& bobDatabase) {
  int alicePartyId = 0;
  int bobPartyId = 1;

  SecDemographicInfo secAliceDatabase(aliceDatabase, alicePartyId);
  SecDemographicInfo secBobDatabase(bobDatabase, bobPartyId);

  std::vector<uint32_t> validAgeAlice = aliceDatabase.ageShare;
  std::vector<uint32_t> validAgeBob = bobDatabase.ageShare;

  // the mpc function defined for the game
  std::vector<SecUnsignedInt> secAliceHistogram;

  // histogram bin boundaries
  std::vector<uint32_t> x = {25, 40, 50, 60, 75};

  // probably can be done in a more optimized way
  auto secAge = secAliceDatabase.ageShare + secBobDatabase.ageShare;

  auto zero = SecUnsignedInt(std::vector<uint32_t>(validAgeAlice.size(), 0), alicePartyId);
  auto one = SecUnsignedInt(std::vector<uint32_t>(validAgeAlice.size(), 1), alicePartyId);

  // calculate histogram vectors, they will have to be aggregated later
  secAliceHistogram.push_back(zero.mux(secAge < SecUnsignedInt(std::vector<uint32_t>(validAgeAlice.size(), x[0]), alicePartyId), one));
  for (long unsigned int i = 1; i < x.size(); ++i) {
    auto binStart = SecUnsignedInt(std::vector<uint32_t>(validAgeAlice.size(), x[i-1]), alicePartyId);
    auto binEnd = SecUnsignedInt(std::vector<uint32_t>(validAgeAlice.size(), x[i]), alicePartyId);
    secAliceHistogram.push_back(zero.mux((secAge >= binStart) & (secAge < binEnd), one));
  }
  secAliceHistogram.push_back(zero.mux(secAge >= SecUnsignedInt(std::vector<uint32_t>(validAgeAlice.size(), x[x.size() - 1]), alicePartyId), one));

  std::vector<long unsigned int> pubAliceHistogram;

  // probably can be done much faster
  for(long unsigned int i = 0; i < secAliceHistogram.size(); ++i){
    pubAliceHistogram.push_back(aggregateBatch(secAliceHistogram.at(i)));
    XLOG(INFO) << "pubAliceHistogram[" << i << "]: " << pubAliceHistogram[i];
  }
  return pubAliceHistogram;
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