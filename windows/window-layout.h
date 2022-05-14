#pragma once

#include "td/utils/Time.h"
#include "window.h"
#include <memory>
#include <list>

namespace windows {

class WindowLayout : public Window {
 public:
  struct WindowInfo {
    WindowInfo(td::int32 x_pos, td::int32 y_pos, std::shared_ptr<Window> window)
        : x_pos(x_pos), y_pos(y_pos), window(std::move(window)) {
    }
    td::int32 x_pos;
    td::int32 y_pos;
    std::shared_ptr<Window> window;
  };
  virtual ~WindowLayout() = default;

  /*void set_refreshed() override {
    for (auto &w : windows_) {
      if (w->window) {
        w->window->set_refreshed();
      }
    }
  }*/

  bool need_refresh() override {
    for (auto &w : windows_) {
      if (w->window->need_refresh()) {
        return true;
      }
    }
    return Window::need_refresh();
  }

  td::Timestamp need_refresh_at() override {
    td::Timestamp t = Window::need_refresh_at();
    for (auto &w : windows_) {
      if (w->window->need_refresh()) {
        t.relax(w->window->need_refresh_at());
      }
    }
    return t;
  }

  void handle_input(TickitKeyEventInfo *info) override;
  virtual void activate_next_window();
  virtual void activate_prev_window();
  virtual void activate_left_window() {
  }
  virtual void activate_right_window() {
  }
  virtual void activate_upper_window() {
  }
  virtual void activate_lower_window() {
  }

  void activate_window(std::shared_ptr<Window> active_window);
  void set_subwindow_list(std::list<std::unique_ptr<WindowInfo>> windows);

  const auto &active_window() {
    return active_window_;
  }

  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override;

  virtual void render_borders(TickitRenderBuffer *rb) {
  }

 private:
  std::list<std::unique_ptr<WindowInfo>> windows_;
  std::shared_ptr<Window> active_window_;
  bool window_edit_mode_{false};

  td::int32 cursor_x_;
  td::int32 cursor_y_;
  TickitCursorShape cursor_shape_;
};

}  // namespace windows
