#pragma once

#include "td/utils/Time.h"
#include "td/utils/logging.h"
#include <atomic>
#include <memory>
#include <utility>
#include "Input.hpp"
#include "Output.hpp"

namespace windows {

class Window {
 public:
  enum class Type { Normal, Popup, Fullscreen, ErrorPopup };

  virtual ~Window() = default;
  Window() {
  }
  void resize(td::int32 new_width, td::int32 new_height);
  virtual void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) {
    set_need_refresh();
  }
  virtual void handle_input(const InputEvent &info) {
  }
  virtual void render(WindowOutputter &rb, bool force) = 0;
  void render_wrap(WindowOutputter &rb, bool force);

  auto width() const {
    return width_;
  }
  auto height() const {
    return height_;
  }

  virtual void set_refreshed() {
    need_refresh_ = false;
    refresh_at_ = td::Timestamp::now();
  }
  void set_need_refresh() {
    if (need_refresh_) {
      return;
    }
    need_refresh_ = true;
    if (refresh_at_) {
      if (refresh_at_.in() >= -0.1) {
        refresh_at_ = td::Timestamp::at(refresh_at_.at() + 0.1);
      } else {
        refresh_at_ = td::Timestamp::now();
      }
    } else {
      refresh_at_ = td::Timestamp::now();
    }
  }
  virtual bool need_refresh() {
    return need_refresh_;
  }
  virtual td::Timestamp need_refresh_at() {
    return refresh_at_;
  }

  bool is_active() const {
    return is_active_;
  }
  virtual void set_active(bool value) {
    if (is_active_ != value) {
      is_active_ = value;
      set_need_refresh();
    }
  }

  virtual td::int32 min_width() {
    return 5;
  }
  virtual td::int32 min_height() {
    return 3;
  }
  virtual td::int32 best_width() {
    return 5;
  }
  virtual td::int32 best_height() {
    return 3;
  }
  virtual bool force_active() {
    return false;
  }

  void set_parent(Window *window) {
    parent_ = window;
  }

  void set_parent_offset(td::int32 y_offset, td::int32 x_offset) {
    y_offset_ = y_offset;
    x_offset_ = x_offset;
    set_need_refresh();
  }

  void render_subwindow(WindowOutputter &rb, Window *next, bool force, bool is_active, bool update_cursor_pos);

 private:
  td::int32 width_{10};
  td::int32 height_{10};
  td::int32 y_offset_{0};
  td::int32 x_offset_{0};
  bool is_active_{false};
  std::atomic<bool> need_refresh_{true};

  td::int32 saved_cursor_y_{0};
  td::int32 saved_cursor_x_{0};
  WindowOutputter::CursorShape saved_cursor_shape_{WindowOutputter::CursorShape::None};

  td::Timestamp refresh_at_;
  Window *parent_{nullptr};
};

}  // namespace windows
