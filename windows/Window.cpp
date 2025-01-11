#include "Window.hpp"
#include "td/utils/logging.h"

namespace windows {

void Window::resize(td::int32 new_height, td::int32 new_width) {
  if (new_width == width_ && new_height == height_) {
    return;
  }
  set_need_refresh();
  auto old_width = width_;
  auto old_height = height_;
  width_ = new_width;
  height_ = new_height;
  on_resize(old_height, old_width, new_height, new_width);
  if (backend_window_ && new_height > 0 && new_width > 0) {
    backend_window_->resize(new_height, new_width);
  }
}

void Window::render_subwindow(WindowOutputter &rb, Window *next, bool force, bool is_active_rec,
                              bool update_cursor_pos) {
  is_active_rec &= rb.is_active();
  auto tmp_rb = rb.create_subwindow_outputter(next->backend_window(), next->y_offset_, next->x_offset_, next->height(),
                                              next->width(), is_active_rec);
  next->render_wrap(*tmp_rb, force);
  if (update_cursor_pos) {
    rb.update_cursor_position_from(*tmp_rb, next->backend_window(), next->y_offset_, next->x_offset_);
  }
}

void Window::render_wrap(WindowOutputter &rb, bool force) {
  if (!need_refresh() || (!force && !need_refresh_at().is_in_past())) {
    rb.cursor_move_yx(saved_cursor_y_, saved_cursor_x_, saved_cursor_shape_);
    return;
  }

  set_refreshed();
  render(rb, force);
  render_subwindows(rb, force);

  saved_cursor_y_ = rb.local_cursor_y();
  saved_cursor_x_ = rb.local_cursor_x();
  saved_cursor_shape_ = rb.cursor_shape();
}

}  // namespace windows
