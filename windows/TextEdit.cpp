#include "TextEdit.hpp"
#include "td/utils/Status.h"
#include "td/utils/utf8.h"
#include "unicode.h"
#include <vector>
#include <algorithm>
#include <functional>
#include <map>

namespace windows {

bool is_printable_control_char(td::Slice data) {
  return data.size() == 1 && (data[0] == '\n' || data[0] == '\t');
}

MarkupElement MarkupElement::fg_color(size_t first_pos, size_t last_pos, Color fg_color) {
  return MarkupElement(first_pos, last_pos, Attr::FgColor, (td::int32)fg_color);
}
MarkupElement MarkupElement::bg_color(size_t first_pos, size_t last_pos, Color bg_color) {
  return MarkupElement(first_pos, last_pos, Attr::BgColor, (td::int32)bg_color);
}
MarkupElement MarkupElement::bold(size_t first_pos, size_t last_pos) {
  return MarkupElement(first_pos, last_pos, Attr::Bold, 1);
}
MarkupElement MarkupElement::underline(size_t first_pos, size_t last_pos, td::int32 line_type) {
  return MarkupElement(first_pos, last_pos, Attr::Underline, line_type);
}
MarkupElement MarkupElement::italic(size_t first_pos, size_t last_pos) {
  return MarkupElement(first_pos, last_pos, Attr::Italic, 1);
}
MarkupElement MarkupElement::reverse(size_t first_pos, size_t last_pos) {
  return MarkupElement(first_pos, last_pos, Attr::Reverse, 1);
}
MarkupElement MarkupElement::strike(size_t first_pos, size_t last_pos) {
  return MarkupElement(first_pos, last_pos, Attr::Strike, 1);
}
MarkupElement MarkupElement::blink(size_t first_pos, size_t last_pos) {
  return MarkupElement(first_pos, last_pos, Attr::Blink, 1);
}
MarkupElement MarkupElement::nolb(size_t first_pos, size_t last_pos) {
  return MarkupElement(first_pos, last_pos, Attr::NoLB, 1);
}

void MarkupElement::install(WindowOutputter &rb) const {
  switch (attr) {
    case Attr::FgColor:
      rb.set_fg_color((Color)arg);
      break;
    case Attr::BgColor:
      rb.set_bg_color((Color)arg);
      break;
    case Attr::Bold:
      rb.set_bold(arg);
      break;
    case Attr::Italic:
      rb.set_italic(arg);
      break;
    case Attr::Reverse:
      rb.set_reverse(arg);
      break;
    case Attr::Strike:
      rb.set_strike(arg);
      break;
    case Attr::Blink:
      rb.set_blink(arg);
      break;
    case Attr::Underline:
      rb.set_underline(arg);
      break;
    default:
      break;
  }
}

void MarkupElement::uninstall(WindowOutputter &rb) const {
  switch (attr) {
    case Attr::FgColor:
      rb.unset_fg_color();
      break;
    case Attr::BgColor:
      rb.unset_bg_color();
      break;
    case Attr::Bold:
      rb.unset_bold();
      break;
    case Attr::Italic:
      rb.unset_italic();
      break;
    case Attr::Reverse:
      rb.unset_reverse();
      break;
    case Attr::Strike:
      rb.unset_strike();
      break;
    case Attr::Blink:
      rb.unset_blink();
      break;
    case Attr::Underline:
      rb.unset_underline();
      break;
    default:
      break;
  }
}

bool TextEdit::move_cursor_right(bool allow_change_line) {
  if (pos_ == text_.size()) {
    return false;
  }

  while (true) {
    if (text_[pos_] == '\n' && !allow_change_line) {
      return false;
    }
    auto x = next_graphem(td::Slice(text_), pos_);
    if (x.width == -2) {
      return false;
    }
    pos_ += x.data.size();

    if (x.width < 0) {
      if (is_printable_control_char(x.data)) {
        return true;
      }
    } else {
      return true;
    }
  }

  return true;
}

bool TextEdit::move_cursor_left(bool allow_change_line) {
  if (pos_ == 0) {
    return false;
  }

  while (pos_ > 0) {
    auto x = prev_graphem(td::Slice(text_), pos_);
    if (x.width == -2) {
      return false;
    }
    if (x.data.size() == 1 && x.data[0] == '\n' && !allow_change_line) {
      return false;
    }
    CHECK(x.data.size() <= pos_);
    pos_ -= x.data.size();

    if (x.width < 0) {
      if (is_printable_control_char(x.data)) {
        return true;
      }
    } else {
      return true;
    }
  }
  return true;
}

td::int32 TextEdit::go_to_beginning_of_line() {
  td::int32 pos = 0;
  while (move_cursor_left(false)) {
    pos++;
  }
  return pos;
}

td::int32 TextEdit::go_to_end_of_line() {
  td::int32 pos = 0;
  while (move_cursor_right(false)) {
    pos++;
  }
  return pos;
}

bool TextEdit::move_cursor_left(td::int32 cnt, bool allow_change_line) {
  for (td::int32 i = 0; i < cnt; i++) {
    if (!move_cursor_left(allow_change_line)) {
      return false;
    }
  }
  return true;
}

bool TextEdit::move_cursor_right(td::int32 cnt, bool allow_change_line) {
  for (td::int32 i = 0; i < cnt; i++) {
    if (!move_cursor_right(allow_change_line)) {
      return false;
    }
  }
  return true;
}

void TextEdit::move_cursor_down() {
  auto c = go_to_beginning_of_line();
  go_to_end_of_line();
  move_cursor_right(true);
  go_to_beginning_of_line();
  move_cursor_right(c, false);
}

void TextEdit::move_cursor_up() {
  auto c = go_to_beginning_of_line();
  move_cursor_left(true);
  go_to_beginning_of_line();
  move_cursor_right(c, false);
}

void TextEdit::insert_char(const char *ch) {
  text_.insert(pos_, ch);
  pos_ += strlen(ch);
}

void TextEdit::remove_next_char() {
  auto old_pos = pos_;
  if (!move_cursor_right(true)) {
    return;
  }
  text_.erase(old_pos, pos_ - old_pos);
  pos_ = old_pos;
}

void TextEdit::remove_prev_char() {
  auto old_pos = pos_;
  if (!move_cursor_left(true)) {
    return;
  }
  text_.erase(pos_, old_pos - pos_);
}
std::string TextEdit::export_data() {
  return text_;
}

td::int32 TextEdit::render(WindowOutputter &rb, td::int32 width, bool is_selected, bool is_password,
                           td::int32 pad_width, std::string pad_char) {
  return render(rb, width, text_, pos_, std::vector<MarkupElement>(), is_selected, is_password, pad_width, pad_char);
}

namespace {

class Builder {
 public:
  Builder(WindowOutputter &rb, td::int32 width, bool is_password) : rb_(rb), width_(width), is_password_(is_password) {
  }
  ~Builder() {
  }
  void start_new_line() {
    rb_.erase_yx(cur_line_, cur_line_pos_, width_ - cur_line_pos_);
    for (int i = 0; i < pad_width_; i++) {
      rb_.putstr_yx(cur_line_, width_ + i, pad_char_.c_str(), pad_char_.size());
    }
    cur_line_++;
    cur_line_pos_ = 0;
  }
  void add_utf8(td::Slice data, td::int32 width, bool has_cursor) {
    if (cur_line_pos_ + (is_password_ ? 1 : width) > width_) {
      if (nolb_) {
        return;
      }
      start_new_line();
    }

    if (!is_password_) {
      rb_.putstr_yx(cur_line_, cur_line_pos_, data.data(), data.size());
      cur_line_pos_ += width;
    } else {
      rb_.putstr_yx(cur_line_, cur_line_pos_, "*", 1);
      cur_line_pos_++;
    }

    if (has_cursor) {
      cursor_y_ = cur_line_;
      cursor_x_ = cur_line_pos_ - 1;
    }
  }

