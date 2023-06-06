#pragma once

#include <sys/types.h>
#include <type_traits>
#include "fbpcf/frontend/mpcGame.h"

namespace fbpcf::input_validation {

template <int schedulerId>
class InputValidationGame : public frontend::MpcGame<schedulerId> {
  using SecUnsignedInt = typename frontend::MpcGame<
      schedulerId>::template SecUnsignedInt<32, true>;

 public:
  explicit InputValidationGame(
      std::unique_ptr<scheduler::IScheduler> scheduler)
      : frontend::MpcGame<schedulerId>(std::move(scheduler)) {}

  struct inputData {
    std::vector<uint32_t> inputShare;
  };

  std::vector<unsigned long int> inputValidate(
      const inputData& aliceDatabase,
      const inputData& bobDatabase);

 private:
  class SecInputData {
   public:
    SecInputData(const inputData& database, int partyId);
    SecUnsignedInt inputShare;
  };
};

} // namespace fbpcf::input_validation

#include "./InputValidationGame_impl.h"
