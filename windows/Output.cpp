#include "Output.hpp"
#include <memory>
#include <tickit.h>
#include "td/utils/logging.h"

namespace windows {

td::int32 color_to_tickit(Color color) {
  return (td::int32)color;
}

class WindowOutputterTickit : public WindowOutputter {
 public:
  WindowOutputterTickit(TickitRenderBuffer *rb, int y_offset, int x_offset, int height, int width)
      : rb_(rb), y_offset_(y_offset), x_offset_(x_offset), height_(height), width_(width) {
    if (rb_) {
      pen_ = tickit_pen_new();
      tickit_renderbuffer_save(rb_);
      auto rect0 = TickitRect{.top = y_offset, .left = x_offset, .lines = height, .cols = width};
      tickit_renderbuffer_clip(rb_, &rect0);
    }
  }
  ~WindowOutputterTickit() {
    if (pen_) {
      tickit_pen_unref(pen_);
      pen_ = nullptr;
    }
    if (rb_) {
      tickit_renderbuffer_restore(rb_);
      auto rect0 = TickitRect{.top = y_offset_, .left = x_offset_, .lines = height_, .cols = width_};
      tickit_renderbuffer_mask(rb_, &rect0);
    }
  }
  td::int32 putstr_yx(td::int32 y, td::int32 x, const char *s, size_t len) override {
    if (!len) {
      len = strlen(s);
    }
    if (rb_) {
      tickit_renderbuffer_setpen(rb_, pen_);
      tickit_renderbuffer_textn_at(rb_, y + y_offset_, x + x_offset_, s, len);
    }
    return (td::int32)len;
  }
  void cursor_move_yx(td::int32 y, td::int32 x, WindowOutputter::CursorShape cursor_shape) override {
    cursor_y_ = y + y_offset_;
    cursor_x_ = x + x_offset_;
    cursor_shape_ = cursor_shape;
  }
  void set_fg_color(Color color) override {
    if (pen_) {
      tickit_pen_set_colour_attr(pen_, TickitPenAttr::TICKIT_PEN_FG, color_to_tickit(color));
    }
  }
  void unset_fg_color() override {
    if (pen_) {
      tickit_pen_clear_attr(pen_, TickitPenAttr::TICKIT_PEN_FG);
      //tickit_pen_set_colour_attr(pen_, TickitPenAttr::TICKIT_PEN_FG, color_to_tickit(Color::White));
    }
  }
  void set_bg_color(Color color) override {
    if (pen_) {
      tickit_pen_set_colour_attr(pen_, TickitPenAttr::TICKIT_PEN_BG, color_to_tickit(color));
    }
  }
  void unset_bg_color() override {
    if (pen_) {
      tickit_pen_clear_attr(pen_, TickitPenAttr::TICKIT_PEN_BG);
      //tickit_pen_set_colour_attr(pen_, TickitPenAttr::TICKIT_PEN_BG, color_to_tickit(Color::Black));
    }
  }
  void set_bold(bool value) override {
    if (pen_) {
      tickit_pen_set_bool_attr(pen_, TickitPenAttr::TICKIT_PEN_BOLD, value ? 1 : 0);
    }
  }
  void unset_bold() override {
    if (pen_) {
      tickit_pen_clear_attr(pen_, TickitPenAttr::TICKIT_PEN_BOLD);
    }
  }
  void set_underline(bool value) override {
    if (pen_) {
      tickit_pen_set_bool_attr(pen_, TickitPenAttr::TICKIT_PEN_UNDER, value ? 1 : 0);
    }
  }
  void unset_underline() override {
    if (pen_) {
      tickit_pen_clear_attr(pen_, TickitPenAttr::TICKIT_PEN_UNDER);
    }
  }
  void set_italic(bool value) override {
    if (pen_) {
      tickit_pen_set_bool_attr(pen_, TickitPenAttr::TICKIT_PEN_ITALIC, value ? 1 : 0);
    }
  }
  void unset_italic() override {
    if (pen_) {
      tickit_pen_clear_attr(pen_, TickitPenAttr::TICKIT_PEN_ITALIC);
    }
  }
  void set_reverse(bool value) override {
    if (pen_) {
      tickit_pen_set_bool_attr(pen_, TickitPenAttr::TICKIT_PEN_REVERSE, value ? 1 : 0);
    }
  }
  void unset_reverse() override {
    if (pen_) {
      tickit_pen_clear_attr(pen_, TickitPenAttr::TICKIT_PEN_REVERSE);
    }
  }
  void set_strike(bool value) override {
    if (pen_) {
      tickit_pen_set_bool_attr(pen_, TickitPenAttr::TICKIT_PEN_STRIKE, value ? 1 : 0);
    }
  }
  void unset_strike() override {
    if (pen_) {
      tickit_pen_clear_attr(pen_, TickitPenAttr::TICKIT_PEN_STRIKE);
    }
  }
  void set_blink(bool value) override {
    if (pen_) {
      tickit_pen_set_bool_attr(pen_, TickitPenAttr::TICKIT_PEN_BLINK, value ? 1 : 0);
    }
  }
  void unset_blink() override {
    if (pen_) {
      tickit_pen_clear_attr(pen_, TickitPenAttr::TICKIT_PEN_BLINK);
    }
  }
  bool is_real() const override {
    return rb_ != nullptr;
  }
  td::int32 local_cursor_y() const override {
    return cursor_y_;
  }
  td::int32 local_cursor_x() const override {
    return cursor_x_;
  }
  td::int32 global_cursor_y() const override {
    return cursor_y_ + y_offset_;
  }
  td::int32 global_cursor_x() const override {
    return cursor_x_ + x_offset_;
  }
  CursorShape cursor_shape() const override {
    return cursor_shape_;
  }
  void translate(td::int32 delta_y, td::int32 delta_x) override {
    y_offset_ += delta_y;
    x_offset_ += delta_x;
  }

