#include "PadWindow.hpp"
#include "stdio.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/StringBuilder.h"
#include <memory>
#include <vector>

namespace windows {

void PadWindow::scroll_up(td::int32 lines) {
  glued_to_ = GluedTo::None;
  adjust_cur_element(-lines);
}

void PadWindow::scroll_prev_element() {
  if (!cur_element_) {
    return;
  }
  glued_to_ = GluedTo::None;
  if (offset_in_cur_element_ > 0) {
    adjust_cur_element(-offset_in_cur_element_);
  } else {
    auto it = elements_.find(cur_element_->element.get());
    if (it != elements_.begin()) {
      it--;
      adjust_cur_element(-it->second->height);
    } else {
      adjust_cur_element(-1);
    }
  }
}

void PadWindow::scroll_next_element() {
  if (!cur_element_) {
    return;
  }
  adjust_cur_element(cur_element_->height - offset_in_cur_element_);
}

void PadWindow::scroll_down(td::int32 lines) {
  glued_to_ = GluedTo::None;
  adjust_cur_element(lines);
}

void PadWindow::scroll_first_line() {
  glued_to_ = GluedTo::Top;
  adjust_cur_element(-1000000000);
}

void PadWindow::scroll_last_line() {
  glued_to_ = GluedTo::Bottom;
  adjust_cur_element(1000000000);
}

void PadWindow::handle_input(TickitKeyEventInfo *info) {
  set_need_refresh();
  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "PageUp")) {
      scroll_up(effective_height() / 2);
    } else if (!strcmp(info->str, "Up")) {
      scroll_up(1);
    } else if (!strcmp(info->str, "PageDown")) {
      scroll_down(effective_height() / 2);
    } else if (!strcmp(info->str, "Down")) {
      scroll_down(1);
    } else if (!strcmp(info->str, "C-u")) {
      scroll_up(effective_height() / 2);
    } else if (!strcmp(info->str, "C-d")) {
      scroll_down(effective_height() / 2);
    } else if (!strcmp(info->str, "C-b")) {
      scroll_up(effective_height() - 1);
    } else if (!strcmp(info->str, "C-f")) {
      scroll_down(effective_height() - 1);
    } else {
      pad_handle_input(info);
    }
  } else {
    if (!strcmp(info->str, "k")) {
      scroll_up(1);
    } else if (!strcmp(info->str, "j")) {
      scroll_down(1);
    } else if (!strcmp(info->str, "g")) {
      scroll_first_line();
    } else if (!strcmp(info->str, "G")) {
      scroll_last_line();
    } else if (!strcmp(info->str, "n")) {
      scroll_next_element();
    } else if (!strcmp(info->str, "N")) {
      scroll_prev_element();
    } else {
      pad_handle_input(info);
    }
  }
}

void PadWindow::on_resize(td::int32, td::int32 old_height, td::int32 new_width, td::int32 new_height) {
  if (elements_.size() == 0) {
    return;
  }

  for (auto &p : elements_) {
    auto &el = *p.second;
    auto old_height = el.height;
    el.element->change_width(new_width);
    el.height = el.element->render(*this, nullptr, cur_element_ == nullptr);
    auto new_height = el.height;

    if (el.element->is_less(*cur_element_->element)) {
      lines_before_cur_element_ += new_height - old_height;
    } else if (cur_element_->element->is_less(*el.element)) {
      lines_after_cur_element_ += new_height - old_height;
    } else {
      CHECK(cur_element_ == &el);
      auto p = offset_in_cur_element_ * 1.0 / old_height;
      CHECK(p < 1.0);
      offset_in_cur_element_ = (td::int32)(p * new_height);
    }
  }

  if (old_height != new_height) {
    if (pad_to_ == PadTo::Bottom) {
      offset_from_window_top_ += (new_height - old_height);
    }
  }

  adjust_cur_element(0);
}

