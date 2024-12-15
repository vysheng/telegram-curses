#include "Output.hpp"
#include <memory>
#include <tickit.h>
#include <notcurses/notcurses.h>
#include <utility>
#include <vector>
#include "td/utils/logging.h"

namespace windows {

std::vector<int> color_to_rgb{0x000000, 0xcc0403, 0x19cb00, 0xcecb00, 0x0d73cc, 0xcb1ed1, 0x0dcdcd, 0xdddddd,
                              0x767676, 0xf2201f, 0x23fd00, 0xfffd00, 0x1a8fff, 0xfd28ff, 0x14ffff, 0xffffff};

td::int32 color_to_tickit(Color color) {
  return (td::int32)color;
}

class WindowOutputterTickit : public WindowOutputter {
 public:
  WindowOutputterTickit(TickitRenderBuffer *rb, int y_offset, int x_offset, int height, int width, bool is_active)
      : rb_(rb), y_offset_(y_offset), x_offset_(x_offset), height_(height), width_(width), is_active_(is_active) {
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
                                                              td::int32 width, bool is_active) override {
    tickit_renderbuffer_setpen(rb_, pen_);
    return std::make_unique<WindowOutputterTickit>(rb_, y_offset + y_offset_, x_offset + x_offset_, height, width,
                                                   is_active);
  }

  void update_cursor_position_from(WindowOutputter &from) override {
    cursor_y_ = from.global_cursor_y();
    cursor_x_ = from.global_cursor_x();
    cursor_shape_ = from.cursor_shape();
  }

  bool is_active() const override {
    return is_active_;
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
  bool is_active_;
};

class WindowOutputterNotcurses : public WindowOutputter {
 public:
  WindowOutputterNotcurses(struct ncplane *rb, int y_offset, int x_offset, int height, int width,
                           td::uint32 default_fg_channel, td::uint32 default_bg_channel, bool is_active)
      : rb_(rb)
      , base_y_offset_(y_offset)
      , base_x_offset_(x_offset)
      , y_offset_(y_offset)
      , x_offset_(x_offset)
      , height_(height)
      , width_(width)
      , is_active_(is_active) {
    fg_channels_.push_back(default_fg_channel);
    bg_channels_.push_back(default_bg_channel);
    set_channels();
    /*if (rb_) {
      pen_ = tickit_pen_new();
      tickit_renderbuffer_save(rb_);
      auto rect0 = TickitRect{.top = y_offset, .left = x_offset, .lines = height, .cols = width};
      tickit_renderbuffer_clip(rb_, &rect0);
    }*/
  }
  ~WindowOutputterNotcurses() {
    /*if (pen_) {
      tickit_pen_unref(pen_);
      pen_ = nullptr;
    }
    if (rb_) {
      tickit_renderbuffer_restore(rb_);
      auto rect0 = TickitRect{.top = y_offset_, .left = x_offset_, .lines = height_, .cols = width_};
      tickit_renderbuffer_mask(rb_, &rect0);
    }*/
  }
  td::int32 putstr_yx(td::int32 y, td::int32 x, const char *s, size_t len) override {
    if (y + y_offset_ < base_y_offset_ || y + y_offset_ >= base_y_offset_ + height_) {
      return (td::int32)len;
    }
    if (x + x_offset_ < base_x_offset_ || x + x_offset_ >= base_x_offset_ + width_) {
      return (td::int32)len;
    }
    if (!len) {
      len = strlen(s);
    }
    if (rb_) {
      //tickit_renderbuffer_setpen(rb_, pen_);

      char *tmp = strndup(s, len);

      ncplane_putstr_yx(rb_, y + y_offset_, x + x_offset_, tmp);

      free(tmp);
    }
    return (td::int32)len;
  }
  void cursor_move_yx(td::int32 y, td::int32 x, WindowOutputter::CursorShape cursor_shape) override {
    cursor_y_ = y + y_offset_;
    cursor_x_ = x + x_offset_;
    cursor_shape_ = cursor_shape;
  }

  void set_channels() {
    auto fg = fg_channels_.back();
    auto bg = bg_channels_.back();
    if (is_reversed_) {
      std::swap(fg, bg);
    }
    auto chan = NCCHANNELS_INITIALIZER((fg >> 16) & 0xff, (fg >> 8) & 0xff, (fg) & 0xff, (bg >> 16) & 0xff,
                                       (bg >> 8) & 0xff, (bg) & 0xff);
    ncplane_set_channels(rb_, chan);
  }

