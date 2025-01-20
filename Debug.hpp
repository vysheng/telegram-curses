#pragma once

#include "td/utils/common.h"

namespace tdcurses {

struct DebugCounters {
  td::int64 allocated_menu_windows;

  std::string to_str() const;
};

DebugCounters &debug_counters();

};  // namespace tdcurses
