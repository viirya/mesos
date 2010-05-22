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
using namespace nexus;
using namespace nexus::internal;

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
  //virtual int logEvent(map<string,string> keyval_pairs) = 0;
  virtual int createTask(TaskID, SlaveID, Resources) = 0;
};

class FileEventWriter : public EventWriter {
private:
  ofstream logfile;
  time_t currTime;
public:
  FileEventWriter(); 
  ~FileEventWriter();
  //int logEvent(map<string,string> keyval_pairs);
  int createTask(TaskID, SlaveID, Resources);
};

class SqlLiteEventWriter : public EventWriter {
private:
  sqlite3 *db; 
  char *zErrMsg;
  time_t currTime;
public:
  SqlLiteEventWriter(); 
  ~SqlLiteEventWriter();
  int createTask(TaskID, SlaveID, Resources);
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
  int createTask(TaskID, SlaveID, Resources);
  //int updateTaskStatus(TaskID, TaskStatus); 
  void writeEvent();
  EventLogger operator() (string, string);
};
