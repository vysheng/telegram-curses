#pragma once

#include "Markup.hpp"
#include "Output.hpp"

#include "td/utils/int_types.h"
#include "td/utils/Slice.h"

#include <string>
#include <vector>

namespace windows {

class TextEdit {
 public:
  TextEdit() {
  }
  TextEdit(std::string text) : text_(std::move(text)) {
    pos_ = text_.size();
  }
  void replace_text(std::string text) {
    text_ = text;
    pos_ = text_.size();
  }
  void clear() {
    replace_text("");
  }
  bool move_cursor_left(bool allow_change_line);
  bool move_cursor_right(bool allow_change_line);
  bool move_cursor_left(td::int32 cnt, bool allow_change_line);
  bool move_cursor_right(td::int32 cnt, bool allow_change_line);
  void move_cursor_down();
  void move_cursor_up();

  td::int32 go_to_beginning_of_line();
  td::int32 go_to_end_of_line();

  void insert_char(const char *ch);

  std::string export_data();
  void remove_prev_char();
  void remove_next_char();

  static td::int32 render(WindowOutputter &rb, td::int32 width, td::Slice text, size_t pos,
                          const std::vector<MarkupElement> &markup, bool is_selected, bool is_password,
                          td::int32 pad_width = 0, std::string pad_char = " ");
  td::int32 render(WindowOutputter &rb, td::int32 width, bool is_selected, bool is_password, td::int32 pad_width = 0,
                   std::string pad_char = " ");

  bool is_empty() const {
    return text_.size() == 0;
  }

 private:
  std::string text_;
  size_t pos_{0};
};

}  // namespace windows
