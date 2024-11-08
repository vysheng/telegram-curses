#include "EditorWindow.hpp"
#include "td/utils/Status.h"
#include "td/utils/common.h"

namespace windows {

void EditorWindow::handle_input(TickitKeyEventInfo *info) {
  set_need_refresh();

  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "Left")) {
      edit_.move_cursor_left(false);
    } else if (!strcmp(info->str, "Right")) {
      edit_.move_cursor_right(false);
    } else if (!strcmp(info->str, "Up")) {
      edit_.move_cursor_up();
    } else if (!strcmp(info->str, "Down")) {
      edit_.move_cursor_down();
    } else if (!strcmp(info->str, "Home")) {
      edit_.go_to_beginning_of_line();
    } else if (!strcmp(info->str, "End")) {
      edit_.go_to_end_of_line();
    } else if (!strcmp(info->str, "Backspace")) {
      edit_.remove_prev_char();
    } else if (!strcmp(info->str, "Delete")) {
      edit_.remove_next_char();
    } else if (!strcmp(info->str, "Enter")) {
      edit_.insert_char("\n");
    } else if (!strcmp(info->str, "Tab")) {
      edit_.insert_char("\t");
    } else if (!strcmp(info->str, "M-Enter")) {
      if (callback_) {
        callback_->on_answer(this, export_data());
        //callback_ = nullptr;
      }
    } else if (!strcmp(info->str, "Escape") || !strcmp(info->str, "C-q") || !strcmp(info->str, "C-Q")) {
      if (callback_) {
        callback_->on_abort(this, export_data());
        //callback_ = nullptr;
      }
    }
  } else {
    edit_.insert_char(info->str);
  }
}

void EditorWindow::render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y,
                          TickitCursorShape &cursor_shape, bool force) {
  auto h = edit_.render(nullptr, cursor_x, cursor_y, cursor_shape, width(), false, false);
  (void)h;

  offset_from_top_ += cursor_y - last_cursor_y_;
  last_cursor_y_ = cursor_y;

  if (offset_from_top_ < 0) {
    offset_from_top_ = 0;
  }
  if (offset_from_top_ >= height()) {
    offset_from_top_ = height() - 1;
  }
  if (offset_from_top_ >= h) {
    offset_from_top_ = h - 1;
  }

  tickit_renderbuffer_save(rb);
  tickit_renderbuffer_translate(rb, offset_from_top_ - last_cursor_y_, 0);
  auto h2 = edit_.render(rb, cursor_x, cursor_y, cursor_shape, width(), false, false);
  (void)h2;
  CHECK(h == h2);
  tickit_renderbuffer_restore(rb);
  cursor_y = offset_from_top_;
  cursor_shape = TickitCursorShape::TICKIT_CURSORSHAPE_BLOCK;

  h += (offset_from_top_ - last_cursor_y_);
  if (h < height()) {
    auto rect = TickitRect{.top = h, .left = 0, .lines = height() - h, .cols = width()};
    tickit_renderbuffer_eraserect(rb, &rect);
  }
}

void OneLineInputWindow::handle_input(TickitKeyEventInfo *info) {
  set_need_refresh();

  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "Left")) {
      edit_.move_cursor_left(false);
    } else if (!strcmp(info->str, "Right")) {
      edit_.move_cursor_right(false);
    } else if (!strcmp(info->str, "Up")) {
      edit_.move_cursor_up();
    } else if (!strcmp(info->str, "Down")) {
      edit_.move_cursor_down();
    } else if (!strcmp(info->str, "Home")) {
      edit_.go_to_beginning_of_line();
    } else if (!strcmp(info->str, "End")) {
      edit_.go_to_end_of_line();
    } else if (!strcmp(info->str, "Backspace")) {
      edit_.remove_prev_char();
    } else if (!strcmp(info->str, "Delete")) {
      edit_.remove_next_char();
    } else if (!strcmp(info->str, "Enter")) {
      if (callback_) {
        return callback_->on_answer(this, export_data());
      }
    } else if (!strcmp(info->str, "Tab")) {
      edit_.insert_char("\t");
    } else if (!strcmp(info->str, "M-Enter")) {
      if (callback_) {
        return callback_->on_answer(this, export_data());
      }
    } else if (!strcmp(info->str, "Escape") || !strcmp(info->str, "C-q") || !strcmp(info->str, "C-Q")) {
      if (callback_) {
        return callback_->on_abort(this, export_data());
      }
    }
  } else {
    edit_.insert_char(info->str);
  }
}

