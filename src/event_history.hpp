#ifndef __EVENT_HISTORY_HPP__
#define __EVENT_HISTORY_HPP__

#include "nexus.hpp"
#include "resources.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include <map>
#include <ctime>
#include <sqlite3.h>

using namespace std;
using nexus::FrameworkID;
using nexus::TaskID;
using nexus::SlaveID;
using nexus::FrameworkID;
using nexus::TaskState;
using nexus::internal::Resources;

class EventWriter {
public:
  virtual ~EventWriter() {}
  virtual string getName() = 0;
  virtual int logTaskCreated(TaskID, FrameworkID, SlaveID, Resources) = 0;
  virtual int logTaskStateUpdated(TaskID, FrameworkID, TaskState) = 0; 
  virtual int logFrameworkRegistered(FrameworkID, string) = 0;
  virtual int logFrameworkUnregistered(FrameworkID) = 0;
};

class FileEventWriter : public EventWriter {
private:
  ofstream logfile;
  time_t currTime;
public:
  string getName();
  FileEventWriter(); 
  ~FileEventWriter();
  int logTaskCreated(TaskID, FrameworkID, SlaveID, Resources);
  int logTaskStateUpdated(TaskID, FrameworkID, TaskState); 
  int logFrameworkRegistered(FrameworkID, string);
  int logFrameworkUnregistered(FrameworkID);
};

class SqlLiteEventWriter : public EventWriter {
private:
  sqlite3 *db; 
  char *zErrMsg;
  time_t currTime;
public:
  string getName();
  SqlLiteEventWriter(); 
  ~SqlLiteEventWriter();
  int logTaskCreated(TaskID, FrameworkID, SlaveID, Resources);
  int logTaskStateUpdated(TaskID, FrameworkID, TaskState); 
  int logFrameworkRegistered(FrameworkID, string);
  int logFrameworkUnregistered(FrameworkID);
};

class EventLogger {
private:
  list<EventWriter*> writers; 
public:
  EventLogger(); 
  ~EventLogger();
  int logResourceOffer(FrameworkID, Resources);
  int logTaskCreated(TaskID, FrameworkID, SlaveID, Resources);
  int logTaskStateUpdated(TaskID, FrameworkID, TaskState); 
  int logFrameworkRegistered(FrameworkID, string);
  int logFrameworkUnregistered(FrameworkID);
  EventLogger operator() (string, string);
};

#endif /* __EVENT_HISTORY_HPP__ */
