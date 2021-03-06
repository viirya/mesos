#ifndef __PROCESS_PROTOBUF_HPP__
#define __PROCESS_PROTOBUF_HPP__

#include <glog/logging.h>

#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>

#include <set>
#include <vector>

#include <tr1/functional>
#include <tr1/unordered_map>

#include <process/dispatch.hpp>
#include <process/process.hpp>


// Provides an implementation of process::post that for a protobuf.
namespace process {

inline void post(const process::UPID& to,
                 const google::protobuf::Message& message)
{
  std::string data;
  message.SerializeToString(&data);
  post(to, message.GetTypeName(), data.data(), data.size());
}

} // namespace process {


// The rest of this file provides libprocess "support" for using
// protocol buffers. In particular, this file defines a subclass of
// Process (ProtobufProcess) that allows you to install protocol
// buffer handlers in addition to normal message and HTTP
// handlers. Note that this header file assumes you will be linking
// against BOTH libprotobuf and libglog.

namespace google { namespace protobuf {

// Type conversions helpful for changing between protocol buffer types
// and standard C++ types (for parameters).
template <typename T>
const T& convert(const T& t)
{
  return t;
}


template <typename T>
std::vector<T> convert(const google::protobuf::RepeatedPtrField<T>& items)
{
  std::vector<T> result;
  for (int i = 0; i < items.size(); i++) {
    result.push_back(items.Get(i));
  }

  return result;
}

}} // namespace google { namespace protobuf {


template <typename T>
class ProtobufProcess : public process::Process<T>
{
public:
  ProtobufProcess(const std::string& id = "")
    : process::Process<T>(id) {}

  virtual ~ProtobufProcess() {}

protected:
  virtual void operator () ()
  {
    // TODO(benh): Shouldn't we just make Process::serve be a virtual
    // function, and then the one we get from process::Process will be
    // sufficient?
    do { if (serve() == process::TERMINATE) break; } while (true);
  }

  template <typename M>
  M message()
  {
    M m;
    m.ParseFromString(process::Process<T>::body());
    return m;
  }

  void send(const process::UPID& to,
            const google::protobuf::Message& message)
  {
    std::string data;
    message.SerializeToString(&data);
    process::Process<T>::send(to, message.GetTypeName(),
                              data.data(), data.size());
  }

  using process::Process<T>::send;

  const std::string& serve(double secs = 0, bool once = false)
  {
    do {
      const std::string& name = process::Process<T>::serve(secs, once);
      if (protobufHandlers.count(name) > 0) {
        protobufHandlers[name](process::Process<T>::body());
      } else {
        return name;
      }
    } while (!once);
  }

  template <typename M>
  void installProtobufHandler(void (T::*method)(const M&))
  {
    google::protobuf::Message* m = new M();
    T* t = static_cast<T*>(this);
    protobufHandlers[m->GetTypeName()] =
      std::tr1::bind(&handlerM<M>,
                     t, method,
                     std::tr1::placeholders::_1);
    delete m;
  }

  template <typename M>
  void installProtobufHandler(void (T::*method)())
  {
    google::protobuf::Message* m = new M();
    T* t = static_cast<T*>(this);
    protobufHandlers[m->GetTypeName()] =
      std::tr1::bind(&handler0,
                     t, method,
                     std::tr1::placeholders::_1);
    delete m;
  }

  template <typename M,
            typename P1, typename P1C>
  void installProtobufHandler(void (T::*method)(P1C),
                              P1 (M::*param1)() const)
  {
    google::protobuf::Message* m = new M();
    T* t = static_cast<T*>(this);
    protobufHandlers[m->GetTypeName()] =
      std::tr1::bind(&handler1<M, P1, P1C>,
                     t, method, param1,
                     std::tr1::placeholders::_1);
    delete m;
  }

  template <typename M,
            typename P1, typename P1C,
            typename P2, typename P2C>
  void installProtobufHandler(void (T::*method)(P1C, P2C),
                              P1 (M::*p1)() const,
                              P2 (M::*p2)() const)
  {
    google::protobuf::Message* m = new M();
    T* t = static_cast<T*>(this);
    protobufHandlers[m->GetTypeName()] =
      std::tr1::bind(&handler2<M, P1, P1C, P2, P2C>,
                     t, method, p1, p2,
                     std::tr1::placeholders::_1);
    delete m;
  }

  template <typename M,
            typename P1, typename P1C,
            typename P2, typename P2C,
            typename P3, typename P3C>
  void installProtobufHandler(void (T::*method)(P1C, P2C, P3C),
                              P1 (M::*p1)() const,
                              P2 (M::*p2)() const,
                              P3 (M::*p3)() const)
  {
    google::protobuf::Message* m = new M();
    T* t = static_cast<T*>(this);
    protobufHandlers[m->GetTypeName()] =
      std::tr1::bind(&handler3<M, P1, P1C, P2, P2C, P3, P3C>,
                     t, method, p1, p2, p3,
                     std::tr1::placeholders::_1);
    delete m;
  }

