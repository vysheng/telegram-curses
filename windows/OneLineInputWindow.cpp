#include "OneLineInputWindow.hpp"

namespace windows {

void OneLineInputWindow::handle_input(const InputEvent &info) {
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
    if (callback_) {
      return callback_->on_answer(this, export_data());
    }
  } else if (info == "T-Tab") {
    edit_.insert_char("\t");
  } else if (info == "M-T-Enter") {
    if (callback_) {
      return callback_->on_answer(this, export_data());
    }
  } else if (info == "T-Escape" || info == "C-q" || info == "C-Q") {
    if (callback_) {
      return callback_->on_abort(this, export_data());
    }
  } else if (info.is_text_key()) {
    edit_.insert_char(info.get_utf8_str());
  }
}

void OneLineInputWindow::render(WindowOutputter &rb, bool force) {
  auto &tmp_rb = empty_window_outputter();
  auto text_width = width() - 1 - (td::int32)prompt_.size();
  auto h = edit_.render(tmp_rb, text_width, true, is_password_);
  (void)h;

  auto line = height() / 2;
  rb.erase_rect(0, 0, height(), width());
  rb.putstr_yx(line, 0, prompt_.data(), prompt_.size());
  rb.translate(line - tmp_rb.local_cursor_y(), 1 + (td::int32)prompt_.size());
  auto h2 = edit_.render(rb, text_width, true, is_password_);
  (void)h2;
  CHECK(h == h2);
  rb.untranslate(line - tmp_rb.local_cursor_y(), 1 + (td::int32)prompt_.size());
}

}  // namespace windows