  void pad_left(td::int32 size, bool has_cursor) {
    if (size < 0) {
      return;
    }
    if (size >= width_) {
      return;
    }
    if (cur_line_pos_ <= size) {
      while (cur_line_pos_ < size) {
        add_utf8(" ", 1, has_cursor);
      }
      return;
    } else {
      start_new_line();
      CHECK(!cur_line_pos_);
      while (cur_line_pos_ < size) {
        add_utf8(" ", 1, has_cursor);
      }
      return;
    }
  }

  void pad_right(td::int32 size, bool has_cursor) {
    if (size <= 0) {
      return;
    }
    if (size > width_) {
      return;
    }
    pad_left(width_ - size, has_cursor);
  }

  void add_control(td::int32 ch, bool has_cursor) {
    if (is_password_) {
      return add_utf8("*", 1, has_cursor);
    }
    switch (ch) {
      case '\n': {
        if (nolb_) {
          add_utf8(" ", 1, has_cursor);
          break;
        }
        if (has_cursor) {
          if (cur_line_pos_ == width_) {
            start_new_line();
          }
          cursor_y_ = cur_line_;
          cursor_x_ = cur_line_pos_;
        }
        start_new_line();
      } break;
      case '\t':
        add_utf8(" ", 1, has_cursor);
        break;
      default: {
        if (has_cursor) {
          if (cur_line_pos_ == width_ && !nolb_) {
            start_new_line();
          }
          cursor_y_ = cur_line_;
          cursor_x_ = cur_line_pos_;
        }
      }
    }
  }

