#pragma once

#include "MenuWindow.hpp"
#include "windows/Markup.hpp"
#include "windows/ViewWindow.hpp"
#include <vector>

namespace tdcurses {

class MenuWindowView
    : public MenuWindow
    , public windows::ViewWindow {
 public:
  MenuWindowView(Tdcurses *root, td::ActorId<Tdcurses> root_actor)
      : MenuWindow(root, root_actor), windows::ViewWindow("", nullptr) {
  }
  MenuWindowView(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::string text,
                 std::vector<windows::MarkupElement> markup)
      : MenuWindow(root, root_actor), windows::ViewWindow(std::move(text), std::move(markup), nullptr) {
  }
  void handle_input(const windows::InputEvent &info) override {
    if (menu_window_handle_input(info)) {
      return;
    }
    return ViewWindow::handle_input(info);
  }

  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> value) override {
    return std::static_pointer_cast<MenuWindowView>(std::move(value));
  }
};

}  // namespace tdcurses
