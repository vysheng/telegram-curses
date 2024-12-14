#include "EditorWindow.hpp"
#include "td/utils/Status.h"
#include "td/utils/common.h"

namespace windows {

void EditorWindow::handle_input(const InputEvent &info) {
  set_need_refresh();

  if (info == "T-Left") {
    edit_.move_cursor_left(false);
  } else if (info == "T-Right") {
    edit_.move_cursor_right(false);
  } else if (info == "T-Up") {
    edit_.move_cursor_up();
  } else if (info == "T-Down") {
    edit_.move_cursor_down();
  } else if (info == "T-Home") {
    edit_.go_to_beginning_of_line();
  } else if (info == "T-End") {
    edit_.go_to_end_of_line();
  } else if (info == "T-Backspace") {
    edit_.remove_prev_char();
  } else if (info == "T-Delete") {
    edit_.remove_next_char();
  } else if (info == "T-Enter") {
    edit_.insert_char("\n");
  } else if (info == "T-Tab") {
    edit_.insert_char("\t");
  } else if (info == "M-T-Enter") {
    if (callback_) {
      callback_->on_answer(this, export_data());
      //callback_ = nullptr;
    }
  } else if (info == "T-Escape" || info == "C-q" || info == "C-Q") {
    if (callback_) {
      callback_->on_abort(this, export_data());
      //callback_ = nullptr;
    }
  } else if (info.is_text_key()) {
    edit_.insert_char(info.get_utf8_str());
  }
}

void EditorWindow::render(WindowOutputter &rb, bool force) {
  auto &tmp_rb = empty_window_outputter();
  auto h = edit_.render(tmp_rb, width(), false, false);
  (void)h;

  offset_from_top_ += tmp_rb.local_cursor_y() - last_cursor_y_;
  last_cursor_y_ = tmp_rb.local_cursor_y();
  auto cursor_x = tmp_rb.local_cursor_x();

  if (offset_from_top_ < 0) {
    offset_from_top_ = 0;
  }
  if (offset_from_top_ >= height()) {
    offset_from_top_ = height() - 1;
  }
  if (offset_from_top_ >= h) {
    offset_from_top_ = h - 1;
  }

  rb.translate(offset_from_top_ - last_cursor_y_, 0);
  auto h2 = edit_.render(rb, width(), false, false);
  (void)h2;
  CHECK(h == h2);
  rb.untranslate(offset_from_top_ - last_cursor_y_, 0);
  rb.cursor_move_yx(offset_from_top_, cursor_x, WindowOutputter::CursorShape::Block);

  h += (offset_from_top_ - last_cursor_y_);
  if (h < height()) {
    rb.erase_rect(h, 0, height() - h, width());
  }
}

}  // namespace windows
