#include "MenuWindowCommon.hpp"

namespace tdcurses {

void MenuWindowElementSpawn::handle_input(MenuWindowCommon &window, TickitKeyEventInfo *info) {
  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "Enter")) {
      window.spawn_submenu(cb_);
      return;
    }
  }
}

void MenuWindowElementRun::handle_input(MenuWindowCommon &window, TickitKeyEventInfo *info) {
  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "Enter")) {
      cb_();
      window.exit();
      return;
    }
  }
}

}  // namespace tdcurses
