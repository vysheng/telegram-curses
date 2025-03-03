#include "WindowLayout.hpp"
#include <memory>

namespace windows {

void WindowLayout::handle_input(const InputEvent &info) {
  if (info == "T-SWITCH_WINDOW_NEXT") {
    activate_next_window();
    return;
  } else if (info == "T-SWITCH_WINDOW_PREV") {
    activate_prev_window();
    return;
  } else if (info == "T-SWITCH_WINDOW_LEFT") {
    activate_left_window();
    return;
  } else if (info == "T-SWITCH_WINDOW_RIGHT") {
    activate_right_window();
    return;
  } else if (info == "T-SWITCH_WINDOW_DOWN") {
    activate_lower_window();
    return;
  } else if (info == "T-SWITCH_WINDOW_UP") {
    activate_upper_window();
    return;
  }

  auto w = active_subwindow();
  if (w) {
    w->handle_input(info);
  }
}

}  // namespace windows
