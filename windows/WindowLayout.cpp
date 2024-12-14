#include "WindowLayout.hpp"
#include <memory>

namespace windows {

void WindowLayout::activate_window(std::shared_ptr<Window> active_window) {
  if (active_window_.get() == active_window.get()) {
    return;
  }
  if (active_window_) {
    active_window_->set_active(false);
  }
  active_window_ = active_window;
  if (active_window_) {
    active_window_->set_active(true);
  }
}

void WindowLayout::activate_next_window() {
  for (auto it = windows_.begin(); it != windows_.end(); it++) {
    if ((*it)->window.get() == active_window_.get()) {
      it++;
      if (it == windows_.end()) {
        it = windows_.begin();
      }
      activate_window((*it)->window);
      return;
    }
  }
  UNREACHABLE();
}

void WindowLayout::activate_prev_window() {
  for (auto it = windows_.begin(); it != windows_.end(); it++) {
    if ((*it)->window.get() == active_window_.get()) {
      if (it == windows_.begin()) {
        it = windows_.end();
      }
      it--;
      activate_window((*it)->window);
      return;
    }
  }
  UNREACHABLE();
}

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

  if (active_window_) {
    auto saved_window = active_window_;
    active_window_->handle_input(info);
  }
}

void WindowLayout::set_subwindow_list(std::list<std::unique_ptr<WindowInfo>> windows) {
  windows_ = std::move(windows);
}

void WindowLayout::render(WindowOutputter &rb, bool force) {
  render_borders(rb);
  for (auto &w : windows_) {
    bool is_active = w->window.get() == active_window_.get();
    if (force || (w->window->need_refresh() && w->window->need_refresh_at().is_in_past())) {
      w->window->set_refreshed();
      render_subwindow(rb, w->window.get(), force, is_active);
    }
  }
}

}  // namespace windows
