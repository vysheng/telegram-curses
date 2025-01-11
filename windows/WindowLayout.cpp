#include "WindowLayout.hpp"
#include <memory>

namespace windows {

void WindowLayout::handle_input(const InputEvent &info) {
  if (info == "C-w") {
    window_edit_mode_ = !window_edit_mode_;
    return;
  }
  if (window_edit_mode_) {
    window_edit_mode_ = false;
    if (info == "w") {
      activate_next_window();
      return;
    } else if (info == "W") {
      activate_prev_window();
      return;
    } else if (info == "h") {
      activate_left_window();
      return;
    } else if (info == "l") {
      activate_right_window();
      return;
    } else if (info == "j") {
      activate_lower_window();
      return;
    } else if (info == "k") {
      activate_upper_window();
      return;
    }
  }

  auto w = active_subwindow();
  if (w) {
    w->handle_input(info);
  }
}

}  // namespace windows
