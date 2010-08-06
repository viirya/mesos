#include <sstream>

#include "slave_webui.hpp"
#include "slave_state.hpp"

#ifdef MESOS_WEBUI

#include <process.hpp>
#include <pthread.h>
#include <Python.h>

extern "C" void init_slave();  // Initializer for the Python slave module

namespace {

PID slave;

}

namespace mesos { namespace internal { namespace slave {

using namespace std;

struct webuiArgs {
  string webuiPort;
  string logDir;
  string workDir;
} myWebuiArgs; 

void *runSlaveWebUI(void *)
{
  LOG(INFO) << "Web UI thread started";
  Py_Initialize();
  char* nargv[4]; 
  nargv[0] = const_cast<char*>("webui/slave/webui.py");
  nargv[1] = const_cast<char*>(myWebuiArgs.webuiPort.c_str());
  nargv[2] = const_cast<char*>(myWebuiArgs.logDir.c_str());
  nargv[3] = const_cast<char*>(myWebuiArgs.workDir.c_str());
  PySys_SetArgv(4,nargv);
  PyRun_SimpleString("import sys\n"
      "sys.path.append('webui/slave/swig')\n"
      "sys.path.append('webui/common')\n"
      "sys.path.append('third_party/bottle-0.5.6')\n");
  init_slave();
  LOG(INFO) << "Loading webui/slave/webui.py";
  FILE *webui = fopen("webui/slave/webui.py", "r");
  PyRun_SimpleFile(webui, "webui/slave/webui.py");
  fclose(webui);
  Py_Finalize();
}


void startSlaveWebUI(const PID &slave, const Params &params)
{
  myWebuiArgs.webuiPort = params.get("webui_port","8081");
  myWebuiArgs.logDir = params.get("log_dir","/tmp");
  myWebuiArgs.workDir = params.get("work_dir","/tmp");
  LOG(INFO) << "Starting slave web UI on port " << myWebuiArgs.webuiPort 
            << ", using log_dir " << myWebuiArgs.logDir;
  ::slave = slave;
  pthread_t thread;
  pthread_create(&thread, 0, runSlaveWebUI, NULL);
}


namespace state {

class StateGetter : public Tuple<Process>
{
public:
  SlaveState *slaveState;

  StateGetter() {}
  ~StateGetter() {}

  void operator () ()
  {
    send(::slave, pack<S2S_GET_STATE>());
    receive();
    CHECK(msgid() == S2S_GET_STATE_REPLY);
    unpack<S2S_GET_STATE_REPLY>(*((intptr_t *) &slaveState));
  }
};


// From slave_state.hpp
SlaveState *get_slave()
{
  StateGetter getter;
  PID pid = Process::spawn(&getter);
  Process::wait(pid);
  return getter.slaveState;
}

} /* namespace state { */

}}} /* namespace mesos { namespace internal { namespace slave { */


#endif /* MESOS_WEBUI */
