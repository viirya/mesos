#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include <map>
#include <ctime>

using namespace std;

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
  virtual int logEvent(map<string,string> keyval_pairs) = 0;
};

class FileEventWriter : public EventWriter {
private:
  ofstream logfile;
  time_t currTime;
public:
  FileEventWriter(); 
  ~FileEventWriter();
  int logEvent(map<string,string> keyval_pairs);
};

class EventLogger {
private:
  list<EventWriter*> writers; 
  Event event;
  bool logOnDestroy;
public:
  EventLogger(); 
  ~EventLogger();
  void setLogOnDestroy(bool);
  int logEvent(int num_pairs, ...);
  void writeEvent();
  EventLogger operator() (string s1, string s2);
};