  void complete(bool has_cursor) {
    if (!is_password_) {
      add_control('\n', has_cursor);
    } else {
      if (has_cursor) {
        if (cur_line_pos_ == width_) {
          start_new_line();
        }
        cursor_y_ = cur_line_;
        cursor_x_ = cur_line_pos_;
      }
      start_new_line();
    }
  }

  void add_markup(const MarkupElement &me) {
    if (me.attr == MarkupElement::Attr::NoLB) {
      nolb_++;
    } else {
      me.install(rb_);
    }
  }

  void del_markup(const MarkupElement &me) {
    if (me.attr == MarkupElement::Attr::NoLB) {
      nolb_--;
    } else {
      me.uninstall(rb_);
    }
  }

  td::int32 height() const {
    return cur_line_ + 1;
  }
  td::int32 cursor_x() const {
    return cursor_x_;
  }
  td::int32 cursor_y() const {
    return cursor_y_;
  }

  void set_pad(td::int32 pad_width, std::string pad_char) {
    pad_width_ = pad_width;
    pad_char_ = pad_char;
  }

 private:
  WindowOutputter &rb_;
  td::int32 width_;
  td::int32 pad_width_{0};
  std::string pad_char_;
  td::int32 cur_line_{0};
  td::int32 cur_line_pos_{0};
  td::int32 cursor_x_{-1};
  td::int32 cursor_y_{-1};
  bool is_password_{false};
  td::int32 nolb_{0};
};

}  // namespace

td::int32 TextEdit::render(WindowOutputter &rb, td::int32 width, td::Slice text, size_t pos,
                           const std::vector<MarkupElement> &input_markup, bool is_selected, bool is_password,
                           td::int32 pad_width, std::string pad_char) {
  struct Action {
    Action(size_t pos, bool enable, const MarkupElement *el) : pos(pos), enable(enable), el(el) {
    }
    size_t pos;
    bool enable;
    const MarkupElement *el;
    bool operator<(const Action &other) const {
      return pos < other.pos || (pos == other.pos && !enable && other.enable);
    }
  };
  std::vector<Action> actions;
  for (auto &m : input_markup) {
    actions.emplace_back(m.first_pos, true, &m);
    actions.emplace_back(m.last_pos, false, &m);
  }
  auto reverse_markup = MarkupElement::reverse(0, text.size() + 1);
  if (is_selected) {
    actions.emplace_back(0, true, &reverse_markup);
    actions.emplace_back(text.size() + 1, false, &reverse_markup);
  }

  std::sort(actions.begin(), actions.end());
  size_t actions_pos = 0;

  Builder builder(rb, width, is_password);
  builder.set_pad(pad_width, pad_char);

  size_t cur_pos = 0;
  while (cur_pos <= text.size()) {
    while (actions_pos < actions.size() && actions[actions_pos].pos <= cur_pos) {
      if (actions[actions_pos].enable) {
        builder.add_markup(*actions[actions_pos++].el);
      } else {
        builder.del_markup(*actions[actions_pos++].el);
      }
    }
    if (cur_pos == text.size()) {
      break;
    }
    auto x = next_graphem(text, cur_pos);
    if (x.first_codepoint >= LEFT_ALIGN_BLOCK_START && x.first_codepoint <= LEFT_ALIGN_BLOCK_END) {
      builder.pad_left(x.first_codepoint - LEFT_ALIGN_BLOCK_START, cur_pos == pos);
    } else if (x.first_codepoint >= RIGHT_ALIGN_BLOCK_START && x.first_codepoint <= RIGHT_ALIGN_BLOCK_END) {
      builder.pad_right(x.first_codepoint - RIGHT_ALIGN_BLOCK_START, cur_pos == pos);
    } else if (x.width >= 0) {
      builder.add_utf8(x.data, x.width, cur_pos == pos);
    } else if (x.width == -1) {
      builder.add_control(x.data.size() > 1 ? 0 : x.data[0], cur_pos == pos);
    } else {
      break;
    }
    cur_pos += x.data.size();
  }
  builder.complete(pos == text.size());
  while (actions_pos < actions.size()) {
    if (actions[actions_pos].enable) {
      builder.add_markup(*actions[actions_pos++].el);
    } else {
      builder.del_markup(*actions[actions_pos++].el);
    }
  }
  return builder.height() - 1;
}

}  // namespace windows
