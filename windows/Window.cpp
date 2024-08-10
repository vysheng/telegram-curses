#include "Window.hpp"
#include "td/utils/logging.h"

namespace windows {

void Window::resize(td::int32 new_width, td::int32 new_height) {
  if (new_width == width_ && new_height == height_) {
    return;
  }
  set_need_refresh();
  auto old_width = width_;
  auto old_height = height_;
  width_ = new_width;
  height_ = new_height;
  on_resize(old_width, old_height, new_width, new_height);
}

void BorderedWindow::render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y,
                            TickitCursorShape &cursor_shape, bool force) {
  auto rect = TickitRect{.top = vert_border_thic_,
                         .left = hor_border_thic_,
                         .lines = height() - 2 * vert_border_thic_,
                         .cols = width() - 2 * hor_border_thic_};
  tickit_renderbuffer_save(rb);
  tickit_renderbuffer_clip(rb, &rect);
  tickit_renderbuffer_translate(rb, vert_border_thic_, hor_border_thic_);
  if (force || next_->need_refresh()) {
    next_->render(rb, cursor_x, cursor_y, cursor_shape, force);
  }
  cursor_x += hor_border_thic_;
  cursor_y += vert_border_thic_;
  tickit_renderbuffer_restore(rb);
  tickit_renderbuffer_mask(rb, &rect);

  if (vert_border_thic_ != 0) {
    TickitLineStyle line_style = (TickitLineStyle)0;
    switch (border_type_) {
      case BorderType::None:
        break;
      case BorderType::Simple:
        if (is_active()) {
          line_style = TickitLineStyle::TICKIT_LINE_THICK;
        } else {
          line_style = TickitLineStyle::TICKIT_LINE_SINGLE;
        }
        break;
      case BorderType::Double:
        if (is_active()) {
          line_style = TickitLineStyle::TICKIT_LINE_THICK;
        } else {
          line_style = TickitLineStyle::TICKIT_LINE_DOUBLE;
        }
        break;
    }
    tickit_renderbuffer_hline_at(rb, 0, 0, width() - 1, line_style, (TickitLineCaps)0);
    tickit_renderbuffer_hline_at(rb, height() - 1, 0, width() - 1, line_style, (TickitLineCaps)0);
    tickit_renderbuffer_vline_at(rb, 0, height() - 1, 0, line_style, (TickitLineCaps)0);
    tickit_renderbuffer_vline_at(rb, 0, height() - 1, width() - 1, line_style, (TickitLineCaps)0);

    auto r1 = TickitRect{.top = 1, .left = 1, .lines = height() - 2, .cols = 1};
    tickit_renderbuffer_eraserect(rb, &r1);
    r1.left = width() - 2;
    tickit_renderbuffer_eraserect(rb, &r1);
  }
}

void BorderedWindow::handle_input(TickitKeyEventInfo *info) {
  next_->handle_input(info);
}

void EmptyWindow::render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y,
                         TickitCursorShape &cursor_shape, bool force) {
  cursor_x = 0;
  cursor_y = 0;
  cursor_shape = (TickitCursorShape)0;

  if (rb) {
    auto rect = TickitRect{.top = 0, .left = 0, .lines = height(), .cols = width()};
    tickit_renderbuffer_eraserect(rb, &rect);
  }
}

}  // namespace windows
