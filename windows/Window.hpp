#pragma once

#include "td/utils/Time.h"
#include "td/utils/logging.h"
#include <atomic>
#include <memory>
#include <utility>
#include <vector>
#include "Input.hpp"
#include "Output.hpp"

namespace windows {

class BackendWindow {
 public:
  virtual ~BackendWindow() = default;
  virtual void move_to_bottom() {
  }
  virtual void move_below(BackendWindow *other) {
  }
  virtual void move_above(BackendWindow *other) {
  }
  virtual void move_to_top() {
  }
  virtual void move_yx(td::int32 y, td::int32 x) {
  }
  virtual void resize(td::int32 height, td::int32 width) {
  }
};

class Window {
 public:
  virtual ~Window() = default;
  Window() {
  }

  auto height() const {
    return height_;
  }
  auto width() const {
    return width_;
  }
  auto y_offset() const {
    return y_offset_;
  }
  auto x_offset() const {
    return x_offset_;
  }
  void resize(td::int32 new_height, td::int32 new_width);
  virtual void on_resize(td::int32 old_height, td::int32 old_width, td::int32 new_height, td::int32 new_width) {
    set_need_refresh();
  }
  virtual void handle_input(const InputEvent &info) {
  }
  virtual void render(WindowOutputter &rb, bool force) = 0;
  void render_wrap(WindowOutputter &rb, bool force);

  void set_refreshed() {
    need_refresh_ = false;
    refresh_at_ = td::Timestamp::never();
    refreshed_at_ = td::Timestamp::now();
  }
  virtual void set_need_refresh() {
    if (need_refresh_) {
      return;
    }
    need_refresh_ = true;
    if (refreshed_at_) {
      if (refreshed_at_.in() >= -0.1) {
        refresh_at_ = td::Timestamp::at(refresh_at_.at() + 0.1);
      } else {
        refresh_at_ = td::Timestamp::now();
      }
    } else {
      refresh_at_ = td::Timestamp::now();
    }
  }
  virtual void set_need_refresh_force() {
    need_refresh_ = true;
    refresh_at_ = td::Timestamp::now();
  }
  void set_need_refresh_rec() {
    set_need_refresh();
    for (auto &w : subwindows_) {
      w->set_need_refresh_force_rec();
    }
  }
  void set_need_refresh_force_rec() {
    set_need_refresh_force();
    for (auto &w : subwindows_) {
      w->set_need_refresh_force_rec();
    }
  }
  void set_need_refresh_active() {
    set_need_refresh();
    if (active_subwindow_) {
      active_subwindow_->set_need_refresh_active();
    }
  }
  void set_need_refresh_active_force() {
    set_need_refresh_force();
    if (active_subwindow_) {
      active_subwindow_->set_need_refresh_active_force();
    }
  }
  virtual bool need_refresh() {
    if (need_refresh_) {
      return true;
    }
    for (auto &w : subwindows_) {
      if (w->need_refresh()) {
        return true;
      }
    }
    return false;
  }
  virtual td::Timestamp need_refresh_at() {
    td::Timestamp t = refresh_at_;
    for (auto &w : subwindows_) {
      t.relax(w->need_refresh_at());
    }
    return t;
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

  void set_parent_window(Window *window, td::int32 y_offset, td::int32 x_offset) {
    if (parent_ == window && y_offset_ == y_offset && x_offset_ == x_offset) {
      return;
    }
    parent_ = window;
    y_offset_ = y_offset;
    x_offset_ = x_offset;
    set_need_refresh_force_rec();
    if (backend_window_) {
      backend_window_->move_yx(y_offset_, x_offset_);
    }
  }
  void move_yx(td::int32 y_offset, td::int32 x_offset) {
    if (y_offset_ == y_offset && x_offset_ == x_offset) {
      return;
    }
    y_offset_ = y_offset;
    x_offset_ = x_offset;
    set_need_refresh_force_rec();
    if (backend_window_) {
      backend_window_->move_yx(y_offset_, x_offset_);
    }
  }
  void add_subwindow(std::shared_ptr<Window> window, td::int32 y_offset, td::int32 x_offset) {
    window->set_parent_window(this, y_offset, x_offset);
    if (!active_subwindow_) {
      active_subwindow_ = window;
      active_subwindow_->set_need_refresh_active();
    }
    subwindows_.push_back(std::move(window));
    set_need_refresh();
  }
  void del_subwindow(Window *window) {
    bool found = false;
    auto it = subwindows_.begin();
    while (it != subwindows_.end()) {
      if (it->get() == window) {
        std::swap(*it, subwindows_.back());
        found = true;
        break;
      }
      it++;
    }
    if (!found) {
      return;
    }
    subwindows_.pop_back();
    if (window == active_subwindow_.get()) {
      if (subwindows_.size() > 0) {
        active_subwindow_ = subwindows_[0];
        active_subwindow_->set_need_refresh_active();
      } else {
        active_subwindow_ = nullptr;
      }
    }
    set_need_refresh_force();
  }
  const auto &active_subwindow() const {
    return active_subwindow_;
  }
  void activate_subwindow(std::shared_ptr<Window> window) {
    if (window.get() == active_subwindow_.get()) {
      return;
    }
    CHECK(window);
    CHECK(active_subwindow_);
    active_subwindow_->set_need_refresh_active_force();
    window->set_need_refresh_active_force();
    active_subwindow_ = std::move(window);
  }
  virtual bool force_active() const {
    return false;
  }

  void render_subwindow(WindowOutputter &rb, Window *next, bool force, bool is_active, bool update_cursor_pos);
  void render_subwindows(WindowOutputter &rb, bool force) {
    for (auto &w : subwindows_) {
      render_subwindow(rb, w.get(), force, w.get() == active_subwindow_.get(), w.get() == active_subwindow_.get());
    }
  }

  void set_backend_window(std::unique_ptr<BackendWindow> window) {
    backend_window_ = std::move(window);
    if (backend_window_) {
      if (height() > 0 && width() > 0) {
        backend_window_->resize(height(), width());
      }
      backend_window_->move_yx(y_offset_, x_offset_);
    }
  }
  BackendWindow *backend_window() const {
    return backend_window_.get();
  }
  std::unique_ptr<BackendWindow> release_backend_window() {
    return std::move(backend_window_);
  }

  void set_name(std::string name) {
    name_ = name;
  }

  const auto &name() const {
    return name_;
  }

 private:
  td::int32 width_{10};
  td::int32 height_{10};
  td::int32 y_offset_{0};
  td::int32 x_offset_{0};
  std::atomic<bool> need_refresh_{true};

  td::int32 saved_cursor_y_{0};
  td::int32 saved_cursor_x_{0};
  WindowOutputter::CursorShape saved_cursor_shape_{WindowOutputter::CursorShape::None};

  td::Timestamp refreshed_at_;
  td::Timestamp refresh_at_;
  Window *parent_{nullptr};
  std::unique_ptr<BackendWindow> backend_window_;
  std::string name_;

  std::vector<std::shared_ptr<Window>> subwindows_;
  std::shared_ptr<Window> active_subwindow_;
};

}  // namespace windows
