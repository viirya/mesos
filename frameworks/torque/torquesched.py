#!/usr/bin/env python

import mesos
import os
import sys
import time
import httplib
import Queue
import threading
import re
import socket
import torquelib
import time
import logging
import logging.handlers

from optparse import OptionParser
from subprocess import *
from socket import gethostname

PBS_SERVER_FILE = "/var/spool/torque/server_name"
EVENT_LOG_FILE = "log_fw_utilization.txt"
LOG_FILE = "fw_torque_log.txt"

SCHEDULER_ITERATION = 2 #number of seconds torque waits before looping through
                        #the queue to try to match resources to jobs. default
                        #is 10min (ie 600) but we want it to be low so jobs 
                        #will run as soon as the framework has acquired enough
                        #resources
SAFE_ALLOCATION = {"cpus":24,"mem":24*1024} #just set statically for now, 48gb 
MIN_SLOT_SIZE = {"cpus":"1","mem":1024} #1GB
MIN_SLOTS_HELD = 0 #keep at least this many slots even if none are needed

eventlog = logging.getLogger("event_logger")
eventlog.setLevel(logging.DEBUG)
fh = logging.FileHandler(EVENT_LOG_FILE,'w') #create handler
fh.setFormatter(logging.Formatter("%(asctime)s %(message)s"))
eventlog.addHandler(fh)

#Something special about this file makes logging not work normally
#I think it might be swig? the StreamHandler prints at DEBUG level
#even though I setLevel to INFO
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
fh = logging.FileHandler(LOG_FILE,"w")
fh.setLevel(logging.DEBUG)

driverlog = logging.getLogger("driver_logger")
driverlog.setLevel(logging.DEBUG)
driverlog.addHandler(fh)
#driverlog.addHandler(ch)

monitorlog = logging.getLogger("monitor_logger")
monitorlog.setLevel(logging.DEBUG)
monitorlog.addHandler(fh)
monitorlog.addHandler(ch)

