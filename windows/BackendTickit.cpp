#include "BackendTickit.h"
#include "Output.hpp"
#include "Window.hpp"
#include "unicode.h"
#include <tickit.h>

namespace windows {

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
  void set_fg_color_rgb(td::uint32 color) override {
    if (pen_) {
      tickit_pen_set_colour_attr(pen_, TickitPenAttr::TICKIT_PEN_FG, color_to_tickit(Color::White));
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
  void set_bg_color_rgb(td::uint32 color) override {
    if (pen_) {
      tickit_pen_set_colour_attr(pen_, TickitPenAttr::TICKIT_PEN_BG, color_to_tickit(Color::Black));
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

  std::unique_ptr<WindowOutputter> create_subwindow_outputter(BackendWindow *bw, td::int32 y_offset, td::int32 x_offset,
                                                              td::int32 height, td::int32 width,
                                                              bool is_active) override {
    tickit_renderbuffer_setpen(rb_, pen_);
    return std::make_unique<WindowOutputterTickit>(rb_, y_offset + y_offset_, x_offset + x_offset_, height, width,
                                                   is_active);
  }

  void update_cursor_position_from(WindowOutputter &from, BackendWindow *bw, td::int32 y_offset,
                                   td::int32 x_offset) override {
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

class WindowOutputterEmptyTickit : public WindowOutputter {
 public:
  WindowOutputterEmptyTickit() {
  }
  ~WindowOutputterEmptyTickit() {
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
  void set_fg_color_rgb(td::uint32 color) override {
  }
  void unset_fg_color() override {
  }
  void set_bg_color(Color color) override {
  }
  void set_bg_color_rgb(td::uint32 color) override {
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

  std::unique_ptr<WindowOutputter> create_subwindow_outputter(BackendWindow *bw, td::int32 y_offset, td::int32 x_offset,
                                                              td::int32 height, td::int32 width,
                                                              bool is_active) override {
    return std::make_unique<WindowOutputterEmptyTickit>();
  }
  void update_cursor_position_from(WindowOutputter &from, BackendWindow *bw, td::int32 y_offset,
                                   td::int32 x_offset) override {
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

void create_empty_window_outputter_libtickit() {
  set_empty_window_outputter(std::make_unique<WindowOutputterEmptyTickit>());
}

std::unique_ptr<WindowOutputter> tickit_window_outputter(void *rb, td::int32 height, td::int32 width) {
  return std::make_unique<WindowOutputterTickit>((TickitRenderBuffer *)rb, 0, 0, height, width, true);
}

struct BackendTickit : public Backend {
  Tickit *tickit_root_{nullptr};
  TickitTerm *tickit_term_{nullptr};
  TickitWindow *tickit_root_window_{nullptr};

  bool stop() override {
    if (tickit_root_) {
      tickit_window_destroy(tickit_root_window_);
      tickit_unref(tickit_root_);
      tickit_root_ = nullptr;
      tickit_term_ = nullptr;
      tickit_root_window_ = nullptr;
      return true;
    } else {
      return false;
    }
  }

  void on_resize() override {
    tickit_term_refresh_size(tickit_term_);
  }

  td::int32 height() override {
    int lines, cols;
    tickit_term_get_size(tickit_term_, &lines, &cols);
    return lines;
  }

  td::int32 width() override {
    int lines, cols;
    tickit_term_get_size(tickit_term_, &lines, &cols);
    return cols;
  }

  void tick() override {
    tickit_tick(tickit_root_, TICKIT_RUN_NOHANG);
  }

  void refresh(bool force, std::shared_ptr<Window> base_window) override {
    TickitRenderBuffer *tickit_rb = tickit_renderbuffer_new(height(), width());
    CHECK(tickit_rb);

    td::int32 cursor_y = 0, cursor_x = 0;
    WindowOutputter::CursorShape cursor_shape = WindowOutputter::CursorShape::None;
    {
      auto rb = tickit_window_outputter(tickit_rb, height(), width());
      base_window->render_wrap(*rb, force);
      cursor_y = rb->global_cursor_y();
      cursor_x = rb->global_cursor_x();
      cursor_shape = rb->cursor_shape();
    }

    if (cursor_shape != WindowOutputter::CursorShape::None) {
      auto shape = [](WindowOutputter::CursorShape cursor_shape) -> TickitCursorShape {
        switch (cursor_shape) {
          case WindowOutputter::CursorShape::Block:
            return TickitCursorShape::TICKIT_CURSORSHAPE_BLOCK;
          case WindowOutputter::CursorShape::Underscore:
            return TickitCursorShape::TICKIT_CURSORSHAPE_UNDER;
          case WindowOutputter::CursorShape::LeftBar:
            return TickitCursorShape::TICKIT_CURSORSHAPE_LEFT_BAR;
          default:
            return TickitCursorShape::TICKIT_CURSORSHAPE_LEFT_BAR;
        }
      }(cursor_shape);
      tickit_window_set_cursor_shape(tickit_root_window_, shape);
      tickit_window_set_cursor_visible(tickit_root_window_, true);
    } else {
      tickit_window_set_cursor_visible(tickit_root_window_, false);
    }
    tickit_window_set_cursor_position(tickit_root_window_, cursor_y, cursor_x);
    tickit_window_flush(tickit_root_window_);

    tickit_renderbuffer_flush_to_term(tickit_rb, tickit_term_);
    tickit_renderbuffer_destroy(tickit_rb);

    tickit_term_flush(tickit_term_);
  }

  td::int32 poll_fd() override {
    return 0;
  }
};

void init_tickit_backend(Screen *screen) {
  set_tickit_wrap();

  auto backend = std::make_unique<BackendTickit>();

  backend->tickit_root_ = tickit_new_stdtty();
  CHECK(backend->tickit_root_);
  backend->tickit_term_ = tickit_get_term(backend->tickit_root_);
  CHECK(backend->tickit_term_);
  td::int32 value;
  CHECK(tickit_term_getctl_int(backend->tickit_term_, TickitTermCtl::TICKIT_TERMCTL_MOUSE, &value));
  CHECK(value == TickitTermMouseMode::TICKIT_TERM_MOUSEMODE_OFF);

  int lines, cols;
  tickit_term_get_size(backend->tickit_term_, &lines, &cols);

  auto handle_resize = [](TickitTerm *tt, TickitEventFlags flags, void *_info, void *data) {
    TickitResizeEventInfo *info = (TickitResizeEventInfo *)_info;
    static_cast<Screen *>(data)->on_resize(info->lines, info->cols);
    return 1;
  };
  tickit_term_bind_event(backend->tickit_term_, TICKIT_TERM_ON_RESIZE, (TickitBindFlags)0, handle_resize, screen);

  auto handle_input = [](TickitTerm *tt, TickitEventFlags flags, void *_info, void *data) {
    TickitKeyEventInfo *info = (TickitKeyEventInfo *)_info;
    if (info->type == TICKIT_KEYEV_KEY || info->type == TICKIT_KEYEV_TEXT) {
      auto r = parse_tickit_input_event(td::CSlice(info->str), info->type == TICKIT_KEYEV_KEY);
      static_cast<Screen *>(data)->handle_input(*r);
    }
    return 1;
  };
  tickit_term_bind_event(backend->tickit_term_, TICKIT_TERM_ON_KEY, (TickitBindFlags)0, handle_input, screen);

  backend->tickit_root_window_ = tickit_window_new_root(backend->tickit_term_);
  tickit_window_take_focus(backend->tickit_root_window_);

  tickit_term_observe_sigwinch(backend->tickit_term_, true);

  create_empty_window_outputter_libtickit();

  screen->set_backend(std::move(backend));
}

}  // namespace windows
