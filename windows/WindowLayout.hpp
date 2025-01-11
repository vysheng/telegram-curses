#pragma once

#include "td/utils/Time.h"
#include "Window.hpp"
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

  void handle_input(const InputEvent &info) override;
  virtual void activate_next_window() {
  }
  virtual void activate_prev_window() {
  }
  virtual void activate_left_window() {
  }
  virtual void activate_right_window() {
  }
  virtual void activate_upper_window() {
  }
  virtual void activate_lower_window() {
  }

  void render(WindowOutputter &rb, bool force) override {
    render_borders(rb);
  }

  virtual void render_borders(WindowOutputter &rb) {
  }

 private:
  bool window_edit_mode_{false};
};

}  // namespace windows
