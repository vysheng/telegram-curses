#include "Debug.hpp"

namespace tdcurses {

DebugCounters &debug_counters() {
  static DebugCounters instance{};
  return instance;
}

}  // namespace tdcurses