void PadWindow::change_element(PadWindowElement *elem) {
  set_need_refresh();
  auto it = elements_.find(elem);
  if (it == elements_.end()) {
    return;
  }

  auto &el = *it->second;

  auto old_height = it->second->height;

  el.height = el.element->render(*this, nullptr, it->second.get() == cur_element_);

  auto new_height = it->second->height;

  if (cur_element_->element->is_less(*elem)) {
    lines_after_cur_element_ += new_height - old_height;
  } else if (elem->is_less(*cur_element_->element)) {
    lines_before_cur_element_ += new_height - old_height;
  } else {
    CHECK(cur_element_ == &el);
    auto p = offset_in_cur_element_ * 1.0 / old_height;
    CHECK(p < 1.0);
    offset_in_cur_element_ = (td::int32)(p * new_height);
  }

  adjust_cur_element(0);
}

void PadWindow::change_element(std::shared_ptr<PadWindowElement> elem, std::function<void()> change) {
  set_need_refresh();
  auto it = elements_.find(elem.get());
  if (it == elements_.end()) {
    change();
    if (elem->is_visible()) {
      add_element(std::move(elem));
    }
    return;
  }

  auto old_height = it->second->height;

  if (elem->is_less(*cur_element_->element)) {
    lines_before_cur_element_ -= it->second->height;
    CHECK(lines_before_cur_element_ >= 0);
    auto ptr = std::move(it->second);
    elements_.erase(it);
    change();
    if (elem->is_visible()) {
      it = elements_.emplace(elem.get(), std::move(ptr)).first;
      auto &el = *it->second;
      el.height = el.element->render(*this, nullptr, it->second.get() == cur_element_);
      if (cur_element_->element->is_less(*elem)) {
        lines_after_cur_element_ += it->second->height;
      } else {
        lines_before_cur_element_ += it->second->height;
      }
    }
  } else if (cur_element_->element->is_less(*elem)) {
    lines_after_cur_element_ -= it->second->height;
    auto ptr = std::move(it->second);
    elements_.erase(it);
    change();
    if (elem->is_visible()) {
      it = elements_.emplace(elem.get(), std::move(ptr)).first;
      auto &el = *it->second;
      el.height = el.element->render(*this, nullptr, it->second.get() == cur_element_);
      if (cur_element_->element->is_less(*elem)) {
        lines_after_cur_element_ += it->second->height;
      } else {
        lines_before_cur_element_ += it->second->height;
      }
    }
  } else {
    auto tot_height = lines_after_cur_element_ + lines_before_cur_element_;
    CHECK(cur_element_ == it->second.get());
    auto p = offset_in_cur_element_ * 1.0 / old_height;
    CHECK(p < 1.0);
    auto ptr = std::move(it->second);
    auto it2 = it;
    it2++;
    elements_.erase(it);
    change();
    it = elements_.emplace(elem.get(), std::move(ptr)).first;
    auto &el = *it->second;
    bool unchanged_pos = false;
    if (it2 != elements_.begin()) {
      it2--;
      if (it2 == it) {
        unchanged_pos = true;
      }
    }
    if (!unchanged_pos) {
      lines_before_cur_element_ = 0;
      for (auto it3 = elements_.begin(); it3 != it; it3++) {
        lines_before_cur_element_ += it3->second->height;
      }
      lines_after_cur_element_ = tot_height - lines_before_cur_element_;
    }

    el.height = el.element->render(*this, nullptr, it->second.get() == cur_element_);
    auto new_height = it->second->height;
    offset_in_cur_element_ = (td::int32)(p * new_height);
    if (!elem->is_visible()) {
      return delete_element(elem.get());
    }
  }

  CHECK(lines_before_cur_element_ >= 0);
  adjust_cur_element(0);
}

void PadWindow::delete_element(PadWindowElement *elem) {
  set_need_refresh();
  auto it = elements_.find(elem);
  if (it == elements_.end()) {
    return;
  }

  CHECK(cur_element_);

  auto &el = *it->second;

  auto old_height = el.height;
  auto old_it = it;

  if (cur_element_->element->is_less(*elem)) {
    lines_after_cur_element_ -= old_height;
    CHECK(lines_before_cur_element_ >= 0);
  } else if (elem->is_less(*cur_element_->element)) {
    lines_before_cur_element_ -= old_height;
    CHECK(lines_before_cur_element_ >= 0);
  } else {
    if (it != elements_.begin()) {
      it--;
      cur_element_ = it->second.get();
      lines_before_cur_element_ -= it->second->height;
      offset_in_cur_element_ = it->second->height - 1;
      CHECK(lines_before_cur_element_ >= 0);
    } else {
      CHECK(lines_before_cur_element_ == 0);
      it++;
      if (it != elements_.end()) {
        cur_element_ = it->second.get();
        lines_after_cur_element_ -= it->second->height;
        offset_in_cur_element_ = 0;
      } else {
        cur_element_ = nullptr;
        offset_in_cur_element_ = 0;
        offset_from_window_top_ = 0;
        CHECK(!lines_after_cur_element_);
        CHECK(!lines_before_cur_element_);
      }
    }
  }

  elements_.erase(old_it);

  adjust_cur_element(0);
}

