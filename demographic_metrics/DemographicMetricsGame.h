#pragma once

#include <sys/types.h>
#include <type_traits>
#include "fbpcf/frontend/mpcGame.h"
#include <tuple>

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

    // Returns the average age of the two databases
    float demographicMetricsAverage(
        const DemographicInfo& aliceDatabase,
        const DemographicInfo& bobDatabase);

    // Returns the average age of the two databases
    float demographicMetricsAverageSecretShared(
        const DemographicInfo& aliceDatabase,
        const DemographicInfo& bobDatabase);

    // Mutates the databases to remove invalid entries
    int demographicMetricsValidate(
        DemographicInfo& aliceDatabase,
        DemographicInfo& bobDatabase);

    // Returns multiplication of two secret shared values
    SecUnsignedInt mul(
        const SecUnsignedInt& self,
        const SecUnsignedInt& other);

    float demographicMetricsVariance(
        const DemographicInfo& aliceDatabase,
        const DemographicInfo& bobDatabase,
        const float mean = 0);

    // Returns the aggregated sum of rows in the database (or ints in the batch in general)
    // reveals to the parties their corresponding shares of values in each row
    // the parties then sum together their corresponding shares
    // and then those sums are added together by one party
    long unsigned int aggregateBatch(
        const SecUnsignedInt& inputBatch);

    // Returns the histogram of the two databases
    std::vector<long unsigned int> demographicMetricsHistogram(
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
