#include "event_history.hpp"
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdarg.h>

//#include <glog/logging.h>

long getTimeStamp(){
  struct timeval curr_time;
  struct timezone tzp;
  gettimeofday(&curr_time, &tzp);
  return (long)(curr_time.tv_sec * 1000000 + curr_time.tv_usec);
}

//////////Event/////////
Event::Event(string s1 = "", string s2 = "") {
if (s1.compare("") != 0 && s2.compare("") != 0) {
addAttribute(s1,s2);
}
}

void Event::addAttribute(string s1, string s2) {
  eventBuffer[s1] = s2;
}


//////////FileEventWriter/////////
FileEventWriter::FileEventWriter() {
  time_t currTime; /* calendar time */
  //if log dir doesn't exist, create it
  struct stat sb;
  if (stat("logs", &sb) == -1) {
    //fatalerror("chdir into log directory failed");
    cout << "stat on logs directory failed, [re]creating the dir" << endl;
    if (mkdir("logs", S_IRWXU | S_IRWXG) != 0) {
      //LOG(ERROR) << "encountered an error while creating 'logs' directory, "
        //<< "event history may not be captured";
    }
  }
  //set up log file in log dir
  logfile.open ("logs/event_history_log.txt",ios::app|ios::out);
  cout << "opened log file" << endl;
}

FileEventWriter::~FileEventWriter() {
  logfile.close();
  cout << "closed log file" << endl;
}

/*int FileEventWriter::logEvent(map<string,string> keyval_pairs) {
  //print curr timestamp
  currTime=time(NULL); // get current cal time
  string timestamp = asctime(localtime(&currTime));
  timestamp.erase(24); // chop off the newline
  logfile << timestamp << ",";

  map<string,string>::iterator it;
  for (it=keyval_pairs.begin(); it != keyval_pairs.end(); it++) {
    logfile << (*it).first << ":" << (*it).second;
    cout << (*it).first << ":" << (*it).second;
    map<string,string>::iterator itp = it;
    itp++;
    if (itp == keyval_pairs.end()) {
      logfile << endl;
      cout << endl;
    } else {
      logfile << ",";
      cout << ",";
    }
  }
  logfile.flush();
  return 0;
}
*/

int FileEventWriter::createTask(TaskID tid, SlaveID sid, Resources resVec) {
  currTime=time(NULL); /* get current cal time */
  string timestamp = asctime(localtime(&currTime));
  timestamp.erase(24); /* chop off the newline */
  logfile << timestamp << "," << tid << "," << sid << ",";;
  logfile << "createTask" << "," << resVec.cpus << "," << resVec.mem << "\n";

  logfile.flush();
  return 0;
}


//////////SqlLiteEventWriter/////////
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

SqlLiteEventWriter::SqlLiteEventWriter() {
  zErrMsg = 0;
  time_t currTime; /* calendar time */
  //if log dir doesn't exist, create it
  struct stat sb;
  if (stat("logs", &sb) == -1) {
    //fatalerror("chdir into log directory failed");
    cout << "stat on logs directory failed, [re]creating the dir" << endl;
    if (mkdir("logs", S_IRWXU | S_IRWXG) != 0) {
      //LOG(ERROR) << "encountered an error while creating 'logs' directory, "
        //<< "event history may not be captured";
    }
  }
  //set up log file in log dir
  int rc = sqlite3_open("logs/event_history_db",&db);
  if( rc ) {
    cerr << "ERROR: Can't open database: " << sqlite3_errmsg(db) << endl;
    sqlite3_close(db);
    //exit(1);
  } else {
    cout << "opened sql lite db" << endl;
  }
  //create task table in case it doesn't already exist,
  //if it does this shouldn't destroy it
  sqlite3_exec(db, "CREATE TABLE task (taskid Varchar(255), fwid Varchar(255), date_created integer, resource_list Varchar(255))", ::callback, 0, &zErrMsg);
}

SqlLiteEventWriter::~SqlLiteEventWriter() {
  sqlite3_close(db);
  cout << "closed sqllite db" << endl;
}

int SqlLiteEventWriter::createTask(TaskID tid, SlaveID sid, Resources resVec) {
  currTime=time(NULL); /* get current cal time */
  string timestamp = asctime(localtime(&currTime));
  timestamp.erase(24); /* chop off the newline */
  stringstream ss;
  ss << "INSERT INTO task VALUES (";
  ss << tid << ",";
  ss << sid << ",";
  ss << getTimeStamp() << ",";
  ss << "'{\"cpus\":" << resVec.cpus << ",\"mem\":" << resVec.mem << "}'";
  ss << ")" << endl;
  cout << "executing " << ss.str() << endl;
  sqlite3_exec(db, ss.str().c_str(), callback, 0, &zErrMsg); 

  return 0;
}

/////////////EventLogger//////////
EventLogger::EventLogger() {
  Event event();
  logOnDestroy = false;
  cout << "creating FileEventWriter" << endl;
  FileEventWriter* fhl = new FileEventWriter();
  writers.push_back(fhl);
  writers.push_front(new SqlLiteEventWriter);
}

EventLogger::~EventLogger() {
  cout << "In ~EventLogger()" << endl;
  if (logOnDestroy) {
    writeEvent();
  }
  cout << ", cleaning up EventWriters" << endl;
  cout << "num of loggers in list: " << writers.size() << endl;
  EventWriter* fhl = writers.front();
  delete fhl;
  cout << "num of loggers in list: " << writers.size() << endl;
}

void EventLogger::setLogOnDestroy(bool newVal) {
  logOnDestroy = newVal; 
}

void EventLogger::writeEvent() {
  //This should loop through all writers in list and send event to each one
}

EventLogger EventLogger::operator() (string s1, string s2) {
  event.addAttribute(s1, s2);
  cout << "added attribute ( " << s1 << ", " << s2 << ") to event" << endl;
  EventLogger el(*this);
  el.setLogOnDestroy(true);
  return el;
}

/*int EventLogger::logEvent(int num_pairs, ...) {
  va_list args;
  va_start(args, num_pairs);
  //TODO: check that num_pairs is odd and positive 
  map<string,string> ev = map<string,string>();
  char * key, * val;
  //TODO: accept values other than strings, ints?
  for (int i = 0; i < num_pairs; i++) {
    key = va_arg(args, char *);
    val = va_arg(args, char *);
    ev[string(key)] = string(val);
  }
  EventWriter* fhl = writers.front();
  fhl->logEvent(ev);
  va_end(args);
}
*/

int EventLogger::createTask(TaskID tid, SlaveID sid, Resources resVec) {
  EventWriter* fhl = writers.front();
  fhl->createTask(tid, sid, resVec);
  cout << "logged createTask event in file event writer, tid: " << tid << endl;
}

//int main(int args, char** argv) {
//  EventLogger evLogger = EventLogger();
//  evLogger.logEvent(3, "test-key","test-val", "test-key2", "test-val2", "test-key3", "test-val3");
//  //future api ideas: 
//  //evLogger ("event-key-A","event-key-B");
//}
