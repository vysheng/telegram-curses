#pragma once

#include "MenuWindow.hpp"
#include "td/utils/Promise.h"
#include "td/utils/Status.h"
#include "td/utils/common.h"
#include "windows/EditorWindow.hpp"
#include <functional>
#include <memory>
#include <utility>

namespace tdcurses {

class FieldEditWindow
    : public MenuWindow
    , public windows::OneLineInputWindow {
 public:
  class Callback : public windows::OneLineInputWindow::Callback {
   public:
    virtual void run(FieldEditWindow &self, td::Result<std::string> text) = 0;
    void on_answer(windows::OneLineInputWindow *_self, std::string text) override {
      auto self = static_cast<FieldEditWindow *>(_self);
      run(*self, std::move(text));
    }
    void on_abort(windows::OneLineInputWindow *_self, std::string text) override {
      auto self = static_cast<FieldEditWindow *>(_self);
      run(*self, td::Status::Error("aborted"));
    }
  };
  template <typename T>
  class FnCallback : public Callback {
   public:
    FnCallback(T x) : x_(std::move(x)) {
    }
    void run(FieldEditWindow &self, td::Result<std::string> text) override {
      x_(self, std::move(text));
    }

   private:
    T x_;
  };

  template <typename T>
  static std::unique_ptr<FnCallback<T>> make_callback(T &&x) {
    return std::make_unique<FnCallback<T>>(std::forward<T>(x));
  }

  FieldEditWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::string prompt, std::string text,
                  std::unique_ptr<Callback> callback)
      : MenuWindow(root, root_actor), windows::OneLineInputWindow(prompt, text, false, nullptr) {
    set_callback(std::move(callback));
  }

  void handle_input(TickitKeyEventInfo *info) override {
    if (info->type == TICKIT_KEYEV_KEY) {
      if (menu_window_handle_input(info)) {
        return;
      }
    }
    return OneLineInputWindow::handle_input(info);
  }

  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> value) override {
    return std::static_pointer_cast<FieldEditWindow>(std::move(value));
  }

  td::int32 best_width() override {
    return 10000;
  }
};

}  // namespace tdcurses