void PadWindow::add_element(std::shared_ptr<PadWindowElement> element) {
  CHECK(element);
  set_need_refresh();
  auto elem = element.get();
  auto it = elements_.find(elem);
  if (it != elements_.end()) {
    LOG(WARNING) << "not adding, already an element";
    return;
  }

  auto x = elements_.emplace(elem, std::make_unique<ElementInfo>(std::move(element)));
  CHECK(x.second);
  it = x.first;

  auto &el = *it->second;
  CHECK(el.element);
  el.element->change_width(width());

  el.height = el.element->render(*this, nullptr, cur_element_ == nullptr);

  auto new_height = it->second->height;

  if (!cur_element_) {
    cur_element_ = &el;
    offset_in_cur_element_ = glued_to_ == GluedTo::Bottom ? cur_element_->height - 1 : 0;
    offset_from_window_top_ = offset_in_cur_element_;
  } else if (cur_element_->element->is_less(*elem)) {
    lines_after_cur_element_ += new_height;
  } else if (elem->is_less(*cur_element_->element)) {
    lines_before_cur_element_ += new_height;
  } else {
    UNREACHABLE();
  }

  adjust_cur_element(0);
}

void PadWindow::adjust_cur_element(td::int32 lines) {
  if (elements_.size() == 0) {
    request_top_elements();
    request_bottom_elements();
    CHECK(!cur_element_);
    CHECK(offset_in_cur_element_ == 0);
    CHECK(offset_from_window_top_ == 0);
    CHECK(!lines_after_cur_element_);
    CHECK(!lines_before_cur_element_);
    return;
  }

  if (glued_to_ == GluedTo::Top) {
    auto h = pad_height();
    cur_element_ = elements_.begin()->second.get();
    offset_in_cur_element_ = 0;
    offset_from_window_top_ = 0;
    lines_before_cur_element_ = 0;
    lines_after_cur_element_ = h - cur_element_->height;
  } else if (glued_to_ == GluedTo::Bottom) {
    auto h = pad_height();
    cur_element_ = elements_.rbegin()->second.get();
    offset_in_cur_element_ = cur_element_->height - 1;
    offset_from_window_top_ = h;
    lines_before_cur_element_ = h - cur_element_->height;
    lines_after_cur_element_ = 0;
  } else if (lines > 0) {
    auto it = elements_.find(cur_element_->element.get());
    CHECK(it != elements_.end());
    auto old_lines = lines;
    while (lines > 0) {
      if (offset_in_cur_element_ < it->second->height - 1) {
        auto l = it->second->height - offset_in_cur_element_ - 1;
        if (l < lines) {
          lines -= l;
          offset_in_cur_element_ += l;
        } else {
          offset_in_cur_element_ += lines;
          lines = 0;
          break;
        }
      }
      CHECK(lines > 0);
      auto old_height = it->second->height;
      it++;
      if (it == elements_.end()) {
        break;
      } else {
        cur_element_ = it->second.get();
        offset_in_cur_element_ = 0;
        lines--;
        lines_before_cur_element_ += old_height;
        lines_after_cur_element_ -= it->second->height;
      }
    }
    if (lines > 0) {
      glued_to_ = GluedTo::Bottom;
    }
    offset_from_window_top_ += (old_lines - lines);
  } else if (lines < 0) {
    lines = -lines;
    auto it = elements_.find(cur_element_->element.get());
    CHECK(it != elements_.end());
    auto old_lines = lines;
    while (lines > 0) {
      if (offset_in_cur_element_ > 0) {
        auto l = offset_in_cur_element_;
        if (l < lines) {
          lines -= l;
          offset_in_cur_element_ = 0;
        } else {
          offset_in_cur_element_ -= lines;
          lines = 0;
          break;
        }
      }
      CHECK(lines > 0);
      auto old_height = it->second->height;
      if (it == elements_.begin()) {
        break;
      } else {
        it--;
        cur_element_ = it->second.get();
        offset_in_cur_element_ = cur_element_->height - 1;
        lines--;
        lines_after_cur_element_ += old_height;
        lines_before_cur_element_ -= it->second->height;
      }
    }
    if (lines > 0) {
      glued_to_ = GluedTo::Top;
    }
    offset_from_window_top_ -= (old_lines - lines);
  }

  if (offset_from_window_top_ < 0) {
    offset_from_window_top_ = 0;
  }
  if (offset_from_window_top_ >= effective_height()) {
    offset_from_window_top_ = effective_height() - 1;
  }
  if (pad_to_ == PadTo::Top) {
    auto off = offset_in_cur_element_ + lines_before_cur_element_;
    CHECK(offset_in_cur_element_ >= 0);
    CHECK(lines_before_cur_element_ >= 0);
    if (offset_from_window_top_ > off) {
      offset_from_window_top_ = off;
    }
  } else {
    auto off = cur_element_->height - offset_in_cur_element_ + lines_after_cur_element_;
    CHECK(cur_element_->height - offset_in_cur_element_ >= 0);
    CHECK(lines_after_cur_element_ >= 0);
    if (offset_from_window_top_ + off < effective_height()) {
      offset_from_window_top_ = effective_height() - off;
    }
  }

  if (lines_before_cur_element_ < 100) {
    request_top_elements();
  }
  if (lines_after_cur_element_ < 100) {
    request_bottom_elements();
  }
}

