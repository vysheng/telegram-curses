#pragma once

#include "MenuWindow.hpp"
#include "windows/PadWindow.hpp"
#include <memory>

namespace tdcurses {

class MenuWindowPad
    : public MenuWindow
    , public windows::PadWindow {
 public:
  MenuWindowPad(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindow(root, root_actor) {
  }
  void handle_input(TickitKeyEventInfo *info) override {
    if (menu_window_handle_input(info)) {
      return;
    }
    return PadWindow::handle_input(info);
  }

  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> value) override {
    return std::static_pointer_cast<MenuWindowPad>(std::move(value));
  }
};

}  // namespace tdcurses
