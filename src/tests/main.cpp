/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glog/logging.h>

#include <gtest/gtest.h>

#include <libgen.h>
#include <stdlib.h>

#include <string>

#include <process/process.hpp>

#include "common/fatal.hpp"

#include "configurator/configurator.hpp"

#include "tests/utils.hpp"

using namespace mesos::internal;
using namespace mesos::internal::test;

using std::string;


namespace {

// Get absolute path to Mesos home directory based on where the alltests
// binary is located (which should be in MESOS_HOME/bin/tests)
string getMesosHome(int argc, char** argv) {
  // Copy argv[0] because dirname can modify it
  int lengthOfArg0 = strlen(argv[0]);
  char* copyOfArg0 = new char[lengthOfArg0 + 1];
  strncpy(copyOfArg0, argv[0], lengthOfArg0 + 1);
  // Get its directory, and then the parent of the parent of that directory
  string myDir = string(dirname(copyOfArg0));
  string parentDir = myDir + "/../..";
  // Get the real name of this parent directory
  char path[PATH_MAX];
  if (realpath(parentDir.c_str(), path) == 0) {
    fatalerror("Failed to find location of MESOS_HOME using realpath");
  }
  return path;
}

}


int main(int argc, char** argv)
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Get absolute path to Mesos home directory based on location of alltests
  mesos::internal::test::mesosHome = getMesosHome(argc, argv);

  std::cout << "MESOS_HOME: " << mesos::internal::test::mesosHome << std::endl;

  // Clear any MESOS_ environment variables so they don't affect our tests
  Configurator::clearMesosEnvironmentVars();

  // Initialize Google Logging and Google Test
  google::InitGoogleLogging("alltests");
  testing::InitGoogleTest(&argc, argv);
  testing::FLAGS_gtest_death_test_style = "threadsafe";
  if (argc == 2 && strcmp("-v", argv[1]) == 0)
    google::SetStderrLogging(google::INFO);

  // Initialize libprocess library (but not glog, done above).
  process::initialize(false);

  return RUN_ALL_TESTS();
}
