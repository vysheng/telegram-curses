#pragma once

#include "td/utils/Time.h"
#include "td/utils/logging.h"
#include <atomic>
#include <memory>
#include <utility>
#include <tickit.h>

namespace windows {

class Window {
 public:
  enum class BorderType { None, Simple, Double };
  enum class Type { Normal, Popup, Fullscreen, ErrorPopup };

  virtual ~Window() = default;
  Window() {
  }
  void resize(td::int32 new_width, td::int32 new_height);
  virtual void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) {
    set_need_refresh();
  }
  virtual void handle_input(TickitKeyEventInfo *info) {
  }
  virtual void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
                      bool force) = 0;

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

 private:
  td::int32 width_{10};
  td::int32 height_{10};
  bool is_active_{false};
  std::atomic<bool> need_refresh_{true};

  td::Timestamp refresh_at_;
};

class BorderedWindow : public Window {
 public:
  BorderedWindow(std::shared_ptr<Window> window, BorderType border_type)
      : next_(std::move(window)), border_type_(border_type) {
    switch (border_type_) {
      case BorderType::None:
        hor_border_thic_ = 0;
        vert_border_thic_ = 0;
        break;
      case BorderType::Simple:
      case BorderType::Double:
        hor_border_thic_ = 2;
        vert_border_thic_ = 1;
        break;
    }
  }

  bool need_refresh() override {
    return Window::need_refresh() || next_->need_refresh();
  }
  void set_refreshed() override {
    Window::set_refreshed();
    next_->set_refreshed();
  }
  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    next_->resize(new_width - 2 * hor_border_thic_, new_height - 2 * vert_border_thic_);
  }

  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override;
  void handle_input(TickitKeyEventInfo *info) override;

  td::int32 min_width() override {
    return 2 * hor_border_thic_ + next_->min_width();
  }
  td::int32 min_height() override {
    return 2 * vert_border_thic_ + next_->min_height();
  }
  td::int32 best_width() override {
    return 2 * hor_border_thic_ + next_->best_width();
  }
  td::int32 best_height() override {
    return 2 * vert_border_thic_ + next_->best_height();
  }

 private:
  std::shared_ptr<Window> next_;

  BorderType border_type_;
  td::int32 vert_border_thic_;
  td::int32 hor_border_thic_;
};

class EmptyWindow : public Window {
 public:
  td::int32 min_width() override {
    return 0;
  }
  td::int32 min_height() override {
    return 0;
  }
  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override;
};

}  // namespace windows
