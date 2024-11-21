#pragma once

#include "Debug.hpp"
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

class MenuWindow : public TdcursesWindowBase {
 public:
  MenuWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : TdcursesWindowBase(root, std::move(root_actor)) {
    debug_counters().allocated_menu_windows++;
  }
  virtual ~MenuWindow() {
    debug_counters().allocated_menu_windows--;
  }

  virtual std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow>) = 0;

  static std::shared_ptr<windows::Window> create_boxed_window(std::shared_ptr<MenuWindow> window) {
    auto ptr = window->get_window(window);
    auto boxed = std::make_shared<windows::BorderedWindow>(std::move(ptr), windows::BorderedWindow::BorderType::Double);
    window->bordered_ = boxed;
    return boxed;
  }

  std::shared_ptr<MenuWindow> rollback() {
    CHECK(history_.size() > 0);
    history_.pop_back();  // self callback
    if (history_.size() == 0) {
      exit();
      return nullptr;
    }

    auto prev = history_.back();
    prev->update_history(std::move(history_));
    prev->root()->add_popup_window(prev->bordered_, 3);

    exit();
    return prev;
  }

  void exit() {
    std::shared_ptr<MenuWindow> self;
    if (history_.size() > 0) {
      self = history_.back();
    }
    root()->del_popup_window(bordered_.get());
    for (auto &w : history_) {
      w->bordered_ = nullptr;
    }
    history_.clear();
    bordered_ = nullptr;
  }

  void update_history(std::vector<std::shared_ptr<MenuWindow>> history) {
    history_ = std::move(history);
  }

  /*std::shared_ptr<MenuWindow> spawn_submenu(MenuWindowSpawnFunction cb) {
    return spawn_submenu(cb(root(), root_actor_id()));
  }*/

  std::shared_ptr<MenuWindow> spawn_submenu(std::shared_ptr<MenuWindow> res) {
    history_.push_back(res);
    res->update_history(std::move(history_));
    auto boxed = create_boxed_window(res);
    res->root()->add_popup_window(boxed, 3);
    res->root()->del_popup_window(bordered_.get());
    return res;
  }

  bool menu_window_handle_input(TickitKeyEventInfo *info) {
    if (info->type == TICKIT_KEYEV_KEY) {
      if (!strcmp(info->str, "Escape")) {
        rollback();
        return true;
      } else if (!strcmp(info->str, "C-q")) {
        rollback();
        return true;
      } else if (!strcmp(info->str, "C-Q")) {
        exit();
        return true;
      }
    } else {
      if (!strcmp(info->str, "q")) {
        rollback();
        return true;
      } else if (!strcmp(info->str, "Q")) {
        exit();
        return true;
      }
    }
    return false;
  }

  template <typename T, typename... ArgsT>
  std::shared_ptr<T> spawn_submenu(ArgsT &&...args) {
    auto res = std::make_shared<T>(root(), root_actor_id(), std::forward<ArgsT>(args)...);
    spawn_submenu(res);
    return res;
  }

 private:
  std::vector<std::shared_ptr<MenuWindow>> history_;
  std::shared_ptr<windows::Window> bordered_;
};

template <typename T, typename... ArgsT>
std::shared_ptr<T> create_menu_window(Tdcurses *root, td::ActorId<Tdcurses> root_actor, ArgsT &&...args) {
  auto res = std::make_shared<T>(root, root_actor, std::forward<ArgsT>(args)...);
  res->update_history(std::vector<std::shared_ptr<MenuWindow>>{res});
  auto boxed = MenuWindow::create_boxed_window(res);
  res->root()->add_popup_window(boxed, 3);
  return res;
}

};  // namespace tdcurses
