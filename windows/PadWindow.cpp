#include "PadWindow.hpp"
#include "td/utils/Slice-decl.h"
#include "td/utils/StringBuilder.h"
#include "td/utils/ScopeGuard.h"
#include <memory>
#include <vector>

namespace windows {

PadWindow::PadWindow() {
  pad_window_body_ = std::make_shared<PadWindowBody>(this);
  add_subwindow(pad_window_body_, 1, 0);
}

static PadWindow::GluedTo adjust_glued_to_up(PadWindow::GluedTo from) {
  switch (from) {
    case PadWindow::GluedTo::Top:
      return from;
    case PadWindow::GluedTo::Bottom:
      return PadWindow::GluedTo::RelBottom;
    case PadWindow::GluedTo::RelTop:
    case PadWindow::GluedTo::RelBottom:
    case PadWindow::GluedTo::None:
    default:
      return from;
  }
}

static PadWindow::GluedTo adjust_glued_to_down(PadWindow::GluedTo from) {
  switch (from) {
    case PadWindow::GluedTo::Top:
      return PadWindow::GluedTo::RelTop;
    case PadWindow::GluedTo::Bottom:
      return from;
    case PadWindow::GluedTo::RelTop:
    case PadWindow::GluedTo::RelBottom:
    case PadWindow::GluedTo::None:
    default:
      return from;
  }
}

void PadWindow::scroll_up(td::int32 lines) {
  glued_to_ = adjust_glued_to_up(glued_to_);
  adjust_cur_element(-lines);
}

void PadWindow::scroll_prev_element() {
  if (!cur_element_) {
    return;
  }
  glued_to_ = adjust_glued_to_up(glued_to_);
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
  glued_to_ = adjust_glued_to_down(glued_to_);
  auto it = elements_.find(cur_element_->element.get());
  it++;
  if (it == elements_.end()) {
    adjust_cur_element(cur_element_->height - offset_in_cur_element_);
    return;
  }
  auto h = std::min(it->second->height, effective_height());
  adjust_cur_element(cur_element_->height - offset_in_cur_element_ + h - 1);
  adjust_cur_element(-(h - 1));
  CHECK(cur_element_ == it->second.get());
}

void PadWindow::scroll_down(td::int32 lines) {
  glued_to_ = adjust_glued_to_down(glued_to_);
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

void PadWindow::handle_input(const InputEvent &info) {
  set_need_refresh();
  pad_window_body_->set_need_refresh();
  if (info == "T-PageUp") {
    scroll_up(effective_height() / 2);
  } else if (info == "T-Up") {
    scroll_up(1);
  } else if (info == "T-PageDown") {
    scroll_down(effective_height() / 2);
  } else if (info == "T-Down") {
    scroll_down(1);
  } else if (info == "C-u") {
    scroll_up(effective_height() / 2);
  } else if (info == "C-d") {
    scroll_down(effective_height() / 2);
  } else if (info == "C-b") {
    scroll_up(effective_height() - 1);
  } else if (info == "C-f") {
    scroll_down(effective_height() - 1);
  } else if (info == "k") {
    scroll_up(1);
  } else if (info == "j") {
    scroll_down(1);
  } else if (info == "g") {
    scroll_first_line();
  } else if (info == "G") {
    scroll_last_line();
  } else if (info == "n") {
    scroll_next_element();
  } else if (info == "N") {
    scroll_prev_element();
  } else {
    active_element_handle_input(info);
  }
}

void PadWindow::active_element_handle_input(const InputEvent &info) {
  auto el = get_active_element();
  if (el) {
    el->handle_input(*this, info);
  }
}

void PadWindow::on_resize(td::int32 old_height, td::int32 old_width, td::int32 new_height, td::int32 new_width) {
  pad_window_body_->resize(new_height - 2, new_width);

  if (elements_.size() == 0) {
    return;
  }

  for (auto &p : elements_) {
    auto &el = *p.second;
    auto old_height = el.height;
    el.element->change_width(new_width);
    el.height = el.element->render_fake(*this, empty_window_outputter(), cur_element_ == nullptr);
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
  pad_window_body_->set_need_refresh();
  auto it = elements_.find(elem);
  if (it == elements_.end()) {
    return;
  }

  auto &el = *it->second;

  auto old_height = it->second->height;

  el.height = el.element->render_fake(*this, empty_window_outputter(), it->second.get() == cur_element_);

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
  pad_window_body_->set_need_refresh();
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
      el.height = el.element->render_fake(*this, empty_window_outputter(), it->second.get() == cur_element_);
      if (cur_element_->element->is_less(*elem)) {
        lines_after_cur_element_ += it->second->height;
      } else {
        lines_before_cur_element_ += it->second->height;
      }
    }
  } else if (cur_element_->element->is_less(*elem)) {
    lines_after_cur_element_ -= it->second->height;
    CHECK(lines_after_cur_element_ >= 0);
    auto ptr = std::move(it->second);
    elements_.erase(it);
    change();
    if (elem->is_visible()) {
      it = elements_.emplace(elem.get(), std::move(ptr)).first;
      auto &el = *it->second;
      el.height = el.element->render_fake(*this, empty_window_outputter(), it->second.get() == cur_element_);
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
    auto next_el = it;
    next_el++;
    elements_.erase(it);
    change();
    if (!elem->is_visible()) {
      if (elements_.size() != 0) {
        if (next_el != elements_.end()) {
          lines_after_cur_element_ -= next_el->second->height;
          CHECK(lines_after_cur_element_ >= 0);
          offset_in_cur_element_ = 0;
          cur_element_ = next_el->second.get();
        } else {
          CHECK(!lines_after_cur_element_);
          next_el--;
          lines_before_cur_element_ -= next_el->second->height;
          CHECK(lines_before_cur_element_ >= 0);
          offset_in_cur_element_ = 0;
          cur_element_ = next_el->second.get();
        }
      } else {
        clear();
      }
      adjust_cur_element(0);
      return;
    }
    it = elements_.emplace(elem.get(), std::move(ptr)).first;
    auto &el = *it->second;
    bool unchanged_pos = false;
    if (next_el != elements_.begin() && el.element->is_visible()) {
      auto prev_el = next_el;
      prev_el--;
      if (prev_el == it) {
        unchanged_pos = true;
      }
    }
    el.height = el.element->render_fake(*this, empty_window_outputter(), it->second.get() == cur_element_);
    auto new_height = it->second->height;
    offset_in_cur_element_ = (td::int32)(p * new_height);

    if (!unchanged_pos) {
      lines_before_cur_element_ = 0;
      for (auto it3 = elements_.begin(); it3 != it; it3++) {
        lines_before_cur_element_ += it3->second->height;
      }
      lines_after_cur_element_ = tot_height - lines_before_cur_element_;
    }
  }

  CHECK(lines_before_cur_element_ >= 0);
  adjust_cur_element(0);
}

void PadWindow::delete_element(PadWindowElement *elem) {
  set_need_refresh();
  pad_window_body_->set_need_refresh();
  auto it = elements_.find(elem);
  if (it == elements_.end()) {
    return;
  }

  CHECK(cur_element_);

  auto &el = *it->second;

  auto old_height = el.height;
  auto old_it = it;

  CHECK(lines_after_cur_element_ >= 0);

  if (cur_element_->element->is_less(*elem)) {
    lines_after_cur_element_ -= old_height;
    CHECK(lines_after_cur_element_ >= 0);
  } else if (elem->is_less(*cur_element_->element)) {
    lines_before_cur_element_ -= old_height;
    CHECK(lines_before_cur_element_ >= 0);
  } else {
    CHECK(cur_element_->element.get() == elem);
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
        CHECK(lines_after_cur_element_ >= 0);
      } else {
        cur_element_ = nullptr;
        offset_in_cur_element_ = 0;
        offset_from_window_top_ = 0;
        LOG_CHECK(!lines_after_cur_element_) << "after=" << lines_after_cur_element_;
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
  pad_window_body_->set_need_refresh();
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

  el.height = el.element->render_fake(*this, empty_window_outputter(), cur_element_ == nullptr);
  LOG_CHECK(el.height >= 0 && el.height <= max_item_height()) << el.height;

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
        CHECK(lines_after_cur_element_ >= 0);
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

  if (glued_to_ == GluedTo::RelTop) {
    if (lines_before_cur_element_ + cur_element_->height > effective_height()) {
      glued_to_ = GluedTo::None;
    } else {
      offset_from_window_top_ = lines_before_cur_element_ + offset_in_cur_element_;
    }
  }
  if (glued_to_ == GluedTo::RelBottom) {
    if (lines_after_cur_element_ + cur_element_->height > effective_height()) {
      glued_to_ = GluedTo::None;
    } else {
      offset_from_window_top_ =
          effective_height() - lines_after_cur_element_ - (cur_element_->height - offset_in_cur_element_);
    }
  }

  if (pad_to_ == PadTo::Top) {
    CHECK(lines_before_cur_element_ >= 0);
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

void PadWindow::scroll_to_element(PadWindowElement *elptr, ScrollMode scroll_mode) {
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

  if (lines_before_cur_element_ + cur_element_->height <= effective_height()) {
    glued_to_ = GluedTo::RelTop;
  } else if (lines_after_cur_element_ + cur_element_->height <= effective_height()) {
    glued_to_ = GluedTo::RelBottom;
  } else {
    glued_to_ = GluedTo::None;
  }

  adjust_cur_element(0);
  set_need_refresh();
  pad_window_body_->set_need_refresh();

  if (lines_before_cur_element_ < 100) {
    request_top_elements();
  }
  if (lines_after_cur_element_ < 100) {
    request_bottom_elements();
  }
}

std::vector<std::shared_ptr<PadWindowElement>> PadWindow::get_visible_elements() {
  if (!cur_element_) {
    return {};
  }

  std::vector<std::shared_ptr<PadWindowElement>> res;

  auto l = lines_before_cur_element_;
  auto it = elements_.find(cur_element_->element.get());
  CHECK(it != elements_.end());
  auto itf = it;
  while (it != elements_.begin() && l > 0) {
    it--;
    l -= it->second->height;
  }

  while (it != itf) {
    res.push_back(it->second->element);
    it++;
  }

  res.push_back(it->second->element);
  it++;

  l = lines_after_cur_element_;

  while (it != elements_.end() && l > 0) {
    res.push_back(it->second->element);
    l -= it->second->height;
    it++;
  }

  return res;
}

void PadWindow::render(WindowOutputter &rb, bool force) {
  {
    rb.erase_rect(0, 0, 1, width());
    rb.erase_rect(height() - 1, 0, 1, width());
  }

  {
    td::int32 lines_over_window =
        (cur_element_ ? lines_before_cur_element_ + offset_in_cur_element_ - offset_from_window_top_ : 0);
    td::int32 lines_under_window =
        (cur_element_ ? lines_after_cur_element_ + (cur_element_->height - offset_in_cur_element_) -
                            (effective_height() - offset_from_window_top_)
                      : 0);

    if (lines_over_window < 0) {
      lines_over_window = 0;
    }

    if (lines_under_window < 0) {
      lines_under_window = 0;
    }

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
    text = sb.as_cslice();
    TextEdit::render(
        rb, width(), text, 0,
        {std::make_shared<MarkupElementFgColor>(MarkupElementPos(0, 0), MarkupElementPos(text.size() + 1, 0),
                                                Color::Grey),
         std::make_shared<MarkupElementNoLb>(MarkupElementPos(0, 0), MarkupElementPos(text.size(), 0), true)},
        rb.is_active(), false);
    sb.clear();

    rb.translate(height() - 1, 0);
    sb << "↓";
    if (glued_to_ == GluedTo::Bottom) {
      sb << "glued";
    } else {
      sb << lines_under_window;
    }
    sb << " ";
    sb << title() << "\n";
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

void PadWindow::render_body(WindowOutputter &rb, bool force) {
  if (!elements_.size()) {
    rb.erase_rect(0, 0, effective_height(), width());
    return;
  }

  auto dir = SavedRenderedImagesDirectory(std::move(saved_images_));

  auto offset = offset_from_window_top_ - offset_in_cur_element_;
  auto it = elements_.find(cur_element_->element.get());
  CHECK(it != elements_.end());

  CHECK(offset >= -max_item_height() && offset <= max_item_height());

  while (offset > 0) {
    if (it == elements_.begin()) {
      break;
    }
    it--;
    LOG_CHECK(it->second->height >= 0 && it->second->height <= max_item_height()) << it->second->height;
    offset -= it->second->height;
  }

  if (offset > 0) {
    rb.erase_rect(0, 0, offset, width());
    request_top_elements();
  }

  while (offset < effective_height() && it != elements_.end()) {
    rb.translate(offset, 0);
    auto x = it->second->element->render(*this, rb, dir, it->second.get() == cur_element_);
    rb.untranslate(offset, 0);

    CHECK(x >= 0 && x <= max_item_height());
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
      pad_window_body_->set_need_refresh();
    }

    it++;
  }

  if (offset < effective_height()) {
    rb.erase_rect(offset, 0, effective_height() - offset, width());
    request_bottom_elements();
  }

  if (need_refresh()) {
    adjust_cur_element(0);
  }

  saved_images_ = dir.release();
}

td::int32 PadWindowElement::render_plain_text(WindowOutputter &rb, td::Slice text, td::int32 width,
                                              td::int32 max_height, bool is_selected,
                                              SavedRenderedImagesDirectory *images) {
  auto h = TextEdit::render(rb, width, text, 0, std::vector<MarkupElement>(), is_selected, false, images);
  if (h > max_height) {
    h = max_height;
  }
  return h;
}

td::int32 PadWindowElement::render_plain_text(WindowOutputter &rb, td::Slice text, std::vector<MarkupElement> markup,
                                              td::int32 width, td::int32 max_height, bool is_selected,
                                              SavedRenderedImagesDirectory *images) {
  auto h = TextEdit::render(rb, width, text, 0, markup, is_selected, false, images);
  if (h > max_height) {
    h = max_height;
  }
  return h;
}

}  // namespace windows
