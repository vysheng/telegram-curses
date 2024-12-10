#pragma once

#include "td/utils/common.h"
#include "td/utils/StringBuilder.h"
#include "td/utils/format.h"
#include "FileManager.hpp"

namespace tdcurses {

struct DebugCounters {
  td::int64 allocated_menu_windows;

  std::string to_str() const {
    td::StringBuilder sb;
    sb << td::tag("allocated_menu_windows", allocated_menu_windows) << "\n";
    sb << td::tag("file_downloads", file_manager().active_downloads()) << "\n";
    return sb.as_cslice().str();
  }
};

DebugCounters &debug_counters();

};  // namespace tdcurses
