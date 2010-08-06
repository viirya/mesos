#include <sstream>

#include "master_state.hpp"
#include "master_webui.hpp"

#ifdef MESOS_WEBUI

#include <pthread.h>
#include <Python.h>

extern "C" void init_master();  // Initializer for the Python master module

namespace {

PID master;

}

namespace mesos { namespace internal { namespace master {

using namespace std;

struct webuiArgs {
  string webuiPort;
  string logDir;
} myWebuiArgs; 


void *runMasterWebUI(void *)
{
  LOG(INFO) << "Web UI thread started";
  LOG(INFO) << "WEBUI AGAIN IS: " << myWebuiArgs.webuiPort;
  LOG(INFO) << "LOG_DIR AGAIN IS: " << myWebuiArgs.logDir;

  Py_Initialize();
  char* nargv[3]; 
  nargv[0] = const_cast<char*>("webui/master/webui.py");
  nargv[1] = const_cast<char*>(myWebuiArgs.webuiPort.c_str());
  nargv[2] = const_cast<char*>(myWebuiArgs.logDir.c_str());
  PySys_SetArgv(3,nargv);
  PyRun_SimpleString("import sys\n"
      "sys.path.append('webui/master/swig')\n"
      "sys.path.append('webui/common')\n"
      "sys.path.append('third_party/bottle-0.5.6')\n");
  init_master();
  LOG(INFO) << "Loading webui/master/webui.py";
  FILE *webui = fopen("webui/master/webui.py", "r");
  PyRun_SimpleFile(webui, "webui/master/webui.py");
  fclose(webui);
  Py_Finalize();
}


void startMasterWebUI(const PID &master, const Params& params)
{
  myWebuiArgs.webuiPort = params.get("webui_port","8080");
  myWebuiArgs.logDir = params.get("log_dir","/tmp");
  LOG(INFO) << "Starting master web UI on port " << myWebuiArgs.webuiPort 
            << ", using log_dir " << myWebuiArgs.logDir;
  ::master = master;
  pthread_t thread;
  pthread_create(&thread, 0, runMasterWebUI, NULL);
}


namespace state {

class StateGetter : public Tuple<Process>
{
public:
  MasterState *masterState;

  StateGetter() {}
  ~StateGetter() {}

  void operator () ()
  {
    send(::master, pack<M2M_GET_STATE>());
    receive();
    int64_t *i = (int64_t *) &masterState;
    unpack<M2M_GET_STATE_REPLY>(*i);
  }
};


// From master_state.hpp
MasterState *get_master()
{
  StateGetter getter;
  PID pid = Process::spawn(&getter);
  Process::wait(pid);
  return getter.masterState;
}

} /* namespace state { */

}}} /* namespace mesos { namespace internal { namespace master { */

#endif /* MESOS_WEBUI */