  template <typename M,
            typename P1, typename P1C,
            typename P2, typename P2C,
            typename P3, typename P3C,
            typename P4, typename P4C>
  void installProtobufHandler(void (T::*method)(P1C, P2C, P3C, P4C),
                              P1 (M::*p1)() const,
                              P2 (M::*p2)() const,
                              P3 (M::*p3)() const,
                              P4 (M::*p4)() const)
  {
    google::protobuf::Message* m = new M();
    T* t = static_cast<T*>(this);
    protobufHandlers[m->GetTypeName()] =
      std::tr1::bind(&handler4<M, P1, P1C, P2, P2C, P3, P3C, P4, P4C>,
                     t, method, p1, p2, p3, p4,
                     std::tr1::placeholders::_1);
    delete m;
  }

  template <typename M,
            typename P1, typename P1C,
            typename P2, typename P2C,
            typename P3, typename P3C,
            typename P4, typename P4C,
            typename P5, typename P5C>
  void installProtobufHandler(void (T::*method)(P1C, P2C, P3C, P4C, P5C),
                              P1 (M::*p1)() const,
                              P2 (M::*p2)() const,
                              P3 (M::*p3)() const,
                              P4 (M::*p4)() const,
                              P5 (M::*p5)() const)
  {
    google::protobuf::Message* m = new M();
    T* t = static_cast<T*>(this);
    protobufHandlers[m->GetTypeName()] =
      std::tr1::bind(&handler5<M, P1, P1C, P2, P2C, P3, P3C, P4, P4C, P5, P5C>,
                     t, method, p1, p2, p3, p4, p5,
                     std::tr1::placeholders::_1);
    delete m;
  }

private:
  template <typename M>
  static void handlerM(T* t, void (T::*method)(const M&),
                       const std::string& data)
  {
    M m;
    m.ParseFromString(data);
    if (m.IsInitialized()) {
      (t->*method)(m);
    } else {
      LOG(WARNING) << "Initialization errors: "
                   << m.InitializationErrorString();
    }
  }

  static void handler0(T* t, void (T::*method)(),
                       const std::string& data)
  {
    (t->*method)();
  }

  template <typename M,
            typename P1, typename P1C>
  static void handler1(T* t, void (T::*method)(P1C),
                       P1 (M::*p1)() const,
                       const std::string& data)
  {
    M m;
    m.ParseFromString(data);
    if (m.IsInitialized()) {
      (t->*method)(google::protobuf::convert((&m->*p1)()));
    } else {
      LOG(WARNING) << "Initialization errors: "
                   << m.InitializationErrorString();
    }
  }

  template <typename M,
            typename P1, typename P1C,
            typename P2, typename P2C>
  static void handler2(T* t, void (T::*method)(P1C, P2C),
                       P1 (M::*p1)() const,
                       P2 (M::*p2)() const,
                       const std::string& data)
  {
    M m;
    m.ParseFromString(data);
    if (m.IsInitialized()) {
      (t->*method)(google::protobuf::convert((&m->*p1)()),
                   google::protobuf::convert((&m->*p2)()));
    } else {
      LOG(WARNING) << "Initialization errors: "
                   << m.InitializationErrorString();
    }
  }

  template <typename M,
            typename P1, typename P1C,
            typename P2, typename P2C,
            typename P3, typename P3C>
  static void handler3(T* t, void (T::*method)(P1C, P2C, P3C),
                       P1 (M::*p1)() const,
                       P2 (M::*p2)() const,
                       P3 (M::*p3)() const,
                       const std::string& data)
  {
    M m;
    m.ParseFromString(data);
    if (m.IsInitialized()) {
      (t->*method)(google::protobuf::convert((&m->*p1)()),
                   google::protobuf::convert((&m->*p2)()),
                   google::protobuf::convert((&m->*p3)()));
    } else {
      LOG(WARNING) << "Initialization errors: "
                   << m.InitializationErrorString();
    }
  }

  template <typename M,
            typename P1, typename P1C,
            typename P2, typename P2C,
            typename P3, typename P3C,
            typename P4, typename P4C>
  static void handler4(T* t, void (T::*method)(P1C, P2C, P3C, P4C),
                       P1 (M::*p1)() const,
                       P2 (M::*p2)() const,
                       P3 (M::*p3)() const,
                       P4 (M::*p4)() const,
                       const std::string& data)
  {
    M m;
    m.ParseFromString(data);
    if (m.IsInitialized()) {
      (t->*method)(google::protobuf::convert((&m->*p1)()),
                   google::protobuf::convert((&m->*p2)()),
                   google::protobuf::convert((&m->*p3)()),
                   google::protobuf::convert((&m->*p4)()));
    } else {
      LOG(WARNING) << "Initialization errors: "
                   << m.InitializationErrorString();
    }
  }

