#pragma once

#include <future>
#include <memory>

#include <folly/dynamic.h>
#include "./DemographicMetricsApp.h" //@manual
#include "fbpcf/engine/communication/SocketPartyCommunicationAgentFactory.h"


namespace fbpcf::demographic_metrics {

inline std::pair<std::vector<std::string>, std::vector<std::string>>
getIOFilepaths(
    std::string inputBasePath,
    std::string outputBasePath,
    std::string inputDirectory,
    std::string outputDirectory,
    std::string inputFilenames,
    std::string outputFilenames,
    int32_t numFiles,
    int32_t fileStartIndex) {
  std::vector<std::string> inputFilepaths;
  std::vector<std::string> outputFilepaths;

  if (!inputBasePath.empty()) {
    std::string inputBasePathPrefix = inputBasePath + "_";
    std::string outputBasePathPrefix = outputBasePath + "_";
    for (auto i = fileStartIndex; i < fileStartIndex + numFiles; ++i) {
      inputFilepaths.push_back(inputBasePathPrefix + std::to_string(i));
      outputFilepaths.push_back(outputBasePathPrefix + std::to_string(i));
    }
  } else {
    std::filesystem::path inputDir{inputDirectory};
    std::filesystem::path outputDir{outputDirectory};

    std::vector<std::string> inputFilenamesVector;
    folly::split(',', inputFilenames, inputFilenamesVector);

    std::vector<std::string> outputFilenamesVector;
    folly::split(",", outputFilenames, outputFilenamesVector);

    // Make sure the number of input files equals output files
    CHECK_EQ(inputFilenamesVector.size(), outputFilenamesVector.size())
        << "Error: input_filenames and output_filenames have unequal sizes";

    for (std::size_t i = 0; i < inputFilenamesVector.size(); ++i) {
      inputFilepaths.push_back(inputDir / inputFilenamesVector[i]);
      outputFilepaths.push_back(outputDir / outputFilenamesVector[i]);
    }
  }
  return std::make_pair(inputFilepaths, outputFilepaths);
}

template <int PARTY, int index>
inline SchedulerStatistics startCalculatorAppsForShardedFilesHelper(
    int startFileIndex,
    long unsigned int remainingThreads,
    int numThreads,
    std::string serverIp,
    int port,
    std::vector<std::string>& inputFilepaths,
    std::string& inputGlobalParamsPath,
    std::vector<std::string>& outputFilepaths,
    fbpcf::engine::communication::SocketPartyCommunicationAgent::TlsInfo&
        tlsInfo,
    bool averageMetrics,
    bool varianceMetrics,
    bool histogramMetrics) {
  // aggregate scheduler statistics across apps
  SchedulerStatistics schedulerStatistics{
      0, 0, 0, 0, folly::dynamic::object()};

  // split files evenly across threads
  auto remainingFiles = inputFilepaths.size() - startFileIndex;
  if (remainingFiles > 0) {
    int numFiles = (remainingThreads > remainingFiles)
        ? 1
        : (remainingFiles / remainingThreads);

    std::map<
        int,
        fbpcf::engine::communication::SocketPartyCommunicationAgentFactory::
            PartyInfo>
        partyInfos(
            {{0, {serverIp, port + index * 100}},
             {1, {serverIp, port + index * 100}}});

    auto metricCollector = std::make_shared<fbpcf::util::MetricCollector>(
        "lift_metrics_for_thread_" + std::to_string(index));

    auto communicationAgentFactory = std::make_unique<
        fbpcf::engine::communication::SocketPartyCommunicationAgentFactory>(
        PARTY, partyInfos, tlsInfo, metricCollector);

    // Each CalculatorApp runs numFiles sequentially on a single thread
    // Publisher uses even schedulerId and partner uses odd schedulerId
    auto app = std::make_unique<DemographicMetricsApp<2 * index + PARTY>>(
        PARTY,
        std::move(communicationAgentFactory),
        inputFilepaths,
        outputFilepaths,
        //std::string(""),
        metricCollector,
        startFileIndex,
        numFiles);

    auto future = std::async([&app, &averageMetrics, &varianceMetrics, &histogramMetrics]() {
      app->run(true, averageMetrics, varianceMetrics, histogramMetrics);
      return app->getSchedulerStatistics();
    });

    // We construct a CalculatorApp for each thread recursively because each app
    // has a different schedulerId, which is a template parameter
    if constexpr (index < 64) {
      if (remainingThreads > 1) {
        auto remainingStats =
            startCalculatorAppsForShardedFilesHelper<PARTY, index + 1>(
                startFileIndex + numFiles,
                remainingThreads - 1,
                numThreads,
                serverIp,
                port,
                inputFilepaths,
                inputGlobalParamsPath,
                outputFilepaths,
                tlsInfo,
                averageMetrics,
                varianceMetrics,
                histogramMetrics);
        schedulerStatistics.add(remainingStats);
      }
    }
    auto stats = future.get();
    schedulerStatistics.add(stats);
  }
  return schedulerStatistics;
}

template <int PARTY>
inline SchedulerStatistics startCalculatorAppsForShardedFiles(
    std::vector<std::string>& inputFilepaths,
    std::string& inputGlobalParamsPath,
    std::vector<std::string>& outputFilepaths,
    int16_t concurrency,
    std::string serverIp,
    int port,
    fbpcf::engine::communication::SocketPartyCommunicationAgent::TlsInfo&
        tlsInfo,
    bool averageMetrics = false,
    bool varianceMetrics = false,
    bool histogramMetrics = false) {

  if (!(averageMetrics || varianceMetrics || histogramMetrics))
    averageMetrics = true;
  // use only as many threads as the number of files
  auto numThreads = std::min((int)inputFilepaths.size(), (int)concurrency);

  return startCalculatorAppsForShardedFilesHelper<PARTY, 0>(
      0,
      numThreads,
      numThreads,
      serverIp,
      port,
      inputFilepaths,
      inputGlobalParamsPath,
      outputFilepaths,
      tlsInfo,
      averageMetrics,
      varianceMetrics,
      histogramMetrics);
}

} // namespace fbpcf::edit_distance
