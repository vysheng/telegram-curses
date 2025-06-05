#include "Debug.hpp"
#include "td/utils/common.h"
#include "td/utils/StringBuilder.h"
#include "td/utils/format.h"
#include "managers/FileManager.hpp"
#include "td/telegram/Version.h"
#include "managers/GlobalParameters.hpp"
#include "td/utils/SliceBuilder.h"

#include <notcurses/notcurses.h>
#include <libconfig.h++>
#if USE_LIBTICKIT
#include "tickit.h"
#endif

namespace tdcurses {

std::string DebugCounters::to_str() const {
  td::StringBuilder sb;
  sb << td::tag("app_version", global_parameters().version()) << "\n";
  sb << td::tag("tdlib_version", global_parameters().tdlib_version()) << "\n";
  sb << td::tag("mtproto_layer", td::MTPROTO_LAYER) << "\n";
  sb << td::tag("notcurses_version", notcurses_version()) << "\n";
#if USE_LIBTICKIT
  sb << td::tag("libtickit_version",
                PSTRING() << TICKIT_VERSION_MAJOR << "." << TICKIT_VERSION_MINOR << "." << TICKIT_VERSION_PATCH)
     << "\n";
#endif
  sb << td::tag("libconfig_version",
                PSTRING() << LIBCONFIGXX_VER_MAJOR << "." << LIBCONFIGXX_VER_MINOR << "." << LIBCONFIGXX_VER_REVISION)
     << "\n";
  sb << td::tag("backend", global_parameters().backend_type()) << "\n";
  sb << td::tag("allocated_menu_windows", allocated_menu_windows) << "\n";
  sb << td::tag("file_downloads", file_manager().active_downloads()) << "\n";
  return sb.as_cslice().str();
}

DebugCounters &debug_counters() {
  static DebugCounters instance{};
  return instance;
}

}  // namespace tdcurses
