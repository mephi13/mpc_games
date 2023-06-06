#pragma once

#include <sys/types.h>
#include <type_traits>
#include "fbpcf/frontend/mpcGame.h"

namespace fbpcf::demographic_metrics {

template <int schedulerId>
class DemographicMetricsGame : public frontend::MpcGame<schedulerId> {
  using SecUnsignedInt = typename frontend::MpcGame<
      schedulerId>::template SecUnsignedInt<32, true>;
  using SecBool = typename frontend::MpcGame<
      schedulerId>::template SecBit<true>;
  using SecUnsignedIntSingle = typename frontend::MpcGame<
      schedulerId>::template SecUnsignedInt<32, false>;
  using PubValue = typename fbpcf::frontend::MpcGame<
      schedulerId>::template PubUnsignedInt<32, false>;


 public:
  explicit DemographicMetricsGame(
      std::unique_ptr<scheduler::IScheduler> scheduler)
      : frontend::MpcGame<schedulerId>(std::move(scheduler)) {}

  struct DemographicInfo {
    std::vector<uint32_t> ageShare;
    std::vector<bool> genderShare;
    std::vector<uint32_t> wealthShare;
  };

  long unsigned int demographicMetricsSum(
      const DemographicInfo& aliceDatabase,
      const DemographicInfo& bobDatabase);

 private:
  class SecDemographicInfo {
   public:
    SecDemographicInfo(const DemographicInfo& database, int partyId);
    SecUnsignedInt ageShare;
    SecBool genderShare;
    SecUnsignedInt wealthShare;
  };
};

} // namespace fbpcf::demographic_metrics

#include "./DemographicMetricsGame_impl.h"
