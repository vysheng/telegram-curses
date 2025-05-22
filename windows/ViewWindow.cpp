#include "ViewWindow.hpp"
#include "TextEdit.hpp"
#include <memory>

namespace windows {

void ViewWindow::alloc_body() {
  body_ = std::make_shared<ViewWindowBody>(this);
  if (height() >= 3) {
    body_->resize(height() - 2, width());
  }
  add_subwindow(body_, 1, 0);
}

void ViewWindow::on_resize(td::int32 old_height, td::int32 old_width, td::int32 new_height, td::int32 new_width) {
  offset_from_top_ = 0;
  if (body_) {
    body_->resize(new_height - 2, new_width);
  }
}

void ViewWindow::handle_input(const InputEvent &info) {
  set_need_refresh();

  if (info == "T-PageUp") {
    offset_from_top_ -= effective_height();
    if (offset_from_top_ < 0) {
      offset_from_top_ = 0;
    }
  } else if (info == "T-PageDown" || info == " ") {
    offset_from_top_ += effective_height();
    if (offset_from_top_ >= cached_height_) {
      offset_from_top_ -= effective_height();
    }
  } else if (info == "j" || info == "T-Down") {
    if (offset_from_top_ + 1 + effective_height() <= cached_height_) {
      offset_from_top_++;
    }
  } else if (info == "k" || info == "T-Up") {
    if (offset_from_top_ > 0) {
      offset_from_top_--;
    }
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
  } else if (info == "q" || info == "Q") {
    if (callback_) {
      return callback_->on_abort(this);
    }
  }
}

void ViewWindow::render_body(WindowOutputter &rb, bool force) {
  auto dir = SavedRenderedImagesDirectory(std::move(saved_images_));

  auto &tmp_rb = empty_window_outputter();
  auto h = TextEdit::render(tmp_rb, width(), text_, 0, markup_, false, false, &dir);
  cached_height_ = h;

  if (offset_from_top_ >= h) {
    offset_from_top_ -= effective_height();
  }
  if (offset_from_top_ < 0) {
    offset_from_top_ = 0;
  }

  rb.erase_rect(0, 0, effective_height(), width());
  rb.translate(-offset_from_top_, 0);
  auto h2 = TextEdit::render(rb, width(), text_, 0, markup_, false, false, &dir);
  LOG_CHECK(h == h2) << h << " " << h2;
  rb.untranslate(-offset_from_top_, 0);
  rb.cursor_move_yx(0, 0, WindowOutputter::CursorShape::None);

  h -= offset_from_top_;
  if (h < effective_height()) {
    rb.erase_rect(h, 0, effective_height() - h, width());
  }

  saved_images_ = dir.release();
}

void ViewWindow::render(WindowOutputter &rb, bool force) {
  if (!body_) {
    render_body(rb, force);
    return;
  }
  {
    rb.erase_rect(0, 0, 1, width());
    rb.erase_rect(height() - 1, 0, 1, width());
  }

  {
    td::int32 lines_over_window = offset_from_top_;
    td::int32 lines_under_window = cached_height_ - (offset_from_top_ + effective_height());

    if (lines_over_window < 0) {
      lines_over_window = 0;
    }

    if (lines_under_window < 0) {
      lines_under_window = 0;
    }

    td::CSlice text;

    td::StringBuilder sb;
    sb << "↑" << lines_over_window << " " << title() << "\n";
    text = sb.as_cslice();
    TextEdit::render(
        rb, width(), text, 0,
        {std::make_shared<MarkupElementFgColor>(MarkupElementPos(0, 0), MarkupElementPos(text.size() + 1, 0),
                                                Color::Grey),
         std::make_shared<MarkupElementNoLb>(MarkupElementPos(0, 0), MarkupElementPos(text.size(), 0), true)},
        rb.is_active(), false);
    sb.clear();

    rb.translate(height() - 1, 0);
    sb << "↓" << lines_under_window << " " << title() << "\n";
    text = sb.as_cslice();
    TextEdit::render(
        rb, width(), text, 0,
        {std::make_shared<MarkupElementFgColor>(MarkupElementPos(0, 0), MarkupElementPos(text.size() + 1, 0),
                                                Color::Grey),
         std::make_shared<MarkupElementNoLb>(MarkupElementPos(0, 0), MarkupElementPos(text.size(), 0), true)},
        rb.is_active(), false);
    rb.untranslate(height() - 1, 0);
  }

  rb.cursor_move_yx(0, 0, WindowOutputter::CursorShape::None);
}

}  // namespace windows
