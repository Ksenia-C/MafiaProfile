# Copyright 2018 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# cmake build file for C++ helloworld example.
# Assumes protobuf and gRPC have been installed using cmake.
# See cmake_externalproject/CMakeLists.txt for all-in-one cmake build
# that automatically builds all the dependencies before building helloworld.

cmake_minimum_required(VERSION 3.8)

project(ProfileServer C CXX)

# Generated sources
set (CMAKE_CXX_STANDARD 20)

include_directories("${CMAKE_CURRENT_LIST_DIR}/libharu/")

add_executable(server server.cpp)
target_link_libraries(server
  SQLiteCpp
  sqlite3
  pthread
  dl
  restbed
  hpdf
  )

target_include_directories(server
  PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/restbed/distribution/include/"
  "${CMAKE_CURRENT_LIST_DIR}/helpers/"
  "/usr/local/include/"
)