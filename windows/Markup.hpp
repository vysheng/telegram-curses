#pragma once

#include "td/utils/common.h"
#include "Output.hpp"

namespace windows {
struct MarkupElement {
  struct Attr {
    enum Values : td::int32 {
      Min,
      FgColor,
      BgColor,
      Bold,
      Underline,
      Italic,
      Reverse,
      Strike,
      Blink,
      NoLB,
      Photo,
      FgColorRGB,
      Max
    };
  };
  MarkupElement() = default;
  MarkupElement(size_t first_pos, size_t last_pos, td::int32 attr, td::int32 arg)
      : first_pos(first_pos), last_pos(last_pos), attr(attr), arg(arg) {
  }
  static MarkupElement fg_color(size_t first_pos, size_t last_pos, Color fg_color);
  static MarkupElement bg_color(size_t first_pos, size_t last_pos, Color bg_color);
  static MarkupElement bold(size_t first_pos, size_t last_pos);
  static MarkupElement underline(size_t first_pos, size_t last_pos, td::int32 line_type = 1);
  static MarkupElement italic(size_t first_pos, size_t last_pos);
  static MarkupElement reverse(size_t first_pos, size_t last_pos);
  static MarkupElement strike(size_t first_pos, size_t last_pos);
  static MarkupElement blink(size_t first_pos, size_t last_pos);
  static MarkupElement nolb(size_t first_pos, size_t last_pos);
  static MarkupElement photo(size_t first_pos, size_t last_pos, td::int32 height, td::int32 width, std::string path);
  static MarkupElement fg_color_rgb(size_t first_pos, size_t last_pos, td::uint32 fg_color_rgb);
  size_t first_pos;
  size_t last_pos;
  td::int32 attr;
  td::int32 arg;
  td::int32 arg2;
  td::int32 arg3;
  std::string str_arg;

  void install(WindowOutputter &rb) const;
  void uninstall(WindowOutputter &rb) const;
};

}  // namespace windows
