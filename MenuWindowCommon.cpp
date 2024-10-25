#include "MenuWindowCommon.hpp"

namespace tdcurses {

void MenuWindowElementSpawn::handle_input(MenuWindowCommon &window, TickitKeyEventInfo *info) {
  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "Enter")) {
      cb_(window);
      return;
    }
  } else {
    if (!strcmp(info->str, " ")) {
      cb_(window);
      return;
    }
  }
}

void MenuWindowElementRun::handle_input(MenuWindowCommon &window, TickitKeyEventInfo *info) {
  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "Enter")) {
      auto res = cb_(*this);
      if (res) {
        window.exit();
      }
      return;
    }
  } else {
    if (!strcmp(info->str, " ")) {
      auto res = cb_(*this);
      if (res) {
        window.exit();
      }
      return;
    }
  }
}

}  // namespace tdcurses
