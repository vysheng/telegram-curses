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

class TextEditWindow
    : public MenuWindow
    , public windows::EditorWindow {
 public:
  class Callback : public windows::EditorWindow::Callback {
   public:
    virtual void run(TextEditWindow &self, td::Result<std::string> text) = 0;
    void on_answer(windows::EditorWindow *_self, std::string text) override {
      auto self = static_cast<TextEditWindow *>(_self);
      run(*self, std::move(text));
    }
    void on_abort(windows::EditorWindow *_self, std::string text) override {
      auto self = static_cast<TextEditWindow *>(_self);
      run(*self, td::Status::Error("aborted"));
    }
  };
  template <typename T>
  class FnCallback : public Callback {
   public:
    FnCallback(T x) : x_(std::move(x)) {
    }
    void run(TextEditWindow &self, td::Result<std::string> text) override {
      x_(self, std::move(text));
    }

   private:
    T x_;
  };

  template <typename T>
  static std::unique_ptr<FnCallback<T>> make_callback(T &&x) {
    return std::make_unique<FnCallback<T>>(std::forward<T>(x));
  }

  TextEditWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::string title, std::string text,
                 std::unique_ptr<Callback> callback)
      : MenuWindow(root, root_actor), windows::EditorWindow(text, nullptr) {
    install_callback(std::move(callback));
  }

  void handle_input(const windows::InputEvent &info) override {
    if (!info.is_text_key()) {
      if (menu_window_handle_input(info)) {
        return;
      }
    }
    return EditorWindow::handle_input(info);
  }

  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> value) override {
    return std::static_pointer_cast<TextEditWindow>(std::move(value));
  }

  td::int32 best_width() override {
    return 10000;
  }
};

template <typename T, typename F>
std::enable_if_t<std::is_base_of<MenuWindow, T>::value, std::shared_ptr<TextEditWindow>> spawn_text_edit_window(
    T &cur_window, std::string caption, std::string initial_value, F &&cb) {
  class Cb : public TextEditWindow::Callback {
   public:
    Cb(F &&cb, T *win) : cb_(std::move(cb)), self_(win), self_id_(win->window_unique_id()) {
    }
    void run(TextEditWindow &w, td::Result<std::string> answer) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
        cb_(std::move(answer));
      }
    }

   private:
    F cb_;
    T *self_;
    td::int64 self_id_;
  };
  auto callback = std::make_unique<Cb>(std::move(cb), &cur_window);
  return cur_window.template spawn_submenu<TextEditWindow>(std::move(caption), std::move(initial_value),
                                                           std::move(callback));
}

template <typename T, typename F>
std::enable_if_t<std::is_base_of<TdcursesWindowBase, T>::value && !std::is_base_of<MenuWindow, T>::value,
                 std::shared_ptr<TextEditWindow>>
spawn_text_edit_window(T &cur_window, std::string caption, std::string initial_value, F &&cb) {
  class Cb : public TextEditWindow::Callback {
   public:
    Cb(F &&cb, T *win) : cb_(std::move(cb)), self_(win), self_id_(win->window_unique_id()) {
    }
    void run(TextEditWindow &w, td::Result<std::string> answer) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
        cb_(std::move(answer));
      }
    }

   private:
    F cb_;
    T *self_;
    td::int64 self_id_;
  };
  auto callback = std::make_unique<Cb>(std::move(cb), &cur_window);
  return create_menu_window<TextEditWindow>(cur_window.root(), cur_window.root_actor_id(), std::move(caption),
                                            std::move(initial_value), std::move(callback));
}

}  // namespace tdcurses
