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

void WindowLayout::handle_input(TickitKeyEventInfo *info) {
  if (info->type == TICKIT_KEYEV_KEY && !strcmp(info->str, "C-w")) {
    window_edit_mode_ = !window_edit_mode_;
    return;
  }
  if (window_edit_mode_) {
    window_edit_mode_ = false;
    if (info->type == TICKIT_KEYEV_TEXT) {
      if (!strcmp(info->str, "w")) {
        activate_next_window();
        return;
      }
      if (!strcmp(info->str, "W")) {
        activate_prev_window();
        return;
      }
      if (!strcmp(info->str, "h")) {
        activate_left_window();
        return;
      }
      if (!strcmp(info->str, "l")) {
        activate_right_window();
        return;
      }
      if (!strcmp(info->str, "j")) {
        activate_lower_window();
        return;
      }
      if (!strcmp(info->str, "k")) {
        activate_upper_window();
        return;
      }
    } else {
      if (!strcmp(info->str, "C-w")) {
        return;
      }
    }
  }
  if (info->type == TICKIT_KEYEV_KEY && !strcmp(info->str, "C-w")) {
    window_edit_mode_ = true;
    return;
  }

  if (active_window_) {
    active_window_->handle_input(info);
  }
}

void WindowLayout::set_subwindow_list(std::list<std::unique_ptr<WindowInfo>> windows) {
  windows_ = std::move(windows);
}

void WindowLayout::render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y,
                          TickitCursorShape &cursor_shape, bool force) {
  render_borders(rb);
  for (auto &w : windows_) {
    auto rect = TickitRect{.top = w->y_pos, .left = w->x_pos, .lines = w->window->height(), .cols = w->window->width()};
    tickit_renderbuffer_save(rb);
    tickit_renderbuffer_clip(rb, &rect);
    tickit_renderbuffer_translate(rb, w->y_pos, w->x_pos);
    if (force || (w->window->need_refresh() && w->window->need_refresh_at().is_in_past())) {
      w->window->set_refreshed();
      td::int32 cx = 0, cy = 0;
      TickitCursorShape cs;
      w->window->render(rb, cx, cy, cs, force);
      if (w->window.get() == active_window_.get()) {
        cursor_x = cursor_x_ = cx + w->x_pos;
        cursor_y = cursor_y_ = cy + w->y_pos;
        cursor_shape = cursor_shape_ = cs;
      }
    } else {
      if (w->window.get() == active_window_.get()) {
        cursor_x = cursor_x_;
        cursor_y = cursor_y_;
        cursor_shape = cursor_shape_;
      }
    }
    tickit_renderbuffer_restore(rb);
    tickit_renderbuffer_mask(rb, &rect);
  }
}

}  // namespace windows
