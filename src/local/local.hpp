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

#ifndef __MESOS_LOCAL_HPP__
#define __MESOS_LOCAL_HPP__

#include <process/process.hpp>

#include "configurator/configurator.hpp"

#include "master/master.hpp"


namespace mesos { namespace internal { namespace local {

// Register the options recognized by the local runner (a combination of
// master and slave options) with a configurator.
void registerOptions(Configurator* configurator);


// Launch a local cluster with a given number of slaves and given numbers
// of CPUs and memory per slave. Additionally one can also toggle whether
// to initialize Google Logging and whether to log quietly.
process::PID<master::Master> launch(int numSlaves,
                                    int32_t cpus,
                                    int64_t mem,
                                    bool initLogging,
                                    bool quiet);


// Launch a local cluster with a given configuration.
process::PID<master::Master> launch(const Configuration& conf,
                                    bool initLogging);


void shutdown();

}}} // namespace mesos { namespace internal { namespace local {

#endif // __MESOS_LOCAL_HPP__