void OneLineInputWindow::render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y,
                                TickitCursorShape &cursor_shape, bool force) {
  auto text_width = width() - 1 - (td::int32)prompt_.size();
  auto h = edit_.render(nullptr, cursor_x, cursor_y, cursor_shape, text_width, true, is_password_);
  (void)h;

  auto line = height() / 2;

  auto rect = TickitRect{.top = 0, .left = 0, .lines = height(), .cols = width()};
  tickit_renderbuffer_eraserect(rb, &rect);
  tickit_renderbuffer_textn_at(rb, line, 0, prompt_.data(), prompt_.size());

  tickit_renderbuffer_save(rb);
  auto rect0 = TickitRect{.top = line, .left = 1 + (td::int32)prompt_.size(), .lines = 1, .cols = text_width};
  tickit_renderbuffer_clip(rb, &rect0);
  tickit_renderbuffer_translate(rb, line - cursor_y, 1 + (td::int32)prompt_.size());
  td::int32 tmp;
  auto h2 = edit_.render(rb, cursor_x, tmp, cursor_shape, text_width, true, is_password_);
  (void)h2;
  CHECK(h == h2);
  tickit_renderbuffer_restore(rb);
  cursor_y = line;
  cursor_x += 1 + (td::int32)prompt_.size();
  cursor_shape = TickitCursorShape::TICKIT_CURSORSHAPE_BLOCK;
}

void ViewWindow::handle_input(TickitKeyEventInfo *info) {
  set_need_refresh();

  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "PageUp")) {
      offset_from_top_ -= height();
    } else if (!strcmp(info->str, "PageDown")) {
      offset_from_top_ += height();
    } else if (!strcmp(info->str, "Enter")) {
      if (callback_) {
        return callback_->on_answer(this);
      }
    } else if (!strcmp(info->str, "M-Enter")) {
      if (callback_) {
        return callback_->on_answer(this);
      }
    } else if (!strcmp(info->str, "Escape") || !strcmp(info->str, "C-q") || !strcmp(info->str, "C-Q")) {
      if (callback_) {
        return callback_->on_abort(this);
      }
    }
  } else {
    if (!strcmp(info->str, " ")) {
      offset_from_top_ += height();
    } else if (!strcmp(info->str, "q") || !strcmp(info->str, "Q")) {
      if (callback_) {
        return callback_->on_abort(this);
      }
    }
  }
}

void ViewWindow::render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y,
                        TickitCursorShape &cursor_shape, bool force) {
  auto h = TextEdit::render(nullptr, cursor_x, cursor_y, cursor_shape, width(), text_, 0, markup_, false, false);

  if (offset_from_top_ >= h) {
    offset_from_top_ -= height();
  }
  if (offset_from_top_ < 0) {
    offset_from_top_ = 0;
  }

  tickit_renderbuffer_save(rb);
  auto rect = TickitRect{.top = 0, .left = 0, .lines = height(), .cols = width()};
  tickit_renderbuffer_clip(rb, &rect);
  tickit_renderbuffer_translate(rb, -offset_from_top_, 0);
  auto h2 = TextEdit::render(rb, cursor_x, cursor_y, cursor_shape, width(), text_, 0, markup_, false, false);
  LOG_CHECK(h == h2) << h << " " << h2;
  tickit_renderbuffer_restore(rb);
  cursor_y = 0;
  cursor_x = 0;
  cursor_shape = (TickitCursorShape)0;

  h -= offset_from_top_;
  if (h < height()) {
    auto rect = TickitRect{.top = h, .left = 0, .lines = height() - h, .cols = width()};
    tickit_renderbuffer_eraserect(rb, &rect);
  }
}

}  // namespace windows
