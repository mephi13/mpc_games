/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <folly/json.h>

#include <gflags/gflags.h>
#include "fbpcf/scheduler/LazySchedulerFactory.h"
#include "folly/init/Init.h"
#include "folly/logging/xlog.h"

#include "./InputValidationGame.h"
#include "fbpcf/engine/communication/SocketPartyCommunicationAgentFactory.h"
#include "fbpcf/util/MetricCollector.h"

DEFINE_int32(party, 0, "my party ID");
DEFINE_string(server_ip, "127.0.0.1", "server's ip address");
DEFINE_int32(port, 5000, "server port number");

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  XLOGF(INFO, "party: {}", FLAGS_party);
  XLOGF(INFO, "server IP: {}", FLAGS_server_ip);
  XLOGF(INFO, "port: {}", FLAGS_port);

  auto metricCollector =
      std::make_shared<fbpcf::util::MetricCollector>("demographic_metrics");

  fbpcf::engine::communication::SocketPartyCommunicationAgent::TlsInfo tlsInfo;
  tlsInfo.certPath = "";
  tlsInfo.keyPath = "";
  tlsInfo.passphrasePath = "";
  tlsInfo.useTls = false;

  std::map<
      int,
      fbpcf::engine::communication::SocketPartyCommunicationAgentFactory::
          PartyInfo>
      partyInfos(
          {{0, {FLAGS_server_ip, FLAGS_port}},
           {1, {FLAGS_server_ip, FLAGS_port}}});
  auto factory = std::make_unique<
      fbpcf::engine::communication::SocketPartyCommunicationAgentFactory>(
      FLAGS_party, partyInfos, tlsInfo, metricCollector);

  auto game = std::make_unique<
      fbpcf::input_validation::InputValidationGame<0>>(
      fbpcf::scheduler::getLazySchedulerFactoryWithRealEngine(
          FLAGS_party, *factory, metricCollector)
          ->create());

  const int size = 1024;

  typename fbpcf::input_validation::InputValidationGame<0>::
      inputData myInput = {
          .inputShare = std::vector<uint32_t>(size),
      };

  typename fbpcf::input_validation::InputValidationGame<0>::
      inputData dummyInput = {
          .inputShare = std::vector<uint32_t>(size),
      };

  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<uint32_t> dist1(0, 123);

  for (auto& item : myInput.inputShare) {
    item = dist1(e);
  }

  XLOG(INFO, "My shares:");
  for (uint32_t i: myInput.inputShare)
    std::cout << i << ", ";

  try {
    auto mpcResult = FLAGS_party == 0
        ? game->inputValidate(myInput, dummyInput)
        : game->inputValidate(dummyInput, myInput);
    XLOG(INFO, "MPC result: ");
    // std::cout << mpcResult << std::endl;
    for (uint32_t i: mpcResult)
       std::cout << i << ", ";
    std::cout << std::endl;
  } 
  catch (...) {
    XLOG(FATAL, "Failed to execute the game!");
  }
  XLOG(INFO, "Game executed successfully!");
  XLOG(
      INFO,
      folly::toPrettyJson(factory->getMetricsCollector()->collectMetrics()));
}