class MyScheduler(mesos.Scheduler):
  def __init__(self, ip):
    mesos.Scheduler.__init__(self)
    self.lock = threading.RLock()
    self.id = 0
    self.ip = ip 
    self.servers = {}
    self.overloaded = False
    self.numToRegister = MIN_SLOTS_HELD

  def registered(self, driver, fid):
    driverlog.info("Mesos torque framwork registered with frameworkID %s" % fid)

  def getFrameworkName(self, driver):
    return "Mesos Torque Framework"

  def getExecutorInfo(self, driver):
    print "in getExectutorInfo()"
    execPath = os.path.join(os.getcwd(), "start_pbs_mom.sh")
    initArg = self.ip # tell executor which node the pbs_server is running on
    driverlog.info("in getExecutorInfo, setting execPath = " + execPath + " and initArg = " + initArg)
    return mesos.ExecutorInfo(execPath, initArg)

  def resourceOffer(self, driver, oid, slave_offers):
    self.driver = driver
    driverlog.debug("Got slot offer %s" % oid)
    self.lock.acquire()
    driverlog.debug("resourceOffer() acquired lock")
    at_safe_alloc, nodes_used, no_more_needed = 0, 0, 0
    tasks = []
    for offer in slave_offers:
      # if we haven't registered this node, accept slot & register w pbs_server
      #TODO: check to see if slot is big enough 
      if self.numToRegister <= 0:
        no_more_needed += 1
      elif offer.host in self.servers.values():
        nodes_used += 1
      elif len(self.servers) >= SAFE_ALLOCATION["cpus"]:
        at_safe_alloc += 1
      else:
        driverlog.info("Need %d more nodes, so accepting slot, setting up params for it..." % self.numToRegister)
        params = {"cpus": "1", "mem": "1024"}
        td = mesos.TaskDescription(
            self.id, offer.slaveId, "task %d" % self.id, params, "")
        driverlog.info("Accepting task, id=" + str(self.id) + ", params: " +
                       params['cpus'] + " CPUS, and " + params['mem'] +
                       " MB, on node " + offer.host)
        tasks.append(td)
        self.servers[self.id] = offer.host
        self.regComputeNode(offer.host)
        self.numToRegister -= 1
        self.id += 1
        driverlog.debug("writing logfile")
        eventlog.info("%d %d" % (time.time(),len(self.servers)))
        driverlog.debug("done writing logfile")
        driverlog.info("self.id now set to " + str(self.id))
    #print "---"
    driverlog.debug("Replying to offer, accepting %d tasks." % len(tasks))
    #driver.replyToOffer(oid, tasks, {"timeout": "1"})
    driver.replyToOffer(oid, tasks, {})
    if nodes_used > 0:
      driverlog.debug(("Rejecting %d slots because we've launched servers " +
                      "on those machines already.") % nodes_used)
    if no_more_needed > 0:
      driverlog.debug(("Rejecting %d slot because we've launched enough " +
                      "tasks.") % no_more_needed)
    if at_safe_alloc > 0:
      driverlog.debug(("Rejecting slot, already at safe allocation (i.e. %d " + 
                      "CPUS)") % SAFE_ALLOCATION["cpus"])
    self.lock.release()
    driverlog.debug("resourceOffer() finished, released lock\n\n")

  def statusUpdate(self, driver, status):
    driverlog.info("got status update from TID %s, state is: %s, data is: %s" %(status.taskId,status.state,status.data))

  def regComputeNode(self, new_node):
    driverlog.info("registering new compute node, "+new_node+", with pbs_server")
    driverlog.info("checking to see if node is registered with server already")
    #nodes = Popen("qmgr -c 'list node'", shell=True, stdout=PIPE).stdout
    nodes = Popen("pbsnodes", shell=True, stdout=PIPE).stdout
    driverlog.info("output of pbsnodes command is: ")
    for line in nodes: 
      driverlog.info(line)
      if line.find(new_node) != -1:
        driverlog.info("Warn: tried to register node that's already registered, skipping")
        return
    #add node to server
    driverlog.info("registering node with command: qmgr -c create node " + new_node)
    qmgr_add = Popen("qmgr -c \"create node " + new_node + "\"", shell=True, stdout=PIPE).stdout
    driverlog.info("output of qmgr:")
    for line in qmgr_add: driverlog.info(line)

  def unregComputeNode(self, node_name):
    #remove node from server
    monitorlog.info("removing node from pbs_server: qmgr -c delete node " + node_name)
    monitorlog.info(Popen('qmgr -c "delete node ' + node_name + '"', shell=True, stdout=PIPE).stdout)
  
  #unreg up to N random compute nodes, leave at least one
  def unregNNodes(self, numNodes):
    monitorlog.debug("unregNNodes called with arg %d" % numNodes)
    if numNodes > len(self.servers)-MIN_SLOTS_HELD:
      monitorlog.debug("... however, only unregistering %d nodes, leaving one alive" % (len(self.servers)-MIN_SLOTS_HELD))
    toKill = min(numNodes,len(self.servers)-MIN_SLOTS_HELD)
    
    monitorlog.debug("getting and filtering list of nodes using torquelib")
    noJobs = lambda x: x.state != "job-exclusive"
    inactiveNodes = map(lambda x: x.name,filter(noJobs, torquelib.getNodes()))
    monitorlog.debug("victim pool of inactive nodes:")
    for inode in inactiveNodes:
      monitorlog.debug(inode)
    for tid, hostname in self.servers.items():
      if len(self.servers) > MIN_SLOTS_HELD and toKill > 0 and hostname in inactiveNodes:
        monitorlog.info("We still have to kill %d of the %d compute nodes which master is tracking" % (toKill, len(self.servers)))
        monitorlog.info("unregistering node " + str(hostname))
        self.unregComputeNode(hostname)
        self.servers.pop(tid)
        eventlog.info("%d %d" % (time.time(),len(self.servers)))
        toKill = toKill - 1
        monitorlog.info("killing corresponding task with tid %d" % tid)
        self.driver.killTask(tid)
    if toKill > 0: 
      monitorlog.warn("Done killing. We were supposed to kill %d nodes, but only found and killed %d free nodes" % (numNodes, numNodes-toKill))



def monitor(sched):
  while True:
    time.sleep(1)
    monitorlog.debug("monitor thread acquiring lock")
    sched.lock.acquire()
    monitorlog.debug("Computing num nodes needed to satisfy eligable " +
                     "jobs in queue")
    needed = 0
    jobs = torquelib.getActiveJobs()
    monitorlog.debug("Retreived jobs in queue, count: %d" % len(jobs))
    for j in jobs:
      #WARNING: this check should only be used if torque is using fifo queue
      #if needed + j.needsnodes <= SAFE_ALLOCATION:
      monitorlog.debug("Job resource list is: " + str(j.resourceList))
      needed += int(j.resourceList["nodect"])
    numToRelease = len(sched.servers) - needed
    monitorlog.debug(("Number of nodes to release is %d - %d = %d.") 
                     % (len(sched.servers), needed, numToRelease))
    if numToRelease > 0:
      sched.unregNNodes(numToRelease)
      sched.numToRegister = 0
    else:
      monitorlog.debug(("Monitor updating sched.numToRegister from %d to %d.") 
                       % (sched.numToRegister, numToRelease * -1))
      sched.numToRegister = numToRelease * -1
    sched.lock.release()
    monitorlog.debug("Monitor thread releasing lock.\n")

