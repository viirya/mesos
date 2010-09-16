======================================================
======================================================
               MESOS Torque Framework
======================================================

Mesos TORQUE framework readme
--------------------------------------------
This framework is a wrapper around the Torque cluster resource manager for the cluster which integrates with a cluster scheduler such as pbs_sched, maui, or moab. It will allow you to run Torque on top of mesos, i.e. Torque will get the resources that it schedules its jobs on (i.e. its nodes) via Mesos resource offers.



Installing TORQUE (comes with pbs_sched), MAUI:
-------------------------------------------------

option #0) if you are running mesos on EC2 using the ec2/mesos-ec2 script, then you should be able to find a number of setup scripts in the ~/mesos-ec2 directory. These may not have been committed to the master branch.

option #1) install from source (recommended), follow instructions at http://www.clusterresources.com/products/torque/docs/

option #2) sudo apt-get install torque-server, torque-mom (also potentially relevant: torque-dev, torque-client)

NOTE: You will probably have to update your PATH after maui is installed (in /usr/local/maui by default) by running something like: `export PATH=$PATH:/usr/local/maui/bin:/usr/local/maui/sbin`.


==Structure Overview of the Framework==
---------------------------------------

==FRAMEWORK EXECUTOR==
The mesos executor for this framework will run pbs_mom and tell it to look at the framework scheduler as its host. Thus, you will need to have pbs_mom installed on all mesos slave nodes that might run tasks for this framework. This is done as part of the ~/mesos-ec2/setup-frameworks/torque/setup-torque script (the location of this script might move around, and it actually currently not committed to master, I don't think? --andyk, 9/15/10).

You should be able to ssh into a slave node and see pbs_mom running (e.g. by running `ps aux|grep pbs_mom`. You can manually interact with pbs_mom using the `momctl` program.


==FRAMEWORK SCHEDULER==
The torque FW scheduler is responsible for managing the pbs_server daemon. This includees starting it if necessary, adding and removing nodes using the `qmgr -c "create node NODE_FQDN"` command after it accepts a resource offer on NODE_FQDN. In this way, the framework will dynamically grow (but never beyond its 'safe allocation') or shrink or releases resources.

The FW scheduler accepts new resource offers and launches pbs_mom tasks in response to an increase in the torque queue length. Conversely, in order to free resources it doesn't currently need for other active frameworks in the cluster, it kills existing tasks (optionally maintaining some minimum number at all times), i.e. pbs_mom backend daemons running in the cluster, in response to the scheduler queue becoming more (or entirely) empty. 

The minimum number of resources torque should hang on to is configurable via global variables at the top of the scheduler code:
MIN_COMPUTE_NODE = resource vector of resources per compute node min
MIN_SLOTS_HELD = # compute nodes to keep around min


===CHOOSING A TORQUE SCHEDULER===
The framework can use whichever torque compatible scheduler that is desired. By default, the torque ec2 setup scripts that (will eventually) come with mesos will install maui, which the Mesos FW Scheduler (i.e. torquesched.py, which is called by torquesched.sh) will try to run when it starts up. You can also use the default torque fifo scheduler (pbs_sched).


===MPI===
The ec2 torque setup scripts also currently install mpich2 for you on all slaves. This is done by running ~/mesos-ec2/[setup-frameworks/torque/]setup-mpi, which uses apt-get to install mpich2 on all nodes (though the default AMI's that mesos-ec2 uses right now already has it installed, I believe).

Epilogue and Prologue scripts are run when qsub returns an allocation of machines. These scripts setup an MPI ring for you, which means you *should* be able to do mpirun right away inside of your allocation.



Permissions:
------------

****Currently****
As of right now, to run this framework, the node that the framework scheduler is run on will need to have pbs_server installed. This is setup for you automatically in the ec2 torque setup scripts which I (andyk) am using for OSDDI and NSDI paper experiments. These should be committed at some point to master branch so that they will get put into ~/mesos-ec2 on the ec2 deployments of mesos using the MESOS_HOME/ec2/mesos-ec2 script.

The framework scheduler will launch pbs_server for you, I *think* you need to be root to run the pbs_server daemon (I haven't figured out how to do it otherwise). 

****Future Alternative****
An alternate way that we could structure this framework is to require that pbs_server is running on some server already but that it is running with the intention to be fully managed by the torque mesos framework. I think that the management commands can be set up to work for non root users, which the framework could then run as. Then the mesos torque framework scheduler would take the address of the pbs_server as a parameter and would assume it has permissions to add and remove nodes from the server 



Debugging notes:
-----------------

- Be *very* careful about naming in config files, I *think* that FQDNs will work if you use them for both pbs_mom and pbs_server but a node keeps showing up with status "down" when I add it without using the .eecs.berkeley.edu (this is on my own laptop as both slave and server)



TODO:
-----

- explore an install a torque UI
 -- maybe PBSWeb  (http://www.clusterresources.com/pipermail/torqueusers/2004-March/000411.html) or apt-get install torque-gui (which has gui clients)
- figure out permissions better (this page http://www.clusterresources.com/torquedocs21/commands/qrun.shtml mentions "PBS Operation or Manager privilege.")
