#include "Screen.hpp"
#include "Window.hpp"
#include "WindowLayout.hpp"
#include "unicode.h"

#include <memory>
#include <tickit.h>

#include "td/utils/Time.h"
#include "td/utils/Timer.h"

namespace windows {

Screen::Screen(std::unique_ptr<Callback> callback) : callback_(std::move(callback)) {
}

void Screen::init() {
  set_tickit_wrap();

  tickit_root_ = tickit_new_stdtty();
  CHECK(tickit_root_);
  tickit_term_ = tickit_get_term(tickit_root_);
  CHECK(tickit_root_);

  int lines, cols;
  tickit_term_get_size(tickit_term_, &lines, &cols);

  auto handle_resize = [](TickitTerm *tt, TickitEventFlags flags, void *_info, void *data) {
    TickitResizeEventInfo *info = (TickitResizeEventInfo *)_info;
    static_cast<Screen *>(data)->resize(info->cols, info->lines);
    return 1;
  };
  tickit_term_bind_event(tickit_term_, TICKIT_TERM_ON_RESIZE, (TickitBindFlags)0, handle_resize, this);

  auto handle_input = [](TickitTerm *tt, TickitEventFlags flags, void *_info, void *data) {
    TickitKeyEventInfo *info = (TickitKeyEventInfo *)_info;
    static_cast<Screen *>(data)->handle_input(info);
    return 1;
  };
  tickit_term_bind_event(tickit_term_, TICKIT_TERM_ON_KEY, (TickitBindFlags)0, handle_input, this);

  tickit_root_window_ = tickit_window_new_root(tickit_term_);
  tickit_window_take_focus(tickit_root_window_);

  tickit_term_observe_sigwinch(tickit_term_, true);

  resize(cols, lines);
}

void Screen::stop() {
  if (tickit_root_ && !finished_) {
    tickit_window_destroy(tickit_root_window_);
    tickit_unref(tickit_root_);
    tickit_root_ = nullptr;
    tickit_term_ = nullptr;
    tickit_root_window_ = nullptr;
    callback_->on_close();
    finished_ = true;
  }
}

void Screen::resize(int width, int height) {
  if (!tickit_term_ || finished_) {
    return;
  }

  tickit_term_refresh_size(tickit_term_);
  refresh();
}

td::int32 Screen::width() {
  if (!tickit_term_ || finished_) {
    return 0;
  }
  int lines, cols;
  tickit_term_get_size(tickit_term_, &lines, &cols);
  return cols;
}

td::int32 Screen::height() {
  if (!tickit_term_ || finished_) {
    return 0;
  }
  int lines, cols;
  tickit_term_get_size(tickit_term_, &lines, &cols);
  return lines;
}

bool Screen::has_popup_window(Window *window) const {
  if (!tickit_term_ || finished_) {
    return false;
  }
  for (auto it = popup_windows_.begin(); it != popup_windows_.end(); it++) {
    if (it->second.get() == window) {
      return true;
    }
  }
  return false;
}

void Screen::add_popup_window(std::shared_ptr<Window> window, td::int32 priority) {
  if (!tickit_term_ || finished_) {
    return;
  }
  auto it = popup_windows_.begin();
  while (it != popup_windows_.end() && it->first <= priority) {
    it++;
  }
  it = popup_windows_.emplace(it, priority, window);
  if (it == popup_windows_.begin()) {
    if (active_window_) {
      active_window_->set_active(false);
    }
    active_window_ = window;
    active_window_->set_active(true);
  }
  refresh(true);
}

void Screen::del_popup_window(Window *window) {
  if (!tickit_term_ || finished_) {
    return;
  }
  for (auto it = popup_windows_.begin(); it != popup_windows_.end(); it++) {
    if (it->second.get() == window) {
      popup_windows_.erase(it);
      break;
    }
  }
  if (active_window_.get() == window) {
    active_window_->set_active(false);
    active_window_ = nullptr;
    if (popup_windows_.size() > 0) {
      active_window_ = popup_windows_.begin()->second;
      active_window_->set_active(true);
    }
  }
  refresh(true);
}

void Screen::change_layout(std::shared_ptr<WindowLayout> window_layout) {
  if (!tickit_term_ || finished_) {
    return;
  }
  layout_ = std::move(window_layout);
  refresh(true);
}

void Screen::loop() {
  if (!tickit_root_ || finished_) {
    return;
  }

  tickit_tick(tickit_root_, TICKIT_RUN_NOHANG);
  refresh(true);
}

void Screen::handle_input(TickitKeyEventInfo *info) {
  if (!tickit_root_ || finished_) {
    return;
  }

  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "C-r")) {
      refresh(true);
      return;
    }
  }

  if (active_window_) {
    active_window_->handle_input(info);
  } else if (layout_) {
    layout_->handle_input(info);
  }

  refresh();
}

