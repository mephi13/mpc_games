# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

cmake_minimum_required(VERSION 3.10.1)
project(games)
set(NAME "games")

find_file(fbpcf_cmake NAMES cmake/fbpcf.cmake)
include(${fbpcf_cmake})

find_package(Boost REQUIRED COMPONENTS serialization)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
include_directories(/usr/local/include)
find_library(fbpcf fbpcf /usr/local/lib/)

find_package(folly REQUIRED)
find_package(google_cloud_cpp_storage REQUIRED)
find_package(gflags REQUIRED)

find_package(Boost COMPONENTS serialization REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(
  demographic
  "demographic_metrics/main.cpp"
  "demographic_metrics/DemographicMetricsGame.h"
  "demographic_metrics/DemographicMetricsGame_impl.h")
target_link_libraries(
  demographic
  fbpcf
  Folly::folly
)

add_executable(
  demographicapp
  "demographic_metrics_app/main.cpp"
  "demographic_metrics_app/DemographicMetricsApp.h"
  "demographic_metrics_app/DemographicMetricsApp_impl.h"
  "demographic_metrics_app/Csv.h"
  "demographic_metrics_app/MainUtil.h"
  "demographic_metrics_app/MPCTypes.h"
  )
target_link_libraries(
  demographicapp
  fbpcf
  ${Boost_LIBRARIES}
  ${AWSSDK_LINK_LIBRARIES}
  ${EMP-OT_LIBRARIES}
  google-cloud-cpp::storage
  Folly::folly
  re2
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
 set(CMAKE_INSTALL_PREFIX "../")
endif()

install(TARGETS demographic DESTINATION bin)
install(TARGETS demographicapp DESTINATION bin)