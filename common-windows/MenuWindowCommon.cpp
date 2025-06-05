#include "MenuWindowCommon.hpp"

namespace tdcurses {

void MenuWindowElementSpawn::handle_input(MenuWindowCommon &window, const windows::InputEvent &info) {
  if (info == "T-Enter") {
    cb_(window);
    return;
  } else if (info == " ") {
    cb_(window);
    return;
  }
}

void MenuWindowElementRun::handle_input(MenuWindowCommon &window, const windows::InputEvent &info) {
  if (info == "T-Enter") {
    auto res = cb_(window, *this);
    if (res) {
      window.exit();
    }
    return;
  } else if (info == " ") {
    auto res = cb_(window, *this);
    if (res) {
      window.exit();
    }
    return;
  }
}

}  // namespace tdcurses