  template <typename M,
            typename P1, typename P1C,
            typename P2, typename P2C,
            typename P3, typename P3C,
            typename P4, typename P4C,
            typename P5, typename P5C>
  static void handler5(T* t, void (T::*method)(P1C, P2C, P3C, P4C, P5C),
                       P1 (M::*p1)() const,
                       P2 (M::*p2)() const,
                       P3 (M::*p3)() const,
                       P4 (M::*p4)() const,
                       P5 (M::*p5)() const,
                       const std::string& data)
  {
    M m;
    m.ParseFromString(data);
    if (m.IsInitialized()) {
      (t->*method)(google::protobuf::convert((&m->*p1)()),
                   google::protobuf::convert((&m->*p2)()),
                   google::protobuf::convert((&m->*p3)()),
                   google::protobuf::convert((&m->*p4)()),
                   google::protobuf::convert((&m->*p5)()));
    } else {
      LOG(WARNING) << "Initialization errors: "
                   << m.InitializationErrorString();
    }
  }

  typedef std::tr1::function<void(const std::string&)> handler;
  std::tr1::unordered_map<std::string, handler> protobufHandlers;
};


// Implements a process for sending protobuf "requests" to a process
// and waiting for a protobuf "response", but uses futures so that
// this can be done without needing to implement a process.
template <typename Req, typename Res>
class ReqResProcess
  : public ProtobufProcess<ReqResProcess<Req, Res> >
{
public:
  typedef ProtobufProcess<ReqResProcess<Req, Res> > Super;

  ReqResProcess(const process::UPID& _pid, const Req& _req)
    : pid(_pid), req(_req)
  {
    Super::template installProtobufHandler<Res>(
        &ReqResProcess<Req, Res>::response);
  }

  process::Promise<Res> run()
  {
    send(pid, req);
    std::tr1::function<void(const process::Future<Res>&)> callback =
      std::tr1::bind(&ReqResProcess<Req, Res>::discard,
                     std::tr1::placeholders::_1, Super::self());
    promise.future().onDiscarded(callback);
    return promise;
  }

private:
  void response(const Res& res) { promise.set(res); }

  static void discard(
      const process::Future<Res>& future,
      const process::PID<ReqResProcess<Req, Res> >& pid)
  {
    process::terminate(pid);
  }

  const process::UPID pid;
  const Req req;
  process::Promise<Res> promise;
};


// Allows you to describe request/response protocols and then use
// those for sending requests and getting back responses.
template <typename Req, typename Res>
struct Protocol
{
  process::Future<Res> operator () (
      const process::UPID& pid,
      const Req& req) const
  {
    // Help debugging by adding some "type constraints".
    { Req* req = NULL; google::protobuf::Message* m = req; }
    { Res* res = NULL; google::protobuf::Message* m = res; }

    ReqResProcess<Req, Res>* process = new ReqResProcess<Req, Res>(pid, req);
    process::spawn(process, true);
    return process::dispatch(process, &ReqResProcess<Req, Res>::run);
  }
};


// A "group" is a collection of protobuf processes (both local and
// remote). A group can be used to abstract away the details of
// maintaining membership as processes come and go (e.g., due to
// failures).
class GroupProcess : public ProtobufProcess<GroupProcess>
{
public:
  GroupProcess() {}

  GroupProcess(const std::set<process::UPID>& _pids) : pids(_pids) {}

  virtual ~GroupProcess() {}

  void add(const process::UPID& pid)
  {
    link(pid);
    pids.insert(pid);
  }

  void remove(const process::UPID& pid)
  {
    // TODO(benh): unlink(pid);
    pids.erase(pid);
  }

  // Returns the current set of known members.
  std::set<process::UPID> members()
  {
    return pids;
  }

  // Sends a request to each of the groups members and returns a set
  // of futures that represent their responses.
  template <typename Req, typename Res>
  std::set<process::Future<Res> > broadcast(
      const Protocol<Req, Res>& protocol,
      const Req& req,
      const std::set<process::UPID>& filter = std::set<process::UPID>())
  {
    std::set<process::Future<Res> > futures;
    typename std::set<process::UPID>::const_iterator iterator;
    for (iterator = pids.begin(); iterator != pids.end(); ++iterator) {
      const process::UPID& pid = *iterator;
      if (filter.count(pid) == 0) {
        futures.insert(protocol(pid, req));
      }
    }
    return futures;
  }

  template <typename M>
  void broadcast(
      const M& m,
      const std::set<process::UPID>& filter = std::set<process::UPID>())
  {
    std::set<process::UPID>::const_iterator iterator;
    for (iterator = pids.begin(); iterator != pids.end(); ++iterator) {
      const process::UPID& pid = *iterator;
      if (filter.count(pid) == 0) {
        process::post(pid, m);
      }
    }
  }

private:
  // Not copyable, not assignable.
  GroupProcess(const GroupProcess&);
  GroupProcess& operator = (const GroupProcess&);

  std::set<process::UPID> pids;
};

#endif // __PROCESS_PROTOBUF_HPP__
