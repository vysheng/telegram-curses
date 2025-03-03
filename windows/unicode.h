#pragma once

#include "td/utils/Slice.h"
#include <vector>

struct Graphem {
  td::Slice data;
  td::int32 width{0};
  td::int32 unicode_codepoints{0};
  td::int32 first_codepoint{0};

  bool is_whitespace() const;
  bool is_whitespace_or_linebreak() const;
  bool is_alpha() const;
  bool is_num() const;
  bool is_alphanum() const;
};

struct UnicodeWidthBlock {
  UnicodeWidthBlock() = default;
  UnicodeWidthBlock(td::int32 begin, td::int32 end, td::int32 width) : begin(begin), end(end), width(width) {
  }
  td::int32 begin, end, width;
};

struct UnicodeCounter {
  size_t bytes;
  size_t codepoints;
  size_t graphemes;
  size_t columns;
};

#define LEFT_ALIGN_BLOCK_START 0xF0000
#define LEFT_ALIGN_BLOCK_END 0xF00FF
#define RIGHT_ALIGN_BLOCK_START 0xF0100
#define RIGHT_ALIGN_BLOCK_END 0xF01FF
#define SOFT_LINE_BREAK_CP 0xF0200

#define SOFT_LINE_BREAK_STR "\xF3\xB0\x88\x80"

Graphem next_graphem(td::Slice data, size_t pos);
Graphem prev_graphem(td::Slice data, size_t pos);
void enable_wide_emojis();
void override_unicode_width(std::vector<UnicodeWidthBlock> blocks);
Graphem next_graphems(td::Slice data, size_t pos = 0, size_t limit_bytes = (size_t)-1, td::int32 limit_width = -1,
                      td::int32 limit_graphems = -1, td::int32 limit_codepoints = -1);
td::Slice get_utf8_string_substring(td::Slice text, size_t from, size_t to);
td::Slice get_utf8_string_substring_utf16_codepoints(td::Slice text, size_t from, size_t to);
td::uint32 utf8_code_to_str(td::int32 code, char buf[6]);
td::int32 utf8_string_width(td::Slice data);
size_t utf8_count(td::Slice S, UnicodeCounter &res, const UnicodeCounter *limit, bool advance_on_error);