  std::unique_ptr<WindowOutputter> create_subwindow_outputter(td::int32 y_offset, td::int32 x_offset, td::int32 height,
                                                              td::int32 width) override {
    tickit_renderbuffer_setpen(rb_, pen_);
    return std::make_unique<WindowOutputterTickit>(rb_, y_offset + y_offset_, x_offset + x_offset_, height, width);
  }

  void update_cursor_position_from(WindowOutputter &from) override {
    cursor_y_ = from.global_cursor_y();
    cursor_x_ = from.global_cursor_x();
    cursor_shape_ = from.cursor_shape();
  }

 private:
  TickitRenderBuffer *rb_{nullptr};
  int y_offset_;
  int x_offset_;
  int height_;
  int width_;
  TickitPen *pen_{nullptr};
  td::int32 cursor_y_{0};
  td::int32 cursor_x_{0};
  CursorShape cursor_shape_{CursorShape::Block};
};

class WindowOutputterEmpty : public WindowOutputter {
 public:
  WindowOutputterEmpty() {
  }
  ~WindowOutputterEmpty() {
  }
  td::int32 putstr_yx(td::int32 y, td::int32 x, const char *s, size_t len) override {
    if (!len) {
      len = strlen(s);
    }
    return (td::int32)len;
  }
  void cursor_move_yx(td::int32 y, td::int32 x, WindowOutputter::CursorShape cursor_shape) override {
    cursor_y_ = y;
    cursor_x_ = x;
    cursor_shape_ = cursor_shape;
  }
  void set_fg_color(Color color) override {
  }
  void unset_fg_color() override {
  }
  void set_bg_color(Color color) override {
  }
  void unset_bg_color() override {
  }
  void set_bold(bool value) override {
  }
  void unset_bold() override {
  }
  void set_underline(bool value) override {
  }
  void unset_underline() override {
  }
  void set_italic(bool value) override {
  }
  void unset_italic() override {
  }
  void set_reverse(bool value) override {
  }
  void unset_reverse() override {
  }
  void set_strike(bool value) override {
  }
  void unset_strike() override {
  }
  void set_blink(bool value) override {
  }
  void unset_blink() override {
  }
  bool is_real() const override {
    return false;
  }
  td::int32 local_cursor_y() const override {
    return cursor_y_;
  }
  td::int32 local_cursor_x() const override {
    return cursor_x_;
  }
  td::int32 global_cursor_y() const override {
    return cursor_y_;
  }
  td::int32 global_cursor_x() const override {
    return cursor_x_;
  }
  CursorShape cursor_shape() const override {
    return cursor_shape_;
  }
  void translate(td::int32 delta_y, td::int32 delta_x) override {
  }

  std::unique_ptr<WindowOutputter> create_subwindow_outputter(td::int32 y_offset, td::int32 x_offset, td::int32 height,
                                                              td::int32 width) override {
    return std::make_unique<WindowOutputterEmpty>();
  }
  void update_cursor_position_from(WindowOutputter &from) override {
    cursor_y_ = from.global_cursor_y();
    cursor_x_ = from.global_cursor_x();
    cursor_shape_ = from.cursor_shape();
  }

 private:
  td::int32 cursor_y_{0};
  td::int32 cursor_x_{0};
  CursorShape cursor_shape_{CursorShape::Block};
};

WindowOutputter &empty_window_outputter() {
  static WindowOutputterEmpty out;
  return out;
}

std::unique_ptr<WindowOutputter> tickit_window_outputter(void *rb, td::int32 height, td::int32 width) {
  return std::make_unique<WindowOutputterTickit>((TickitRenderBuffer *)rb, 0, 0, height, width);
}

}  // namespace windows