if __name__ == "__main__":
  parser = OptionParser(usage = "Usage: %prog mesos_master")

  (options,args) = parser.parse_args()
  if len(args) < 1:
    print >> sys.stderr, "At least one parameter required."
    print >> sys.stderr, "Use --help to show usage."
    exit(2)

  fqdn = socket.getfqdn()
  ip = socket.gethostbyname(gethostname())

  #monitorlog.info("running killall pbs_server")
  #Popen("killall pbs_server", shell=True)
  #time.sleep(1)

  #monitorlog.info("writing $(TORQUECFG)/server_name file with fqdn of pbs_server: " + fqdn)
  #Popen("touch %s" % PBS_SERVER_FILE, shell=True)
  #FILE = open(PBS_SERVER_FILE,'w')
  #FILE.write(fqdn)
  #FILE.close()

  monitorlog.info("starting pbs_server")
  #Popen("/etc/init.d/pbs_server start", shell=True)
  Popen("pbs_server", shell=True)
  #time.sleep(2)

 # monitorlog.info("running command: qmgr -c \"set queue batch resources_available.nodes=%s\"" % SAFE_ALLOCATION["cpus"])
 # Popen("qmgr -c \"set queue batch resources_available.nodect=%s\"" % SAFE_ALLOCATION["cpus"], shell=True)
 # Popen("qmgr -c \"set server resources_available.nodect=%s\"" % SAFE_ALLOCATION["cpus"], shell=True)

 # #these lines might not be necessary since we hacked the torque fifo scheduler
 # Popen("qmgr -c \"set queue batch resources_max.nodect=%s\"" % SAFE_ALLOCATION["cpus"], shell=True)
 # Popen("qmgr -c \"set server resources_max.nodect=%s\"" % SAFE_ALLOCATION["cpus"], shell=True)
 # Popen("qmgr -c \"set server scheduler_iteration=%s\"" % SCHEDULER_ITERATION, shell=True)

 # outp = Popen("qmgr -c \"l queue batch\"", shell=True, stdout=PIPE).stdout
 # for l in outp:
 #   monitorlog.info(l)

 # monitorlog.info("RE-killing pbs_server for resources_available setting to take effect")
 # #Popen("/etc/init.d/pbs_server start", shell=True)
 # Popen("qterm", shell=True)
 # time.sleep(1)

 # monitorlog.info("RE-starting pbs_server for resources_available setting to take effect")
 #Popen("pbs_server", shell=True)
 # monitorlog.debug("qmgr list queue settings: ")
 # output = Popen("qmgr -c 'l q batch'", shell=True, stdout=PIPE).stdout
 # for line in output:
 #   monitorlog.debug(line)

 # monitorlog.info("running killall pbs_sched")
 # Popen("killall pbs_sched", shell=True)
 # #time.sleep(2)

  #monitorlog.info("starting pbs_scheduler")
  #Popen("/etc/init.d/pbs_sched start", shell=True)
  #Popen("pbs_sched", shell=True)

  monitorlog.info("starting maui")
  #Popen("/etc/init.d/pbs_sched start", shell=True)
  Popen("maui", shell=True)

  #ip = Popen("hostname -i", shell=True, stdout=PIPE).stdout.readline().rstrip() #linux
  #ip = Popen("ifconfig en1 | awk '/inet / { print $2 }'", shell=True, stdout=PIPE).stdout.readline().rstrip() # os x
  monitorlog.info("Remembering IP address of scheduler (" + ip + "), and fqdn: " + fqdn)

  monitorlog.info("Connecting to mesos master %s" % args[0])

  sched = MyScheduler(fqdn)
  threading.Thread(target = monitor, args=[sched]).start()

  mesos.MesosSchedulerDriver(sched, args[0]).run()

  monitorlog.info("Finished!")
