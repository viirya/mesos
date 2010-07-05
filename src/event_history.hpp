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
using nexus::TaskStatus;
using nexus::internal::Resources;

class Event {
public:
  Event(string, string);
  //Event operator() (string, string);
  void addAttribute(string, string);
  //void addAttributes(map<string,string>);
  //void toString();

private:
  map<string,string> eventBuffer;
};

class EventWriter {
public:
  virtual ~EventWriter() {}
  virtual string getName() = 0;
  //virtual int logEvent(map<string,string> keyval_pairs) = 0;
  virtual int logCreateTask(TaskID, SlaveID, Resources) = 0;
  virtual int logCreateFramework(FrameworkID, string) = 0;
};

class FileEventWriter : public EventWriter {
private:
  ofstream logfile;
  time_t currTime;
public:
  string getName();
  FileEventWriter(); 
  ~FileEventWriter();
  //int logEvent(map<string,string> keyval_pairs);
  int logCreateTask(TaskID, SlaveID, Resources);
  int logCreateFramework(FrameworkID, string);
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
  int logCreateTask(TaskID, SlaveID, Resources);
  int logCreateFramework(FrameworkID, string);
};

class EventLogger {
private:
  Event event;
  list<EventWriter*> writers; 
  bool logOnDestroy;
public:
  EventLogger(); 
  ~EventLogger();
  void setLogOnDestroy(bool);
  /*log arbitrary keyval pair */
  //int logEvent(int num_pairs, ...);
  /*semantic logging statements */
  int logResourceOffer(FrameworkID, Resources);
  int logCreateTask(TaskID, SlaveID, Resources);
  int logCreateFramework(FrameworkID, string);
  //int updateTaskStatus(TaskID, TaskStatus); 
  void writeEvent();
  EventLogger operator() (string, string);
};

#endif /* __EVENT_HISTORY_HPP__ */
