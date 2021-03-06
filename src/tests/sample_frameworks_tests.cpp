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

#include <gtest/gtest.h>

#include "config/config.hpp"

#include "tests/external_test.hpp"


// Run each of the sample frameworks in local mode
TEST_EXTERNAL(SampleFrameworks, CppFramework)
#if MESOS_HAS_JAVA
  TEST_EXTERNAL(SampleFrameworks, JavaFramework)
  TEST_EXTERNAL(SampleFrameworks, JavaExceptionFramework)
#endif 
#if MESOS_HAS_PYTHON
  TEST_EXTERNAL(SampleFrameworks, PythonFramework)
#endif

// TODO(*): Add the tests below as C++ tests.
// // Some tests for command-line and environment configuration
// TEST_EXTERNAL(SampleFrameworks, CFrameworkCmdlineParsing)
// TEST_EXTERNAL(SampleFrameworks, CFrameworkInvalidCmdline)
// TEST_EXTERNAL(SampleFrameworks, CFrameworkInvalidEnv)
