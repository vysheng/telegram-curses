#pragma once

#include "td/utils/common.h"
#include <tickit.h>

namespace windows {
struct MarkupElement {
  struct Attr {
    using Tickit = TickitPenAttr;
    enum Values : td::int32 { Min = TickitPenAttr::TICKIT_N_PEN_ATTRS + 1, NoLB, Max };
  };
  MarkupElement() = default;
  MarkupElement(size_t first_pos, size_t last_pos, td::int32 attr, td::int32 arg)
      : first_pos(first_pos), last_pos(last_pos), attr(attr), arg(arg) {
  }
  static MarkupElement fg_color(size_t first_pos, size_t last_pos, td::int32 fg_color);
  static MarkupElement bg_color(size_t first_pos, size_t last_pos, td::int32 bg_color);
  static MarkupElement bold(size_t first_pos, size_t last_pos);
  static MarkupElement underline(size_t first_pos, size_t last_pos,
                                 TickitPenUnderline type = TickitPenUnderline::TICKIT_PEN_UNDER_SINGLE);
  static MarkupElement italic(size_t first_pos, size_t last_pos);
  static MarkupElement reverse(size_t first_pos, size_t last_pos);
  static MarkupElement strike(size_t first_pos, size_t last_pos);
  static MarkupElement altfont(size_t first_pos, size_t last_pos, td::int32 font_id);
  static MarkupElement blink(size_t first_pos, size_t last_pos);
  size_t first_pos;
  size_t last_pos;
  int attr;
  td::int32 arg;

  void install(TickitPen *pen) const;
  void uninstall(TickitPen *pen) const;
};

}  // namespace windows
