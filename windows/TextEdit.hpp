#pragma once

#include "Markup.hpp"
#include "Output.hpp"

#include "td/utils/int_types.h"
#include "td/utils/Slice.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace windows {

struct SavedRenderedImagesDirectory {
  SavedRenderedImagesDirectory(SavedRenderedImages l) {
    for (auto &x : l) {
      old_images[x.first] = std::move(x.second);
    }
    l.clear();
  }
  SavedRenderedImages release() {
    SavedRenderedImages r;
    for (auto &x : new_images) {
      r.emplace_back(x.first, std::move(x.second));
    }
    old_images.clear();
    new_images.clear();
    return r;
  }
  std::map<std::string, std::vector<std::unique_ptr<RenderedImage>>> old_images;
  std::map<std::string, std::vector<std::unique_ptr<RenderedImage>>> new_images;
};

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
  bool move_cursor_next_word(bool allow_change_line);
  bool move_cursor_prev_word(bool allow_change_line);

  td::int32 go_to_beginning_of_line();
  td::int32 go_to_end_of_line();

  void insert_char(const char *ch);

  std::string export_data();
  void remove_prev_char();
  void remove_next_char();
  void clear_before_cursor(bool allow_change_line);
  void clear_after_cursor(bool allow_change_line);
  void clear_word_before_cursor(bool allow_change_line);
  void clear_word_after_cursor(bool allow_change_line);

  static td::int32 render(WindowOutputter &rb, td::int32 width, td::Slice text, size_t pos,
                          const std::vector<MarkupElement> &markup, bool is_selected, bool is_password,
                          SavedRenderedImagesDirectory *rendered_images = nullptr, td::int32 pad_width = 0,
                          std::string pad_char = " ");
  td::int32 render(WindowOutputter &rb, td::int32 width, bool is_selected, bool is_password,
                   SavedRenderedImagesDirectory *rendered_images = nullptr, td::int32 pad_width = 0,
                   std::string pad_char = " ");

  bool is_empty() const {
    return text_.size() == 0;
  }

 private:
  std::string text_;
  size_t pos_{0};
};

}  // namespace windows
