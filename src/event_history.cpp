#include "event_history.hpp"
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdarg.h>

#include <glog/logging.h>

//returns the current time in microseconds
long getTimeStamp(){
  struct timeval curr_time;
  struct timezone tzp;
  gettimeofday(&curr_time, &tzp);
  return (long)(curr_time.tv_sec * 1000000 + curr_time.tv_usec);
}

//returns a human readable timestamp
string getHumanReadableTimeStamp(){
  time_t currTime; /* calendar time */
  currTime=time(NULL); /* get current cal time */
  string timestamp = asctime(localtime(&currTime));
  return timestamp.erase(24); /* chop off the newline */
}

//////////FileEventWriter/////////
FileEventWriter::FileEventWriter() {
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

string FileEventWriter::getName() {
  return "File Event Writer";
}

FileEventWriter::~FileEventWriter() {
  logfile.close();
  cout << "closed log file" << endl;
}

int FileEventWriter::logTaskCreated(TaskID tid, FrameworkID fwid, SlaveID sid, Resources resVec) {
  logfile << getHumanReadableTimeStamp() << ",CreateTask, " 
          << "taskid: " << tid << ", "
          << "fwid: " << fwid << ", "
          << "sid: " << sid << ","
          << "cpus: " << resVec.cpus << ", mem: " << resVec.mem << endl;

  return 0;
}

int FileEventWriter::logTaskStateUpdated(TaskID tid, FrameworkID fwid, TaskState state) {
  logfile << getHumanReadableTimeStamp() << ", TaskStateUpdate, " 
          << "taskid: " << tid << ", "
          << "fwid: " << fwid << ", "
          << "state: " << state << endl;

  return 0;
}

int FileEventWriter::logFrameworkRegistered(FrameworkID fwid, string user) {
  logfile << getHumanReadableTimeStamp() << ", CreateFramework, "
          << "fwid: " << fwid << ", "
          << "userid: " << user << endl;

  return 0;
}

int FileEventWriter::logFrameworkUnregistered(FrameworkID fwid) {
  LOG(FATAL) << "FileEventWriter::logFrameworkUnregistered not implemented yet";
  return -1;
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

string SqlLiteEventWriter::getName(){
  return "Sqlite Event Writer";
}

SqlLiteEventWriter::SqlLiteEventWriter() {
  zErrMsg = 0;
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
  int rc = sqlite3_open("logs/event_history_db.sqlite3",&db);
  if( rc ) {
    cerr << "ERROR: Can't open database: " << sqlite3_errmsg(db) << endl;
    sqlite3_close(db);
    //exit(1);
  } else {
    cout << "opened sql lite db" << endl;
  }
  //create task table in case it doesn't already exist,
  //if it does this shouldn't destroy it
  sqlite3_exec(db, "CREATE TABLE task (taskid Varchar(255), fwid Varchar(255), sid Varchar(255), datetime_created integer, resource_list Varchar(255))", ::callback, 0, &zErrMsg);
  sqlite3_exec(db, "CREATE TABLE taskstate (taskid Varchar(255), fwid Varchar(255), state Varchar(255), datetime_updated integer)", ::callback, 0, &zErrMsg);

  sqlite3_exec(db, "CREATE TABLE framework (fwid Varchar(255), user Varchar(255), datetime_registered integer)", ::callback, 0, &zErrMsg);
}

SqlLiteEventWriter::~SqlLiteEventWriter() {
  sqlite3_close(db);
  cout << "closed sqllite db" << endl;
}

int SqlLiteEventWriter::logTaskCreated(TaskID tid, FrameworkID fwid, SlaveID sid, Resources resVec) {
  stringstream ss;
  ss << "INSERT INTO task VALUES ("
     << "\"" << tid << "\"" << ","
     << "\"" << fwid << "\"" << ","
     << "\"" << sid << "\"" << ","
     << getTimeStamp() << ","
     << "'{"
       << "\"cpus\":\"" << resVec.cpus << "\","
       << "\"mem\":\"" << resVec.mem << "\""
     << "}'"
     << ")" << endl;
  cout << "executing " << ss.str() << endl;
  sqlite3_exec(db, ss.str().c_str(), callback, 0, &zErrMsg); 

  return 0;
}

int SqlLiteEventWriter::logTaskStateUpdated(TaskID tid, FrameworkID fwid, TaskState state) {
  stringstream ss;
  ss << "INSERT INTO taskstate VALUES ("
     << "\"" << tid << "\"" << ","
     << "\"" << fwid << "\"" << ","
     << "\"" << state << "\"" << ","
     << getTimeStamp() << ")" << endl;
  cout << "executing " << ss.str() << endl;
  sqlite3_exec(db, ss.str().c_str(), callback, 0, &zErrMsg); 

  return 0;
}

int SqlLiteEventWriter::logFrameworkRegistered(FrameworkID fwid, string user) {
  stringstream ss;
  ss << "INSERT INTO framework VALUES ("
     << "\"" << fwid << "\"" << ","
     << "\"" << user << "\"" << ","
     << getTimeStamp() << ")" << endl;
  cout << "executing " << ss.str() << endl;
  sqlite3_exec(db, ss.str().c_str(), callback, 0, &zErrMsg); 

  return 0;
}

int SqlLiteEventWriter::logFrameworkUnregistered(FrameworkID fwid) {
  LOG(FATAL) << "SqlLiteEvent::logFrameworkUnregistered not implemented yet";
  return -1;
}

/////////////EventLogger//////////
EventLogger::EventLogger() {
  cout << "creating FileEventWriter" << endl;
  writers.push_front(new FileEventWriter());
  writers.push_front(new SqlLiteEventWriter);
}

EventLogger::~EventLogger() {
  cout << "In ~EventLogger()" << endl;
  cout << ", cleaning up EventWriters" << endl;
  cout << "num of loggers in list: " << writers.size() << endl;
  //delete all eventWriters in list
  list<EventWriter*>::iterator it;
  for (it = writers.begin(); it != writers.end(); it++) {
    delete *it;
  } 
  cout << "num of loggers in list: " << writers.size() << endl;
}

int EventLogger::logFrameworkRegistered(FrameworkID fwid, string user) {
  list<EventWriter*>::iterator it;
  for (it = writers.begin(); it != writers.end(); it++) {
    (*it)->logFrameworkRegistered(fwid, user);
    cout << "logged FrameworkRegistered event with " << (*it)->getName() << ". fwid: " << fwid << ", user: " << user << endl;
  }
}

int EventLogger::logFrameworkUnregistered(FrameworkID fwid) {
  list<EventWriter*>::iterator it;
  for (it = writers.begin(); it != writers.end(); it++) {
    (*it)->logFrameworkUnregistered(fwid);
    cout << "logged FrameworkUnregistered event with " << (*it)->getName() << ". fwid: " << fwid << endl;
  }
}

int EventLogger::logTaskCreated(TaskID tid, FrameworkID fwid, SlaveID sid, Resources resVec) {
  list<EventWriter*>::iterator it;
  for (it = writers.begin(); it != writers.end(); it++) {
    (*it)->logTaskCreated(tid, fwid, sid, resVec);
    cout << "logged TaskCreated event with " << (*it)->getName() << endl;
  } 
}

int EventLogger::logTaskStateUpdated(TaskID tid, FrameworkID fwid, TaskState state) {
  list<EventWriter*>::iterator it;
  for (it = writers.begin(); it != writers.end(); it++) {
    (*it)->logTaskStateUpdated(tid, fwid, state);
    cout << "logged TaskStateUpated event with " << (*it)->getName() << endl;
  } 
}
