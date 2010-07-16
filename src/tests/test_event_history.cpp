#include <gtest/gtest.h>
#include "master.hpp"
#include "event_history.hpp"
#include <sys/stat.h>

using nexus::FrameworkID;
using namespace nexus::internal::eventhistory;

EventLogger evLogger = EventLogger();

TEST(EventHistoryTest, logFrameworkRegistered)
{
  ASSERT_TRUE(GTEST_IS_THREADSAFE);
  FrameworkID fid = "MasterID-FrameworkID";
  evLogger.logFrameworkRegistered(fid, "UserID");
  // make sure log file, and sqlite3 db were created
  // and are  regular file s
  struct stat sb;
  ASSERT_NE(stat("logs/event_history_log.txt", &sb), -1);
  ASSERT_TRUE(S_ISREG(sb.st_mode));

  ASSERT_NE(stat("logs/event_history_db.sqlite3", &sb), -1);
  ASSERT_TRUE(S_ISREG(sb.st_mode));
}

