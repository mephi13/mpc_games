# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

cmake_minimum_required(VERSION 3.10.1)
project(games)
set(NAME "games")

add_compile_options(-std=c++17)
add_compile_options(-march=haswell)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't compile with AVX512 instructions since many of the AWS
# instances won't have access to that instruction set.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-avx512f")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-avx512f")

find_package(Boost REQUIRED COMPONENTS serialization)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
include_directories(/usr/local/include)
find_library(fbpcf fbpcf /usr/local/lib/)

find_package(folly REQUIRED)
find_package(google_cloud_cpp_storage REQUIRED)
find_package(gflags REQUIRED)

add_executable(
  billionaire
  "billionaire_problem/main.cpp"
  "billionaire_problem/BillionaireProblemGame.h"
  "billionaire_problem/BillionaireProblemGame_impl.h")
target_link_libraries(
  billionaire
  fbpcf
  Folly::folly
)

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

# add_executable(
#   validation
#   "input_validation/main.cpp"
#   "input_validation/InputValidationGame.h"
#   "input_validation/InputValidationGame_impl.h")
# target_link_libraries(
#   validation
#   fbpcf
#   Folly::folly
# )

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
 set(CMAKE_INSTALL_PREFIX "../")
endif()

install(TARGETS billionaire DESTINATION bin)
install(TARGETS demographic DESTINATION bin)
