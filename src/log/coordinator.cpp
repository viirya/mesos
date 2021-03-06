#include <algorithm>

#include <process/dispatch.hpp>
#include <process/future.hpp>
#include <process/timeout.hpp>

#include "common/foreach.hpp"
#include "common/option.hpp"

#include "log/coordinator.hpp"
#include "log/replica.hpp"

using std::list;
using std::pair;
using std::set;
using std::string;


namespace mesos { namespace internal { namespace log {

Coordinator::Coordinator(int _quorum,
                         ReplicaProcess* _replica,
                         GroupProcess* _group)
  : elected(false),
    quorum(_quorum),
    replica(_replica),
    group(_group),
    id(0),
    index(0) {}


Coordinator::~Coordinator() {}


Result<uint64_t> Coordinator::elect(uint64_t _id)
{
  CHECK(!elected);

  id = _id;

  PromiseRequest request;
  request.set_id(id);

  // Broadcast the request to the group.
  set<Future<PromiseResponse> > futures =
    broadcast(protocol::promise, request);

  Option<Future<PromiseResponse> > option;
  int okays = 0;
  
  Timeout timeout = 1.0;

  do {
    option = select(futures, timeout.remaining());
    if (option.isSome()) {
      CHECK(option.get().ready());
      const PromiseResponse& response = option.get().get();
      if (!response.okay()) {
        return Result<uint64_t>::error("Coordinator demoted");
      } else if (response.okay()) {
        CHECK(response.has_position());
        index = std::max(index, response.position());
        okays++;
        if (okays >= quorum) {
          break;
        }
      }
      futures.erase(option.get());
    }
  } while (option.isSome());

  // Discard the remaining futures.
  discard(futures);

  // Either we have a quorum or we timed out.
  if (okays >= quorum) {
    elected = true;

    // Need to "catchup" local replica (i.e., fill in any unlearned
    // and/or missing positions) so that we can do local reads.
    // Usually we could do this lazily, however, a local learned
    // position might have been truncated, so we actually need to
    // catchup the local replica all the way to the end of the log
    // before we can perform any local reads.
    // TODO(benh): Dispatch and timed wait on future instead?
    set<uint64_t> positions = call(replica, &ReplicaProcess::missing, index);

    foreach (uint64_t position, positions) {
      Result<Action> result = fill(position);
      if (result.isError()) {
        return Result<uint64_t>::error(result.error());
      } else if (result.isNone()) {
        return Result<uint64_t>::none();
      } else {
        CHECK(result.isSome());
        CHECK(result.get().position() == position);
      }
    }

    index += 1;
    return index - 1;
  }

  // Timed out ...
  return Result<uint64_t>::none();
}


Result<uint64_t> Coordinator::demote()
{
  elected = false;
  return index - 1;
}


Result<uint64_t> Coordinator::append(const string& bytes)
{
  if (!elected) {
    return Result<uint64_t>::error("Coordinator not elected");
  }

  Action action;
  action.set_position(index);
  action.set_promised(id);
  action.set_performed(id);
  action.set_type(Action::APPEND);
  Action::Append* append = action.mutable_append();
  append->set_bytes(bytes);

  Result<uint64_t> result = write(action);

  if (result.isSome()) {
    CHECK(result.get() == index);
    index++;
  }

  return result;
}


Result<uint64_t> Coordinator::truncate(uint64_t to)
{
  if (!elected) {
    return Result<uint64_t>::error("Coordinator not elected");
  }

  Action action;
  action.set_position(index);
  action.set_promised(id);
  action.set_performed(id);
  action.set_type(Action::TRUNCATE);
  Action::Truncate* truncate = action.mutable_truncate();
  truncate->set_to(to);

  Result<uint64_t> result = write(action);

  if (result.isSome()) {
    CHECK(result.get() == index);
    index++;
  }

  return result;
}


Result<list<pair<uint64_t, string> > > Coordinator::read(
    uint64_t from,
    uint64_t to)
{
  LOG(INFO) << "Coordinator requested read from " << from << " -> " << to;

  if (!elected) {
    return Result<list<pair<uint64_t, string> > >::error(
        "Coordinator not elected");
  } else if (from == 0) { // TODO(benh): Fix this hack!
    return Result<list<pair<uint64_t, string> > >::error(
        "Bad read range (from == 0)");
  } else if (to < from) {
    return Result<list<pair<uint64_t, string> > >::error(
        "Bad read range (to < from)");
  } else if (index <= from) {
    return Result<list<pair<uint64_t, string> > >::error(
        "Bad read range (index <= from)");
  }

  list<Future<Result<Action> > > futures;

  for (uint64_t position = from; position <= to; position++) {
    futures.push_back(dispatch(replica, &ReplicaProcess::read, position));
  }

  // TODO(benh): Implement 'collect' for lists of futures, use below.

  // Collect actions for each position.
  list<Action> actions;

  foreach (const Future<Result<Action> >& future, futures) {
    future.await(); // TODO(benh): Timeout?
    CHECK(future.ready());
    const Result<Action>& result = future.get();
    if (result.isError()) {
      return Result<list<pair<uint64_t, string> > >::error(result.error());
    } else if (result.isNone()) {
      LOG(FATAL) << "Coordinator's local replica missing positions!";
    } else {
      CHECK(result.isSome());
      const Action& action = result.get();
      CHECK(action.has_learned() && action.learned())
        << "Coordinator's local replica has unlearned positions!";
      actions.push_back(action);
    }
  }

  // Filter out all the no-ops and truncates. TODO(benh): Get
  // convinced that there can't be a truncate action someplace else in
  // the log which would eliminate what we are about to return!
  list<pair<uint64_t, string> > appends;

  foreach (const Action& action, actions) {
    CHECK(action.has_performed());
    CHECK(action.has_learned() && action.learned());
    CHECK(action.has_type());
    if (action.type() == Action::APPEND) {
      appends.push_back(make_pair(action.position(), action.append().bytes()));
    }
  }

  return appends;
}


Result<uint64_t> Coordinator::write(const Action& action)
{
  LOG(INFO) << "Coordinator attempting to write "
            << Action::Type_Name(action.type())
            << " action at position " << action.position();

  CHECK(elected);

  CHECK(action.has_performed());
  CHECK(action.has_type());

  // TODO(benh): Eliminate this special case hack?
  if (quorum == 1) {
    Result<uint64_t> result = commit(action);
    if (result.isError()) {
      return Result<uint64_t>::error(result.error());
    } else if (result.isNone()) {
      return Result<uint64_t>::none();
    } else {
      CHECK(result.isSome());
      return action.position();
    }
  }

  WriteRequest request;
  request.set_id(id);
  request.set_position(action.position());
  request.set_type(action.type());
  switch (action.type()) {
    case Action::NOP:
      CHECK(action.has_nop());
      request.mutable_nop();
      break;
    case Action::APPEND:
      CHECK(action.has_append());
      request.mutable_append()->MergeFrom(action.append());
      break;
    case Action::TRUNCATE:
      CHECK(action.has_truncate());
      request.mutable_truncate()->MergeFrom(action.truncate());
      break;
    default:
      LOG(FATAL) << "Unknown Action::Type!";
  }

  // Broadcast the request to the group *excluding* the local replica.
  set<Future<WriteResponse> > futures =
    remotecast(protocol::write, request);

  Option<Future<WriteResponse> > option;
  int okays = 0;

  Timeout timeout = 1.0;

  do {
    option = select(futures, timeout.remaining());
    if (option.isSome()) {
      CHECK(option.get().ready());
      const WriteResponse& response = option.get().get();
      CHECK(response.id() == request.id());
      CHECK(response.position() == request.position());

      if (!response.okay()) {
        return Result<uint64_t>::error("Coordinator demoted");
      } else if (response.okay()) {
        if (++okays >= (quorum - 1)) { // N.B. Using (quorum - 1) here!
          // Got enough remote okays, discard the remaining futures
          // and try and commit the action locally.
          discard(futures);
          Result<uint64_t> result = commit(action);
          if (result.isError()) {
            return Result<uint64_t>::error(result.error());
          } else if (result.isNone()) {
            return Result<uint64_t>::none();
          } else {
            CHECK(result.isSome());
            return action.position();
          }
        }
      }
      futures.erase(option.get());
    }
  } while (option.isSome());

  // Timed out ...
  discard(futures);
  return Result<uint64_t>::none();
}


Result<uint64_t> Coordinator::commit(const Action& action)
{
  CHECK(elected);

  CommitRequest request;
  request.set_id(id);
  request.set_position(action.position());
  request.set_type(action.type());
  switch (action.type()) {
    case Action::NOP:
      CHECK(action.has_nop());
      request.mutable_nop();
      break;
    case Action::APPEND:
      CHECK(action.has_append());
      request.mutable_append()->MergeFrom(action.append());
      break;
    case Action::TRUNCATE:
      CHECK(action.has_truncate());
      request.mutable_truncate()->MergeFrom(action.truncate());
      break;
    default:
      LOG(FATAL) << "Unknown Action::Type!";
  }

  Future<CommitResponse> future = protocol::commit(replica->self(), request);

  Timeout timeout = 1.0;

  if (!future.await(timeout.remaining())) {
    future.discard();
    return Result<uint64_t>::none();
  }

  const CommitResponse& response = future.get();
  CHECK(response.id() == request.id());
  CHECK(response.position() == request.position());

  if (!response.okay()) {
    return Result<uint64_t>::error("Coordinator demoted");
  }

  // Commit successful, send a learned message to the group
  // *excluding* the local replica and return the position.

  LearnedMessage message;
  message.mutable_action()->MergeFrom(action);

  if (!action.has_learned() || !action.learned()) {
    message.mutable_action()->set_learned(true);
  }

  remotecast(message);

  return action.position();
}


Result<Action> Coordinator::fill(uint64_t position)
{
  LOG(INFO) << "Coordinator attempting to fill position "
            << position << " in the log";

  CHECK(elected);

  PromiseRequest request;
  request.set_id(id);
  request.set_position(position);

  // Broadcast the request to the group.
  set<Future<PromiseResponse> > futures =
    broadcast(protocol::promise, request);

  Option<Future<PromiseResponse> > option;
  list<PromiseResponse> responses;

  Timeout timeout = 1.0;

  do {
    option = select(futures, timeout.remaining());
    if (option.isSome()) {
      CHECK(option.get().ready());
      const PromiseResponse& response = option.get().get();
      CHECK(response.id() == request.id());
      if (!response.okay()) {
        return Result<Action>::error("Coordinator demoted");
      } else if (response.okay()) {
        responses.push_back(response);
        if (responses.size() >= quorum) {
          break;
        }
      }
      futures.erase(option.get());
    }
  } while (option.isSome());

  // Discard the remaining futures.
  discard(futures);

  // Either have a quorum or we timed out.
  if (responses.size() >= quorum) {
    // Check the responses for a learned action, otherwise, pick the
    // action with the higest performed id or a no-op if no responses
    // include performed actions.
    Action action;
    foreach (const PromiseResponse& response, responses) {
      if (response.has_action()) {
        CHECK(response.action().position() == position);
        if (response.action().has_learned() && response.action().learned()) {
          // Received a learned action, try and commit locally.
          Result<uint64_t> result = commit(response.action());
          if (result.isError()) {
            return Result<Action>::error(result.error());
          } else if (result.isNone()) {
            return Result<Action>::none();
          } else {
            CHECK(result.isSome());
            return response.action();
          }
        } else if (response.action().has_performed() &&
                   (!action.has_performed() ||
                    response.action().performed() > action.performed())) {
          action = response.action();
        }
      } else {
        CHECK(response.has_position());
        CHECK(response.position() == position);
      }
    }

    // Use a no-op if no known action has been performed.
    if (!action.has_performed()) {
      action.set_position(position);
      action.set_promised(id);
      action.set_performed(id);
      action.set_type(Action::NOP);
      action.mutable_nop();
    } else {
      action.set_performed(id);
    }

    Result<uint64_t> result = write(action);

    if (result.isError()) {
      return Result<Action>::error(result.error());
    } else if (result.isNone()) {
      return Result<Action>::none();
    } else {
      CHECK(result.isSome());
      return action;
    }
  }

  // Timed out ...
  return Result<Action>::none();
}


template <typename Req, typename Res>
set<Future<Res> > Coordinator::broadcast(
    const Protocol<Req, Res>& protocol,
    const Req& req)
{
  // TODO(benh): Dispatch and timed wait on future instead?
  return call(group,
              &GroupProcess::template broadcast<Req, Res>,
              protocol,
              req,
              set<UPID>());
}


template <typename Req, typename Res>
set<Future<Res> > Coordinator::remotecast(
    const Protocol<Req, Res>& protocol,
    const Req& req)
{
  set<UPID> filter;
  filter.insert(replica->self());

  // TODO(benh): Dispatch and timed wait on future instead?
  return call(group,
              &GroupProcess::template broadcast<Req, Res>,
              protocol,
              req,
              filter);
}


template <typename M>
void Coordinator::remotecast(const M& m)
{
  set<UPID> filter;
  filter.insert(replica->self());

  // Need to disambiguate overloaded function.
  void (GroupProcess::*broadcast) (const M&, const std::set<process::UPID>&);
  broadcast = &GroupProcess::broadcast<M>;

  dispatch(group, broadcast, m, filter);
}

}}} // namespace mesos { namespace internal { namespace log {