void PadWindow::scroll_to_element(PadWindowElement *elptr, bool top) {
  auto it = elements_.find(elptr);
  if (it == elements_.end()) {
    return;
  }
  auto el = it->second.get();
  cur_element_ = el;
  offset_from_window_top_ = 0;
  offset_in_cur_element_ = 0;
  lines_before_cur_element_ = 0;
  lines_after_cur_element_ = 0;
  for (auto &e : elements_) {
    if (e.first->is_less(*elptr)) {
      lines_before_cur_element_ += e.second->height;
    } else if (elptr->is_less(*e.first)) {
      lines_after_cur_element_ += e.second->height;
    }
  }

  glued_to_ = GluedTo::None;
  adjust_cur_element(0);
  set_need_refresh();

  if (lines_before_cur_element_ < 100) {
    request_top_elements();
  }
  if (lines_after_cur_element_ < 100) {
    request_bottom_elements();
  }
}

void PadWindow::render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y,
                       TickitCursorShape &cursor_shape, bool force) {
  {
    auto rect = TickitRect{.top = 0, .left = 0, .lines = 1, .cols = width()};
    tickit_renderbuffer_eraserect(rb, &rect);

    rect.top = height() - 1;
    tickit_renderbuffer_eraserect(rb, &rect);
  }
  auto pad_rect = TickitRect{.top = 1, .left = 0, .lines = effective_height(), .cols = width()};
  tickit_renderbuffer_save(rb);
  tickit_renderbuffer_clip(rb, &pad_rect);
  tickit_renderbuffer_translate(rb, 1, 0);

  cursor_x = 0;
  cursor_y = 0;
  cursor_shape = (TickitCursorShape)0;
  if (!elements_.size()) {
    cursor_x = 0;
    cursor_y = 0;
    auto rect = TickitRect{.top = 0, .left = 0, .lines = effective_height(), .cols = width()};
    tickit_renderbuffer_eraserect(rb, &rect);
    return;
  }

  auto offset = offset_from_window_top_ - offset_in_cur_element_;
  auto it = elements_.find(cur_element_->element.get());
  CHECK(it != elements_.end());

  while (offset > 0) {
    if (it == elements_.begin()) {
      break;
    }
    it--;
    offset -= it->second->height;
  }

  if (offset > 0) {
    auto rect = TickitRect{.top = 0, .left = 0, .lines = offset, .cols = width()};
    tickit_renderbuffer_eraserect(rb, &rect);

    request_top_elements();
  }

  while (offset < effective_height() && it != elements_.end()) {
    auto rect = TickitRect{.top = offset, .left = 0, .lines = it->second->height, .cols = width()};
    tickit_renderbuffer_save(rb);
    tickit_renderbuffer_clip(rb, &rect);
    tickit_renderbuffer_translate(rb, offset, 0);
    auto x = it->second->element->render(*this, rb, it->second.get() == cur_element_);
    tickit_renderbuffer_restore(rb);

    offset += x;

    if (x != it->second->height) {
      if (it->second->element->is_less(*cur_element_->element)) {
        lines_before_cur_element_ += x - it->second->height;
      } else if (cur_element_->element->is_less(*it->second->element)) {
        lines_after_cur_element_ += x - it->second->height;
      } else {
        if (offset_in_cur_element_ >= x) {
          offset_in_cur_element_ = x > 0 ? x - 1 : 0;
        }
      }
      it->second->height = x;

      set_need_refresh();
    }

    it++;
  }

  if (offset < effective_height()) {
    auto rect = TickitRect{.top = offset, .left = 0, .lines = effective_height() - offset, .cols = width()};
    tickit_renderbuffer_eraserect(rb, &rect);
    request_bottom_elements();
  }

  if (need_refresh()) {
    adjust_cur_element(0);
  }

  tickit_renderbuffer_restore(rb);
  cursor_x++;

  td::int32 lines_over_window = lines_before_cur_element_ + offset_in_cur_element_ - offset_from_window_top_;
  td::int32 lines_under_window = lines_after_cur_element_ + (cur_element_->height - offset_in_cur_element_) -
                                 (effective_height() - offset_from_window_top_);

  if (lines_over_window < 0) {
    lines_over_window = 0;
  }

  if (lines_under_window < 0) {
    lines_under_window = 0;
  }

  {
    td::int32 t_x, t_y;
    TickitCursorShape t_cursor;
    TickitRect rect;
    td::CSlice text;

    td::StringBuilder sb;
    sb << "↑";
    if (glued_to_ == GluedTo::Top) {
      sb << "glued";
    } else {
      sb << lines_over_window;
    }
    sb << " ";
    sb << title() << "\n";
    rect = TickitRect{.top = 0, .left = 0, .lines = 1, .cols = width()};
    tickit_renderbuffer_save(rb);
    tickit_renderbuffer_clip(rb, &rect);
    text = sb.as_cslice();
    TextEdit::render(rb, t_x, t_y, t_cursor, width(), text, 0, {MarkupElement::fg_color(0, text.size(), 8)},
                     is_active(), false);
    tickit_renderbuffer_restore(rb);

    sb.clear();

    sb << "↓";
    if (glued_to_ == GluedTo::Bottom) {
      sb << "glued";
    } else {
      sb << lines_under_window;
    }
    sb << " ";
    sb << title() << "\n";
    rect = TickitRect{.top = height() - 1, .left = 0, .lines = 1, .cols = width()};
    tickit_renderbuffer_save(rb);
    tickit_renderbuffer_clip(rb, &rect);
    tickit_renderbuffer_translate(rb, height() - 1, 0);
    text = sb.as_cslice();
    TextEdit::render(rb, t_x, t_y, t_cursor, width(), text, 0, {MarkupElement::fg_color(0, text.size(), 8)},
                     is_active(), false);
    tickit_renderbuffer_restore(rb);
  }
}

td::int32 PadWindowElement::render_plain_text(TickitRenderBuffer *rb, td::Slice text, td::int32 width,
                                              td::int32 max_height, bool is_selected) {
  td::int32 cursor_x, cursor_y;
  TickitCursorShape cursor_shape;
  auto h = TextEdit::render(rb, cursor_x, cursor_y, cursor_shape, width, text, 0, std::vector<MarkupElement>(),
                            is_selected, false);
  if (h > max_height) {
    h = max_height;
  }
  return h;
}

td::int32 PadWindowElement::render_plain_text(TickitRenderBuffer *rb, td::Slice text, std::vector<MarkupElement> markup,
                                              td::int32 width, td::int32 max_height, bool is_selected) {
  td::int32 cursor_x, cursor_y;
  TickitCursorShape cursor_shape;
  auto h = TextEdit::render(rb, cursor_x, cursor_y, cursor_shape, width, text, 0, markup, is_selected, false);
  if (h > max_height) {
    h = max_height;
  }
  return h;
}

}  // namespace windows
