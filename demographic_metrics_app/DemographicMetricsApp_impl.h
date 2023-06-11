/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <fbpcf/io/api/FileIOWrappers.h>
#include <fbpcf/scheduler/LazySchedulerFactory.h>
#include <fbpcf/scheduler/NetworkPlaintextSchedulerFactory.h>
#include <vector>

#include "./DemographicMetricsApp.h"
#include "./Csv.h"

namespace fbpcf::demographic_metrics {

template <int schedulerId>
void DemographicMetricsApp<schedulerId>::run() {
  std::unique_ptr<fbpcf::scheduler::IScheduler> scheduler = fbpcf::scheduler::getLazySchedulerFactoryWithRealEngine(
            party_, *communicationAgentFactory_, metricCollector_)
            ->create();

  auto game = DemographicMetricsGame<schedulerId>(std::move(scheduler));

  XLOG(INFO, "Scheduler created successfully");

  for (size_t i = startFileIndex_; i < startFileIndex_ + numbFiles_; ++i) {
    try {
      CHECK_LT(i, inputPaths_.size()) << "File index exceeds number of files.";
      std::string output;
      auto myInput = getInputData(inputPaths_.at(i));

      auto numRows = myInput.ageShare.size();
      XLOG(INFO) << "Have " << numRows << " values in inputData.";

      DemographicInfo dummyInput = {
          .ageShare = std::vector<uint32_t>(numRows),
          .genderShare = std::vector<bool>(numRows),
          .wealthShare = std::vector<uint32_t>(numRows),
      };

      auto validateResult = party_ == 0
          ? game.demographicMetricsValidate(myInput, dummyInput)
          : game.demographicMetricsValidate(dummyInput, myInput);

      XLOG(INFO) << "done calculating";

      putOutputData(std::to_string(validateResult), outputPaths_.at(i));
    } catch (const std::exception& e) {
      XLOGF(
          ERR,
          "Error: Exception caught in CalculatorApp run.\n \t error msg: {} \n \t input shard: {}.",
          e.what(),
          inputPaths_.at(i));
      std::exit(1);
    }

    auto gateStatistics =
          fbpcf::scheduler::SchedulerKeeper<schedulerId>::getGateStatistics();
      XLOGF(
          INFO,
          "Non-free gate count = {}, Free gate count = {}",
          gateStatistics.first,
          gateStatistics.second);

      auto trafficStatistics =
          fbpcf::scheduler::SchedulerKeeper<schedulerId>::getTrafficStatistics();
      XLOGF(
          INFO,
          "Sent network traffic = {}, Received network traffic = {}",
          trafficStatistics.first,
          trafficStatistics.second);

      schedulerStatistics_.nonFreeGates = gateStatistics.first;
      schedulerStatistics_.freeGates = gateStatistics.second;
      schedulerStatistics_.sentNetwork = trafficStatistics.first;
      schedulerStatistics_.receivedNetwork = trafficStatistics.second;
      fbpcf::scheduler::SchedulerKeeper<schedulerId>::deleteEngine();
      schedulerStatistics_.details = metricCollector_->collectMetrics();
      }
}

template <int schedulerId>
void DemographicMetricsApp<schedulerId>::addFromCSV(
    const std::vector<std::string>& header,
    const std::vector<std::string>& parts,
    DemographicInfo& demographicInfo) {
  std::vector<std::string> featureValues;

  uint32_t ageValue;
  bool genderValue;
  uint32_t wealthValue;

  for (std::size_t i = 0; i < header.size(); ++i) {
    auto column = header[i];
    auto value = parts[i];
    uint32_t parsed = 0;
    std::istringstream iss{value};

    // Array columns and features may be parsed differently
    if ((column == "age" || column == "wealth" || column == "id_" || column == "gender")) {
      iss >> parsed;

      if (iss.fail()) {
        XLOG(FATAL) << "Failed to parse '" << iss.str() << "' to uint32_t";
      }
    }

    if (column == "age") {
      ageValue = (parsed);
    } else if (column == "gender") {
      genderValue = (parsed);
    } else if (column == "wealth") {
      wealthValue = (parsed);
    } else if (column != "id_") {
      // We shouldn't fail if there are extra columns in the input
      XLOG(WARNING) << "Warning: Unknown column in csv: " << column;
    }
  }

  demographicInfo.ageShare.push_back(ageValue);
  demographicInfo.genderShare.push_back(genderValue);
  demographicInfo.wealthShare.push_back(wealthValue);
}

template <int schedulerId>
typename DemographicMetricsApp<schedulerId>::DemographicInfo
DemographicMetricsApp<schedulerId>::getInputData(
      const std::string& inputPath) {
    XLOG(INFO) << "Parsing input from " << inputPath;
    DemographicInfo outputInfo;

    auto readLine = [&](const std::vector<std::string>& header,
                        const std::vector<std::string>& parts) {
      addFromCSV(header, parts, outputInfo);
    };

    if (!fbpcf::demographic_metrics::readCsv(inputPath, readLine)) {
      XLOG(FATAL) << "Failed to read input file " << inputPath;
    }
    return outputInfo;
  }

  template <int schedulerId>
  void DemographicMetricsApp<schedulerId>::putOutputData(
      const std::string& output,
      const std::string& outputPath) {
    XLOG(INFO) << "putting out data...";
    fbpcf::io::FileIOWrappers::writeFile(outputPath, output);
  }

} // namespace DemographicMetricsApp

#include "./Csv.cpp"
