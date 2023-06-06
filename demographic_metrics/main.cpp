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

#include "./DemographicMetricsGame.h"
#include "fbpcf/engine/communication/SocketPartyCommunicationAgentFactory.h"
#include "fbpcf/util/MetricCollector.h"

DEFINE_int32(party, 0, "my party ID");
DEFINE_string(server_ip, "127.0.0.1", "server's ip address");
DEFINE_int32(port, 5000, "server port number");
DEFINE_int32(size, 123, "database size");

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  XLOGF(INFO, "party: {}", FLAGS_party);
  XLOGF(INFO, "server IP: {}", FLAGS_server_ip);
  XLOGF(INFO, "port: {}", FLAGS_port);
  XLOGF(INFO, "size: {}", FLAGS_size);

  auto size = FLAGS_size;

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
      fbpcf::demographic_metrics::DemographicMetricsGame<1>>(
      fbpcf::scheduler::getLazySchedulerFactoryWithRealEngine(
          FLAGS_party, *factory, metricCollector)
          ->create());

  typename fbpcf::demographic_metrics::DemographicMetricsGame<1>::
      DemographicInfo myInfo = {
          .ageShare = std::vector<uint32_t>(size),
          .genderShare = std::vector<bool>(size),
          .wealthShare = std::vector<uint32_t>(size),
      };

  typename fbpcf::demographic_metrics::DemographicMetricsGame<1>::
      DemographicInfo dummyInfo = {
          .ageShare = std::vector<uint32_t>(size),
          .genderShare = std::vector<bool>(size),
          .wealthShare = std::vector<uint32_t>(size),
      };

  std::random_device rd;
  std::mt19937_64 e(rd());
  std::uniform_int_distribution<uint32_t> dist1(0, 100/2);
  std::uniform_int_distribution<uint32_t> dist2(0, 250000);

  for (auto& item : myInfo.ageShare) {
    item = dist1(e);
  }
  for (auto item : myInfo.genderShare) { 
    item = (bool) (rand() % 2);
  }
  for (auto& item : myInfo.wealthShare) {
    item = dist2(e);
  }

  auto sum = 0;
    for (uint32_t i: myInfo.ageShare)
     sum += i;
      
  XLOG(INFO, "My shares: ", sum);

  try {
    auto mpcResult = FLAGS_party == 0
        ? game->demographicMetricsSum(myInfo, dummyInfo)
        : game->demographicMetricsSum(dummyInfo, myInfo);
    XLOG(INFO, "MPC result: ", mpcResult);
    // for (uint32_t i: mpcResult)
    //   std::cout << i << ", ";
    // std::cout << std::endl;
  } 
  catch (...) {
    XLOG(FATAL, "Failed to execute the game!");
  }
  XLOG(INFO, "Game executed successfully!");
  XLOG(
      INFO,
      folly::toPrettyJson(factory->getMetricsCollector()->collectMetrics()));
}