void Screen::refresh(bool force) {
  if (!tickit_term_ || finished_) {
    return;
  }
  if (force) {
    tickit_term_refresh_size(tickit_term_);
  }
  TickitRenderBuffer *rb = tickit_renderbuffer_new(height(), width());
  CHECK(rb);

  auto screen_width = width();
  auto screen_height = height();

  bool is_first = true;

  for (auto &p : popup_windows_) {
    auto &window = p.second;
    if (window->min_width() > screen_width || window->min_height() > screen_height) {
      UNREACHABLE();
    }

    auto max_w = std::max(screen_width / 2, window->min_width());
    auto max_h = std::max(screen_height / 2, window->min_height());
    auto w = std::min(max_w, window->best_width());
    auto h = std::min(max_h, window->best_height());

    window->resize(w, h);
    auto w_off = (screen_width - w) / 2;
    auto h_off = (screen_height - h) / 2;

    auto rect = TickitRect{.top = h_off, .left = w_off, .lines = h, .cols = w};
    if (force || (window->need_refresh() && window->need_refresh_at().is_in_past())) {
      tickit_renderbuffer_save(rb);
      tickit_renderbuffer_clip(rb, &rect);
      tickit_renderbuffer_translate(rb, h_off, w_off);
      td::int32 cursor_x, cursor_y;
      TickitCursorShape cursor_shape;
      window->render(rb, cursor_x, cursor_y, cursor_shape, force);
      if (is_first) {
        cursor_x_ = cursor_x + h_off;
        cursor_y_ = cursor_y + w_off;
        cursor_shape_ = cursor_shape;
      }
      tickit_renderbuffer_restore(rb);
    }
    tickit_renderbuffer_mask(rb, &rect);
    window->set_refreshed();
    is_first = false;
  }

  if (layout_) {
    layout_->resize(screen_width, screen_height);
    if (force || (layout_->need_refresh() && layout_->need_refresh_at().is_in_past())) {
      tickit_renderbuffer_save(rb);
      td::int32 cursor_x, cursor_y;
      TickitCursorShape cursor_shape;
      layout_->render(rb, cursor_x, cursor_y, cursor_shape, force);
      if (is_first) {
        cursor_x_ = cursor_x;
        cursor_y_ = cursor_y;
        cursor_shape_ = cursor_shape;
      }
      tickit_renderbuffer_restore(rb);
      layout_->set_refreshed();
    }
  } else if (force) {
    auto rect = TickitRect{.top = 0, .left = 0, .lines = height(), .cols = width()};
    tickit_renderbuffer_eraserect(rb, &rect);
  }

  tickit_renderbuffer_flush_to_term(rb, tickit_term_);
  tickit_renderbuffer_destroy(rb);

  if (cursor_shape_ != (TickitCursorShape)0) {
    tickit_window_set_cursor_shape(tickit_root_window_, cursor_shape_);
    tickit_window_set_cursor_visible(tickit_root_window_, true);
  } else {
    tickit_window_set_cursor_visible(tickit_root_window_, false);
  }
  tickit_window_set_cursor_position(tickit_root_window_, cursor_y_, cursor_x_);
  tickit_window_flush(tickit_root_window_);

  tickit_term_flush(tickit_term_);
}

td::Timestamp Screen::need_refresh_at() {
  td::Timestamp r;
  for (auto &x : popup_windows_) {
    if (x.second->need_refresh()) {
      r.relax(x.second->need_refresh_at());
    }
  }
  if (layout_ && layout_->need_refresh()) {
    r.relax(layout_->need_refresh_at());
  }
  return r;
}

Screen::~Screen() {
  stop();
}

}  // namespace windows
