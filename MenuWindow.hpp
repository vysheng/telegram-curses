#pragma once

#include "Outputter.hpp"
#include "windows/Markup.hpp"
#include "windows/PadWindow.hpp"
#include "TdcursesWindowBase.hpp"
#include "windows/Window.hpp"
#include <functional>
#include <limits>
#include <memory>
#include <vector>

namespace tdcurses {

class MenuWindow;
using MenuWindowSpawnFunction =
    std::function<std::shared_ptr<MenuWindow>(Tdcurses *root, td::ActorId<Tdcurses> root_actor)>;

class MenuWindow
    : public windows::PadWindow
    , public TdcursesWindowBase {
 public:
  MenuWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor)
      : windows::PadWindow(), TdcursesWindowBase(root, std::move(root_actor)) {
  }

  static void create_popup(std::shared_ptr<MenuWindow> window) {
    auto boxed = std::make_shared<windows::BorderedWindow>(window, windows::BorderedWindow::BorderType::Double);
    window->bordered_ = boxed;
    window->root()->add_popup_window(boxed, 1);
  }

  void rollback() {
    CHECK(history_.size() > 0);
    history_.pop_back();  // self callback
    if (history_.size() == 0) {
      return exit();
    }

    auto cb = history_.back();

    auto res = cb(root(), root_actor_id());
    res->update_history(std::move(history_));

    MenuWindow::create_popup(res);
    exit();
  }

  void exit() {
    root()->del_popup_window(bordered_.get());
    bordered_ = nullptr;
  }

  void update_history(std::vector<MenuWindowSpawnFunction> history) {
    history_ = std::move(history);
  }

  void spawn_submenu(MenuWindowSpawnFunction cb) {
    history_.push_back(cb);
    auto res = cb(root(), root_actor_id());
    res->update_history(std::move(history_));

    MenuWindow::create_popup(res);
    exit();
  }

  void handle_input(TickitKeyEventInfo *info) override {
    if (info->type == TICKIT_KEYEV_KEY) {
      if (!strcmp(info->str, "Escape")) {
        rollback();
        return;
      }
    } else {
      if (!strcmp(info->str, "q")) {
        rollback();
        return;
      }
      if (!strcmp(info->str, "Q")) {
        exit();
        return;
      }
    }
    return PadWindow::handle_input(info);
  }

 private:
  std::vector<MenuWindowSpawnFunction> history_;
  std::shared_ptr<windows::Window> bordered_;
};

std::shared_ptr<MenuWindow> create_menu_window(Tdcurses *root, td::ActorId<Tdcurses> root_actor,
                                               MenuWindowSpawnFunction cb);

};  // namespace tdcurses
