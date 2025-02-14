#include "ViewWindow.hpp"
#include "TextEdit.hpp"

namespace windows {

void ViewWindow::handle_input(const InputEvent &info) {
  set_need_refresh();

  if (info == "T-PageUp") {
    offset_from_top_ -= height();
  } else if (info == "T-PageDown") {
    offset_from_top_ += height();
  } else if (info == "T-Enter") {
    if (callback_) {
      return callback_->on_answer(this);
    }
  } else if (info == "M-T-Enter") {
    if (callback_) {
      return callback_->on_answer(this);
    }
  } else if (info == "T-Escape" || info == "C-q" || info == "C-Q") {
    if (callback_) {
      return callback_->on_abort(this);
    }
  } else if (info == " ") {
    offset_from_top_ += height();
  } else if (info == "q" || info == "Q") {
    if (callback_) {
      return callback_->on_abort(this);
    }
  }
}

void ViewWindow::render(WindowOutputter &rb, bool force) {
  auto dir = SavedRenderedImagesDirectory(std::move(saved_images_));

  auto &tmp_rb = empty_window_outputter();
  auto h = TextEdit::render(tmp_rb, width(), text_, 0, markup_, false, false, &dir);
  cached_height_ = h;

  if (offset_from_top_ >= h) {
    offset_from_top_ -= height();
  }
  if (offset_from_top_ < 0) {
    offset_from_top_ = 0;
  }

  rb.erase_rect(0, 0, height(), width());
  rb.translate(-offset_from_top_, 0);
  auto h2 = TextEdit::render(rb, width(), text_, 0, markup_, false, false, &dir);
  LOG_CHECK(h == h2) << h << " " << h2;
  rb.untranslate(-offset_from_top_, 0);
  rb.cursor_move_yx(0, 0, WindowOutputter::CursorShape::None);

  h -= offset_from_top_;
  if (h < height()) {
    rb.erase_rect(h, 0, height() - h, width());
  }

  saved_images_ = dir.release();
}

}  // namespace windows