  void set_fg_color(Color color) override {
    fg_channels_.push_back(color_to_rgb[(td::int32)color]);
    set_channels();
  }
  void unset_fg_color() override {
    CHECK(fg_channels_.size() > 1);
    fg_channels_.pop_back();
    set_channels();
  }
  void set_bg_color(Color color) override {
    bg_channels_.push_back(color_to_rgb[(td::int32)color]);
    set_channels();
  }
  void unset_bg_color() override {
    CHECK(bg_channels_.size() > 1);
    bg_channels_.pop_back();
    set_channels();
  }
  void set_bold(bool value) override {
    if (value) {
      ncplane_on_styles(rb_, NCSTYLE_BOLD);
    } else {
      ncplane_off_styles(rb_, NCSTYLE_BOLD);
    }
  }
  void unset_bold() override {
    ncplane_off_styles(rb_, NCSTYLE_BOLD);
  }
  void set_underline(bool value) override {
    if (value) {
      ncplane_on_styles(rb_, NCSTYLE_UNDERLINE);
    } else {
      ncplane_off_styles(rb_, NCSTYLE_UNDERLINE);
    }
  }
  void unset_underline() override {
    ncplane_off_styles(rb_, NCSTYLE_UNDERLINE);
  }
  void set_italic(bool value) override {
    if (value) {
      ncplane_on_styles(rb_, NCSTYLE_ITALIC);
    } else {
      ncplane_off_styles(rb_, NCSTYLE_ITALIC);
    }
  }
  void unset_italic() override {
    ncplane_off_styles(rb_, NCSTYLE_ITALIC);
  }
  void set_reverse(bool value) override {
    is_reversed_ = value;
    set_channels();
  }
  void unset_reverse() override {
    is_reversed_ = false;
    set_channels();
  }
  void set_strike(bool value) override {
    if (value) {
      ncplane_on_styles(rb_, NCSTYLE_STRUCK);
    } else {
      ncplane_off_styles(rb_, NCSTYLE_STRUCK);
    }
  }
  void unset_strike() override {
    ncplane_off_styles(rb_, NCSTYLE_STRUCK);
  }
  void set_blink(bool value) override {
  }
  void unset_blink() override {
  }
  bool is_real() const override {
    return rb_ != nullptr;
  }
  td::int32 local_cursor_y() const override {
    return cursor_y_ - base_y_offset_;
  }
  td::int32 local_cursor_x() const override {
    return cursor_x_ - base_x_offset_;
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
    y_offset_ += delta_y;
    x_offset_ += delta_x;
  }

  std::unique_ptr<WindowOutputter> create_subwindow_outputter(td::int32 y_offset, td::int32 x_offset, td::int32 height,
                                                              td::int32 width, bool is_active) override {
    //tickit_renderbuffer_setpen(rb_, pen_);
    return std::make_unique<WindowOutputterNotcurses>(rb_, y_offset + y_offset_, x_offset + x_offset_, height, width,
                                                      color_to_rgb[(td::int32)(is_active ? Color::White : Color::Grey)],
                                                      color_to_rgb[(td::int32)Color::Black], is_active);
  }

  void update_cursor_position_from(WindowOutputter &from) override {
    cursor_y_ = from.global_cursor_y();
    cursor_x_ = from.global_cursor_x();
    cursor_shape_ = from.cursor_shape();
  }

  bool is_active() const override {
    return is_active_;
  }

 private:
  struct ncplane *rb_{nullptr};
  int base_y_offset_;
  int base_x_offset_;
  int y_offset_;
  int x_offset_;
  int height_;
  int width_;

  std::vector<td::uint32> fg_channels_;
  std::vector<td::uint32> bg_channels_;
  bool is_reversed_{false};

  //TickitPen *pen_{nullptr};
  td::int32 cursor_y_{0};
  td::int32 cursor_x_{0};
  CursorShape cursor_shape_{CursorShape::Block};

  bool is_active_;
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
                                                              td::int32 width, bool is_active) override {
    return std::make_unique<WindowOutputterEmpty>();
  }
  void update_cursor_position_from(WindowOutputter &from) override {
    cursor_y_ = from.global_cursor_y();
    cursor_x_ = from.global_cursor_x();
    cursor_shape_ = from.cursor_shape();
  }
  bool is_active() const override {
    return false;
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
  return std::make_unique<WindowOutputterTickit>((TickitRenderBuffer *)rb, 0, 0, height, width, true);
}

std::unique_ptr<WindowOutputter> notcurses_window_outputter(void *rb, td::int32 height, td::int32 width) {
  return std::make_unique<WindowOutputterNotcurses>((struct ncplane *)rb, 0, 0, height, width, 0xdddddd, 0x000000,
                                                    true);
}

}  // namespace windows
