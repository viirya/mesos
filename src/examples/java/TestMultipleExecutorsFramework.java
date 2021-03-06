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

import com.google.protobuf.ByteString;

import java.io.File;
import java.io.IOException;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.mesos.*;
import org.apache.mesos.Protos.*;


public class TestMultipleExecutorsFramework {
  static class MyScheduler implements Scheduler {
    int launchedTasks = 0;
    int finishedTasks = 0;
    int totalTasks = 5;

    public MyScheduler() {}

    public MyScheduler(int numTasks) {
      totalTasks = numTasks;
    }

    @Override
    public String getFrameworkName(SchedulerDriver driver) {
      return "Java test framework";
    }

    @Override
    public ExecutorInfo getExecutorInfo(SchedulerDriver driver) {
      try {
        File file = new File("./test_executor");
        return ExecutorInfo.newBuilder()
          .setExecutorId(ExecutorID.newBuilder().setValue("default").build())
          .setUri(file.getCanonicalPath())
          .build();
      } catch (Throwable t) {
        throw new RuntimeException(t);
      }
    }

    @Override
    public void registered(SchedulerDriver driver, FrameworkID frameworkId) {
      System.out.println("Registered! ID = " + frameworkId.getValue());
    }

    @Override
    public void resourceOffer(SchedulerDriver driver,
                              OfferID offerId,
                              List<SlaveOffer> offers) {

      List<TaskDescription> tasks = new ArrayList<TaskDescription>();

      try {
        File file = new File("./test_executor");

        for (SlaveOffer offer : offers) {
          if (!fooLaunched) {
            TaskID taskId = TaskID.newBuilder()
              .setValue("foo")
              .build();

            System.out.println("Launching task " + taskId.getValue());

            TaskDescription task = TaskDescription.newBuilder()
              .setName("task " + taskId.getValue())
              .setTaskId(taskId)
              .setSlaveId(offer.getSlaveId())
              .addResources(Resource.newBuilder()
                            .setName("cpus")
                            .setType(Resource.Type.SCALAR)
                            .setScalar(Resource.Scalar.newBuilder()
                                       .setValue(1)
                                       .build())
                            .build())
              .addResources(Resource.newBuilder()
                            .setName("mem")
                            .setType(Resource.Type.SCALAR)
                            .setScalar(Resource.Scalar.newBuilder()
                                       .setValue(128)
                                       .build())
                            .build())
              .setExecutor(ExecutorInfo.newBuilder()
                           .setExecutorId(ExecutorID.newBuilder()
                                          .setValue("executor-foo")
                                          .build())
                               .setUri(file.getCanonicalPath())
                               .build())
              .setData(ByteString.copyFromUtf8("100000"))
              .build();

            tasks.add(task);

            fooLaunched = true;
          }

          if (!barLaunched) {
            TaskID taskId = TaskID.newBuilder()
              .setValue("bar")
              .build();

            System.out.println("Launching task " + taskId.getValue());

            TaskDescription task = TaskDescription.newBuilder()
              .setName("task " + taskId.getValue())
              .setTaskId(taskId)
              .setSlaveId(offer.getSlaveId())
              .addResources(Resource.newBuilder()
                            .setName("cpus")
                            .setType(Resource.Type.SCALAR)
                            .setScalar(Resource.Scalar.newBuilder()
                                       .setValue(1)
                                       .build())
                            .build())
              .addResources(Resource.newBuilder()
                            .setName("mem")
                            .setType(Resource.Type.SCALAR)
                            .setScalar(Resource.Scalar.newBuilder()
                                       .setValue(128)
                                       .build())
                            .build())
              .setExecutor(ExecutorInfo.newBuilder()
                           .setExecutorId(ExecutorID.newBuilder()
                                          .setValue("executor-bar")
                                          .build())
                           .setUri(file.getCanonicalPath())
                           .build())
              .setData(ByteString.copyFrom("100000".getBytes()))
              .build();

            tasks.add(task);

            barLaunched = true;
          }

          driver.replyToOffer(offerId, tasks);
        }
      } catch (Throwable t) {
        throw new RuntimeException(t);
      }
    }

    @Override
    public void offerRescinded(SchedulerDriver driver, OfferID offerId) {}

    @Override
    public void statusUpdate(SchedulerDriver driver, TaskStatus status) {
      System.out.println("Status update: task " + status.getTaskId() +
                         " is in state " + status.getState());
      if (status.getState() == TaskState.TASK_FINISHED) {
        finishedTasks++;
        System.out.println("Finished tasks: " + finishedTasks);
        if (finishedTasks == totalTasks)
          driver.stop();
      }
    }

    @Override
    public void frameworkMessage(SchedulerDriver driver, SlaveID slaveId, ExecutorID executorId, byte[] data) {}

    @Override
    public void slaveLost(SchedulerDriver driver, SlaveID slaveId) {}

    @Override
    public void error(SchedulerDriver driver, int code, String message) {
      System.out.println("Error: " + message);
    }

    private boolean fooLaunched = false;
    private boolean barLaunched = false;
  }

  public static void main(String[] args) throws Exception {
    if (args.length < 1 || args.length > 2) {
      System.out.println("Invalid use: please specify a master");
    } else if (args.length == 1) {
      new MesosSchedulerDriver(new MyScheduler(),args[0]).run();
    } else {
      new MesosSchedulerDriver(new MyScheduler(Integer.parseInt(args[1])), args[0]).run();
    }
  }
}
