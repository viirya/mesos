#include <gtest/gtest.h>
#include <sys/stat.h>

#include "event_history.hpp"
#include "master.hpp"
#include "params.hpp"

using mesos::FrameworkID;
using namespace mesos::internal::eventhistory;
using mesos::internal::Params;

TEST(EventHistoryTest, logFrameworkRegisteredDefaultParams)
{
  ASSERT_TRUE(GTEST_IS_THREADSAFE);

  EventLogger evLogger;
  FrameworkID fid = "MasterID-FrameworkID";
  evLogger.logFrameworkRegistered(fid, "UserID");
  // make sure log file, and sqlite3 db were created
  // and are regular files
  struct stat sb;
  ASSERT_NE(stat("logs/event_history_log.txt", &sb), -1);
  ASSERT_TRUE(S_ISREG(sb.st_mode));

  //TODO(andyk): check that framework registration got logged to textfile

  ASSERT_NE(stat("logs/event_history_db.sqlite3", &sb), -1);
  ASSERT_TRUE(S_ISREG(sb.st_mode));
 
  //TODO(andyk): check that framework registration got logged to sqlite
}


TEST(EventHistoryTest, logFrameworkRegisteredWithParams)
{
  ASSERT_TRUE(GTEST_IS_THREADSAFE);

  //TODO(andyk): load log_dir and sqlite/file flags into params file
  Params params;
  params["log_dir"] = "/tmp/test_mesos";
  params["sqlite-event-history"] = "true";
  EventLogger evLogger(params);
  FrameworkID fid = "MasterID-FrameworkID";
  evLogger.logFrameworkRegistered(fid, "UserID");
}
