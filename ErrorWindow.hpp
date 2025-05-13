#pragma once

#include "MenuWindow.hpp"
#include "windows/ViewWindow.hpp"
#include "windows/Markup.hpp"
#include "windows/Window.hpp"
#include <memory>
#include <type_traits>
#include <vector>

namespace tdcurses {

class ErrorWindow
    : public MenuWindow
    , public windows::ViewWindow {
 public:
  class Callback : public windows::ViewWindow::Callback {
   public:
    virtual void on_answer(ErrorWindow &window) = 0;
    virtual void on_abort(ErrorWindow &window) = 0;

   private:
    void on_answer(ViewWindow *window) override {
      on_answer(static_cast<ErrorWindow &>(*window));
    }
    void on_abort(ViewWindow *window) override {
      on_abort(static_cast<ErrorWindow &>(*window));
    }
  };
  ErrorWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::string text,
              std::vector<windows::MarkupElement> markup, std::unique_ptr<Callback> callback)
      : MenuWindow(root, root_actor), windows::ViewWindow(std::move(text), std::move(markup), std::move(callback)) {
  }
  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> value) override {
    return std::static_pointer_cast<ErrorWindow>(std::move(value));
  }
  void on_install() override {
    set_border_color(windows::BorderType::Double, (td::int32)Color::Red);
    set_need_refresh();
  }
};

template <typename T>
std::enable_if_t<std::is_base_of<MenuWindow, T>::value, std::shared_ptr<ErrorWindow>> spawn_error_window(
    T &cur_window, std::string text, std::vector<windows::MarkupElement> markup) {
  class Cb : public ErrorWindow::Callback {
   public:
    Cb(T *win) : self_(win), self_id_(win->window_unique_id()) {
    }
    void on_abort(ErrorWindow &w) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
      }
    }
    void on_answer(ErrorWindow &w) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
      }
    }

   private:
    T *self_;
    td::int64 self_id_;
  };
  auto callback = std::make_unique<Cb>(&cur_window);
  return cur_window.template spawn_submenu<ErrorWindow>(text, markup, std::move(callback));
}

template <typename T>
std::enable_if_t<std::is_base_of<MenuWindow, T>::value, std::shared_ptr<ErrorWindow>> spawn_replace_error_window(
    T &cur_window, std::string text, std::vector<windows::MarkupElement> markup = {}) {
  class Cb : public ErrorWindow::Callback {
   public:
    Cb(T *win) : self_(win), self_id_(win->window_unique_id()) {
    }
    void on_abort(ErrorWindow &w) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
      }
    }
    void on_answer(ErrorWindow &w) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
      }
    }

   private:
    T *self_;
    td::int64 self_id_;
  };
  auto callback = std::make_unique<Cb>(&cur_window);
  return cur_window.template spawn_submenu<ErrorWindow>(text, markup, std::move(callback));
}

template <typename T>
std::enable_if_t<!std::is_base_of<MenuWindow, T>::value && std::is_base_of<TdcursesWindowBase, T>::value,
                 std::shared_ptr<ErrorWindow>>
spawn_error_window(T &cur_window, std::string text, std::vector<windows::MarkupElement> markup) {
  class Cb : public ErrorWindow::Callback {
   public:
    Cb(T *win) : self_(win), self_id_(win->window_unique_id()) {
    }
    void on_abort(ErrorWindow &w) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
      }
    }
    void on_answer(ErrorWindow &w) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
      }
    }

   private:
    T *self_;
    td::int64 self_id_;
  };
  auto callback = std::make_unique<Cb>(&cur_window);
  return create_menu_window<ErrorWindow>(cur_window.root(), cur_window.root_actor_id(), text, markup,
                                         std::move(callback));
}

}  // namespace tdcurses
