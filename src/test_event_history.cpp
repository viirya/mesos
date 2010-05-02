/***
 * Compile with g++ test_event_history.cpp event_history.o
 ***/
#include "event_history.hpp"

int main(int args, char** argv) {
  EventLogger evLogger = EventLogger();
  evLogger.logEvent(3, "test-key","test-val", "test-key2", "test-val2", "test-key3", "test-val3");
  //future api ideas: 
  //evLogger ("event-key-A","event-key-B");
}
