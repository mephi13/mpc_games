/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <signal.h>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>

#include "folly/String.h"
#include "folly/init/Init.h"
#include "folly/logging/xlog.h"

#include <fbpcf/aws/AwsSdk.h>
#include "./MPCTypes.h" // @manual
#include "./MainUtil.h" // @manual

DEFINE_int32(party, 1, "1 = publisher, 2 = partner");
DEFINE_string(server_ip, "127.0.0.1", "Server's IP Address");
DEFINE_int32(
    port,
    10000,
    "Network port for establishing connection to other player");
DEFINE_string(
    input_directory,
    "",
    "Data directory where input files are located");
DEFINE_string(
    input_filenames,
    "in.csv_0[,in.csv_1,in.csv_2,...]",
    "List of input file names that should be parsed (should have a header)");
DEFINE_string(
    input_global_params_path,
    "out.csv_global_params_0",
    "Input file name of global parameter setup. Used when reading inputs in secret share format rather than plaintext.");
DEFINE_string(
    output_directory,
    "",
    "Local or s3 path where output files are written to");
DEFINE_string(
    output_filenames,
    "out.csv_0[,out.csv_1,out.csv_2,...]",
    "List of output file names that correspond to input filenames (positionally)");
DEFINE_string(
    input_base_path,
    "",
    "Local or s3 base path for the sharded input files");
DEFINE_string(
    output_base_path,
    "",
    "Local or s3 base path where output files are written to");
DEFINE_int32(
    file_start_index,
    0,
    "First file that will be read with base path");
DEFINE_int32(num_files, 0, "Number of files that should be read");

DEFINE_int32(
    concurrency,
    1,
    "max number of game(s) that will run concurrently?");
DEFINE_string(
    run_name,
    "",
    "A user given run name that will be used in s3 filename");
DEFINE_string(
    run_id,
    "",
    "A run_id used to identify all the logs in a PL run.");

DEFINE_string(log_cost_s3_bucket, "", "s3 bucket name");
DEFINE_string(
    log_cost_s3_region,
    ".s3.us-west-2.amazonaws.com/",
    "s3 regioni name");
DEFINE_string(
    pc_feature_flags,
    "",
    "A String of PC Feature Flags passing from PCS, separated by comma");
DEFINE_bool(
    use_tls,
    false,
    "Whether to use TLS when communicating with other parties.");
DEFINE_string(
    ca_cert_path,
    "",
    "Relative file path where root CA cert is stored. It will be prefixed with $HOME.");
DEFINE_string(
    server_cert_path,
    "",
    "Relative file path where server cert is stored. It will be prefixed with $HOME.");
DEFINE_string(
    private_key_path,
    "",
    "Relative file path where private key is stored. It will be prefixed with $HOME.");

int main(int argc, char** argv) {
  folly::init(&argc, &argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  fbpcf::AwsSdk::aquire();

  int16_t concurrency = static_cast<int16_t>(FLAGS_concurrency);

  signal(SIGPIPE, SIG_IGN);

  auto filepaths = fbpcf::demographic_metrics::getIOFilepaths(
      FLAGS_input_base_path,
      FLAGS_output_base_path,
      FLAGS_input_directory,
      FLAGS_output_directory,
      FLAGS_input_filenames,
      FLAGS_output_filenames,
      FLAGS_num_files,
      FLAGS_file_start_index);
  auto inputFilepaths = filepaths.first;
  auto outputFilepaths = filepaths.second;

  auto tlsInfo = fbpcf::engine::communication::getTlsInfoFromArgs(
      FLAGS_use_tls,
      FLAGS_ca_cert_path,
      FLAGS_server_cert_path,
      FLAGS_private_key_path,
      "");

  {
    // Build a quick list of input/output files to log
    std::ostringstream inputFileLogList;
    for (auto inputFilepath : inputFilepaths) {
      inputFileLogList << "\t\t" << inputFilepath << "\n";
    }
    std::ostringstream outputFileLogList;
    for (auto outputFilepath : outputFilepaths) {
      outputFileLogList << "\t\t" << outputFilepath << "\n";
    }
    XLOG(INFO) << "Running conversion lift with settings:\n"
               << "\tparty: " << FLAGS_party << "\n"
               << "\tserver_ip_address: " << FLAGS_server_ip << "\n"
               << "\tport: " << FLAGS_port << "\n"
               << "\tconcurrency: " << FLAGS_concurrency << "\n"
               << "\tpc_feature_flags:" << FLAGS_pc_feature_flags
               << "\tinput: " << inputFileLogList.str()
               << "\toutput: " << outputFileLogList.str() << "\n"
               << "\tinput global params path: "
               << FLAGS_input_global_params_path << "\n"
               << "\trun_id: " << FLAGS_run_id;
  }

  FLAGS_party--; // subtract 1 because we use 0 and 1 for publisher and partner
                 // instead of 1 and 2
  fbpcf::demographic_metrics::SchedulerStatistics schedulerStatistics;

  XLOG(INFO) << "Start Private Lift...";
  if (FLAGS_party == 0) {
    XLOG(INFO)
        << "Starting as Alice, will wait for Bob...";
    schedulerStatistics =
        fbpcf::demographic_metrics::startCalculatorAppsForShardedFiles<0>(
            inputFilepaths,
            FLAGS_input_global_params_path,
            outputFilepaths,
            concurrency,
            FLAGS_server_ip,
            FLAGS_port,
            tlsInfo);
  } else if (FLAGS_party == 1) {
    XLOG(INFO)
        << "Starting as Bob, will wait for Alice...";
    schedulerStatistics =
        fbpcf::demographic_metrics::startCalculatorAppsForShardedFiles<1>(
            inputFilepaths,
            FLAGS_input_global_params_path,
            outputFilepaths,
            concurrency,
            FLAGS_server_ip,
            FLAGS_port,
            tlsInfo);
  } else {
    XLOGF(FATAL, "Invalid Party: {}", FLAGS_party);
  }

  XLOGF(
      INFO,
      "Non-free gate count = {}, Free gate count = {}",
      schedulerStatistics.nonFreeGates,
      schedulerStatistics.freeGates);

  XLOGF(
      INFO,
      "Sent network traffic = {}, Received network traffic = {}",
      schedulerStatistics.sentNetwork,
      schedulerStatistics.receivedNetwork);

  return 0;
}