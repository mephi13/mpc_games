#pragma once

#include <cstdlib>
#include <filesystem>
#include <string>
#include "folly/logging/xlog.h"
#include <folly/dynamic.h>
#include <cstdint>
#include <exception>

#include "fbpcf/engine/communication/IPartyCommunicationAgentFactory.h"
#include "fbpcf/scheduler/SchedulerHelper.h"
#include "../demographic_metrics/DemographicMetricsGame.h"

namespace fbpcf::demographic_metrics {

struct SchedulerStatistics {
    uint64_t nonFreeGates;
    uint64_t freeGates;
    uint64_t sentNetwork;
    uint64_t receivedNetwork;
    folly::dynamic details;

    void add(SchedulerStatistics other) {
        nonFreeGates += other.nonFreeGates;
        freeGates += other.freeGates;
        sentNetwork += other.sentNetwork;
        receivedNetwork += other.receivedNetwork;
        try {
        details = folly::dynamic::merge(details, other.details);
        } catch (std::exception& e) {
        details = std::string("Failed to merge details: ") + e.what();
        }
    }
};

template <int schedulerId>
class DemographicMetricsApp {
    using DemographicInfo = 
        typename fbpcf::demographic_metrics::DemographicMetricsGame<schedulerId>::DemographicInfo;

    public:
        DemographicMetricsApp(
            const int party,
            std::unique_ptr<
                fbpcf::engine::communication::IPartyCommunicationAgentFactory>
                communicationAgentFactory,
            const std::vector<std::string>& inputPaths,
            const std::vector<std::string>& outputPaths,
            //const std::string& paramsFilePath,
            std::shared_ptr<fbpcf::util::MetricCollector> metricCollector,
            const int startFileIndex = 0,
            const int numFiles = 1)
            : party_{party},
            communicationAgentFactory_{std::move(communicationAgentFactory)},
            inputPaths_(inputPaths),
            //paramsPath_(paramsFilePath),
            outputPaths_(outputPaths),
            metricCollector_(metricCollector),
            startFileIndex_(startFileIndex),
            numbFiles_(numFiles) {};

        void run(
            bool validate = true,
            bool average = true,
            bool variance = false,
            bool histogram = false
        );

        void addFromCSV(
            const std::vector<std::string>& header,
            const std::vector<std::string>& parts,
            DemographicInfo& demographicInfo);

        DemographicInfo getInputData(
            const std::string& inputPath);

        void putOutputData(
            const std::string& output,
            const std::string& outputPath);

        SchedulerStatistics getSchedulerStatistics() {
            return schedulerStatistics_;
        }
    private:   
        int party_;
        std::unique_ptr<fbpcf::engine::communication::IPartyCommunicationAgentFactory>
            communicationAgentFactory_;
        std::vector<std::string> inputPaths_;
        std::vector<std::string> paramsPath_;
        std::vector<std::string> outputPaths_;
        std::shared_ptr<fbpcf::util::MetricCollector> metricCollector_;
        long unsigned int startFileIndex_;
        int numbFiles_;
        SchedulerStatistics schedulerStatistics_;
};

} // namespace demographic_metrics

#include "./DemographicMetricsApp_impl.h"
