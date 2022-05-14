#pragma once

#include "td/utils/Slice.h"
#include <vector>

struct Graphem {
  td::Slice data;
  td::int32 width{0};
  td::int32 unicode_codepoints{0};
};

struct UnicodeWidthBlock {
  UnicodeWidthBlock() = default;
  UnicodeWidthBlock(td::int32 begin, td::int32 end, td::int32 width) : begin(begin), end(end), width(width) {
  }
  td::int32 begin, end, width;
};

Graphem next_graphem(td::Slice data, size_t pos);
Graphem prev_graphem(td::Slice data, size_t pos);
void set_tickit_wrap();
void enable_wide_emojis();
void override_unicode_width(std::vector<UnicodeWidthBlock> blocks);
Graphem next_graphems(td::Slice data, size_t pos = 0, size_t limit_bytes = (size_t)-1, td::int32 limit_width = -1,
                      td::int32 limit_graphems = -1, td::int32 limit_codepoints = -1);
