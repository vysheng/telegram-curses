#include "unicode.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/utf8.h"
#include <limits>
#include <utf8proc.h>
#include <algorithm>
#include "td/utils/int_types.h"
#include "td/utils/unicode.h"
#include "td/utils/logging.h"

namespace {
const unsigned char *next_utf8(const unsigned char *ptr, const unsigned char *end, td::uint32 &code) {
  if (end <= ptr) {
    code = 0;
    return ptr;
  }
  td::uint32 a = ptr[0];
  if ((a & 0x80) == 0) {
    code = a;
    return ptr + 1;
  } else if ((a & 0x20) == 0) {
    if (end - ptr < 2) {
      code = 0;
      return ptr;
    }
    code = ((a & 0x1f) << 6) | (ptr[1] & 0x3f);
    return ptr + 2;
  } else if ((a & 0x10) == 0) {
    if (end - ptr < 3) {
      code = 0;
      return ptr;
    }
    code = ((a & 0x0f) << 12) | ((ptr[1] & 0x3f) << 6) | (ptr[2] & 0x3f);
    return ptr + 3;
  } else if ((a & 0x08) == 0) {
    if (end - ptr < 4) {
      code = 0;
      return ptr;
    }
    code = ((a & 0x07) << 18) | ((ptr[1] & 0x3f) << 12) | ((ptr[2] & 0x3f) << 6) | (ptr[3] & 0x3f);
    return ptr + 4;
  } else if ((a & 0x04) == 0) {
    if (end - ptr < 5) {
      code = 0;
      return ptr;
    }
    code = ((a & 0x03) << 24) | ((ptr[1] & 0x3f) << 18) | ((ptr[2] & 0x3f) << 12) | ((ptr[3] & 0x3f) << 6) |
           (ptr[4] & 0x3f);
    return ptr + 5;
  } else {
    code = 0;
    return ptr;
  }
}

bool wide_emojis{false};
std::vector<UnicodeWidthBlock> override_blocks;

td::int32 my_wcwidth(td::int32 cp) {
  if (cp < 0x20 || (cp >= 0x80 && cp < 0xa0)) {
    return -1;
  }
  td::int32 l = -1;
  td::int32 r = (td::int32)override_blocks.size();
  while (r - l > 1) {
    auto x = (r + l) / 2;
    if (cp < override_blocks[x].begin) {
      r = x;
    } else if (cp > override_blocks[x].end) {
      l = x;
    } else {
      return override_blocks[x].width;
    }
  }
  return utf8proc_charwidth(cp);
}

bool is_control_char(td::int32 op) {
  return (op >= LEFT_ALIGN_BLOCK_START && op <= LEFT_ALIGN_BLOCK_END) ||
         (op >= RIGHT_ALIGN_BLOCK_START && op <= RIGHT_ALIGN_BLOCK_END) || (op == SOFT_LINE_BREAK_CP);
}

}  // namespace

void enable_wide_emojis() {
  wide_emojis = true;
}

void override_unicode_width(std::vector<UnicodeWidthBlock> blocks) {
  std::sort(blocks.begin(), blocks.end(),
            [&](const UnicodeWidthBlock &l, const UnicodeWidthBlock &r) { return l.begin < r.begin; });
  override_blocks = std::move(blocks);
}

static bool is_regional_indicator(td::int32 value) {
  return value >= 0x1f1e6 && value <= 0x1f1ff;
}

Graphem next_graphem(td::Slice data, size_t pos) {
  auto cur = data.ubegin() + pos;
  auto first = cur;
  auto last = data.uend();
  td::int32 cur_width = 0;
  td::int32 cur_codepoints = 0;
  td::int32 base_codepoint = 0;
  td::int32 graphems_cnt = 0;
  td::int32 first_codepoint = 0;
  while (cur < last) {
    td::uint32 code;
    auto next = next_utf8(cur, last, code);
    if (!code) {
      break;
    }
    bool is_first = cur_codepoints == 0;
    if (code == 0) {
      return Graphem{.data = td::Slice(first, cur), .width = -2, .unicode_codepoints = 1};
    }
    if (is_first) {
      first_codepoint = code;
    }
    bool control_symbol = is_control_char(code);
    auto width = my_wcwidth(code);
    if (width < 0) {
      if (is_first) {
        return Graphem{
            .data = td::Slice(first, next), .width = -1, .unicode_codepoints = 1, .first_codepoint = (td::int32)code};
      } else {
        break;
      }
    } else if (width == 0 && !control_symbol) {
      cur_codepoints++;
      if (wide_emojis && code == 0xFE0F && cur_width == 1) {
        cur_width++;
      }
      cur = next;
    } else {
      if (is_first) {
        cur_width += width;
        cur_codepoints++;
        cur = next;
        base_codepoint = code;
        graphems_cnt++;
        if (control_symbol) {
          break;
        }
      } else {
        if (graphems_cnt == 1 && is_regional_indicator(base_codepoint) && is_regional_indicator(code)) {
          graphems_cnt++;
          cur_width = 2;
          cur_codepoints++;
          cur = next;
        } else {
          break;
        }
      }
    }
  }

  return Graphem{.data = td::Slice(first, cur),
                 .width = cur_width,
                 .unicode_codepoints = cur_codepoints,
                 .first_codepoint = first_codepoint};
}

Graphem prev_graphem(td::Slice data, size_t pos) {
  auto cur = data.ubegin() + pos;
  auto first = cur;
  auto last = data.ubegin();
  td::int32 cur_width = 0;
  td::int32 cur_codepoints = 0;
  td::int32 first_codepoint = 0;
  while (cur > last) {
    cur--;

    if (!td::is_utf8_character_first_code_unit(*cur)) {
      continue;
    }

    td::uint32 code;
    next_utf8(cur, first, code);
    if (code == 0) {
      return Graphem{
          .data = td::Slice(first, cur), .width = -2, .unicode_codepoints = 1, .first_codepoint = (td::int32)code};
    }
    bool is_first = cur_codepoints == 0;
    auto width = my_wcwidth(code);
    if (width < 0) {
      if (is_first) {
        return Graphem{
            .data = td::Slice(cur, first), .width = -1, .unicode_codepoints = 1, .first_codepoint = (td::int32)code};
      } else {
        break;
      }
    } else if (width == 0) {
      cur_codepoints++;
      if (wide_emojis && code == 0xFE0F) {
        cur_width++;
      }
    } else {
      cur_width += width;
      cur_codepoints++;
      first_codepoint = code;
      break;
    }
    first_codepoint = code;
  }

  return Graphem{.data = td::Slice(cur, first),
                 .width = cur_width,
                 .unicode_codepoints = cur_codepoints,
                 .first_codepoint = first_codepoint};
}

size_t utf8_count(td::Slice S, UnicodeCounter &res, const UnicodeCounter *limit, bool advance_on_error) {
  UnicodeCounter here{};

  size_t cur_pos = 0;

  while (cur_pos < S.size() && S[cur_pos] != 0) {
    auto g = next_graphem(S, cur_pos);
    if (g.width < 0) {
      if (advance_on_error) {
        res = here;
      }
      return -1;
    }

    CHECK(g.data.size() > 0);

    res = here;

    if (limit && limit->bytes != (size_t)-1 && cur_pos + g.data.size() > limit->bytes) {
      break;
    }
    if (limit && here.codepoints + g.unicode_codepoints > limit->codepoints) {
      break;
    }
    if (limit && here.graphemes + 1 > limit->graphemes) {
      break;
    }
    if (limit && here.columns + g.width > limit->columns) {
      break;
    }

    cur_pos += g.data.size();
    here.bytes = cur_pos;
    here.codepoints += g.unicode_codepoints;
    here.graphemes += 1;
    here.columns += g.width;
  }

  res = here;

  return res.bytes;
}

Graphem next_graphems(td::Slice data, size_t pos, size_t limit_bytes, td::int32 limit_width, td::int32 limit_graphems,
                      td::int32 limit_codepoints) {
  limit_bytes = std::min<size_t>(data.size() - pos, limit_bytes);
  UnicodeCounter p = {.bytes = 0, .codepoints = 0, .graphemes = 0, .columns = 0};
  UnicodeCounter lim = {
      .bytes = limit_bytes,
      .codepoints = limit_codepoints >= 0 ? (size_t)limit_codepoints : std::numeric_limits<size_t>::max(),
      .graphemes = limit_graphems >= 0 ? (size_t)limit_graphems : std::numeric_limits<size_t>::max(),
      .columns = limit_width >= 0 ? (size_t)limit_width : std::numeric_limits<size_t>::max()};

  auto r = utf8_count(data.copy().remove_prefix(pos), p, &lim, true);
  if (r == (size_t)-1) {
    return Graphem{.data = data.remove_prefix(pos).truncate(1), .width = 0, .unicode_codepoints = 1};
  } else if (r == (size_t)-2) {
    return Graphem{.data = td::Slice(), .width = 0, .unicode_codepoints = 0};
  } else {
    return Graphem{.data = data.remove_prefix(pos).truncate(r),
                   .width = (td::int32)p.columns,
                   .unicode_codepoints = (td::int32)p.codepoints};
  }
}

td::Slice get_utf8_string_substring(td::Slice text, size_t from, size_t to) {
  if (from >= to) {
    return td::Slice();
  }
  const unsigned char *text_ptr = (const unsigned char *)text.data();
  auto text_start = text_ptr;
  size_t from_pos = text.size();
  size_t to_pos = text.size();
  size_t p = 0;
  while (*text_ptr) {
    if (from == p) {
      from_pos = text_ptr - text_start;
    }
    if (to == p) {
      to_pos = text_ptr - text_start;
      break;
    }
    td::uint32 code;
    text_ptr = td::next_utf8_unsafe(text_ptr, &code);
    p++;
  }

  return td::Slice(text).remove_prefix(from_pos).truncate(to_pos - from_pos);
}

td::Slice get_utf8_string_substring_utf16_codepoints(td::Slice text, size_t from, size_t to) {
  if (from >= to) {
    return td::Slice();
  }
  const unsigned char *text_ptr = (const unsigned char *)text.data();
  auto text_start = text_ptr;
  size_t from_pos = text.size();
  size_t to_pos = text.size();
  size_t p = 0;
  while (*text_ptr) {
    if (from == p) {
      from_pos = text_ptr - text_start;
    }
    if (to == p) {
      to_pos = text_ptr - text_start;
      break;
    }
    td::uint32 code;
    text_ptr = td::next_utf8_unsafe(text_ptr, &code);
    p += (code <= 0xffff) ? 1 : 2;  // UTF16 codepoints =((
  }

  return td::Slice(text).remove_prefix(from_pos).truncate(to_pos - from_pos);
}

td::uint32 utf8_code_to_str(td::int32 code, char buf[6]) {
  td::int32 p = 0;
  if (code <= 0x7f) {
    buf[p++] = static_cast<char>(code);
  } else if (code <= 0x7ff) {
    buf[p++] = static_cast<char>(0xc0 | (code >> 6));  // implementation-defined
    buf[p++] = static_cast<char>(0x80 | (code & 0x3f));
  } else if (code <= 0xffff) {
    buf[p++] = static_cast<char>(0xe0 | (code >> 12));  // implementation-defined
    buf[p++] = static_cast<char>(0x80 | ((code >> 6) & 0x3f));
    buf[p++] = static_cast<char>(0x80 | (code & 0x3f));
  } else {
    buf[p++] = static_cast<char>(0xf0 | (code >> 18));  // implementation-defined
    buf[p++] = static_cast<char>(0x80 | ((code >> 12) & 0x3f));
    buf[p++] = static_cast<char>(0x80 | ((code >> 6) & 0x3f));
    buf[p++] = static_cast<char>(0x80 | (code & 0x3f));
  }
  buf[p++] = 0;
  return p - 1;
}

td::int32 utf8_string_width(td::Slice data) {
  td::int32 res = 0;
  while (data.size() > 0) {
    auto x = next_graphem(data, 0);
    if (x.width >= 0) {
      res += x.width;
    }
    if (x.data.size() == 0) {
      break;
    }
    data.remove_prefix(x.data.size());
  }
  return res;
}

bool Graphem::is_whitespace() const {
  auto r = utf8proc_category(first_codepoint);

  return r == UTF8PROC_CATEGORY_ZS;
}
bool Graphem::is_whitespace_or_linebreak() const {
  auto r = utf8proc_category(first_codepoint);

  return r == UTF8PROC_CATEGORY_ZS || r == UTF8PROC_CATEGORY_ZL || r == UTF8PROC_CATEGORY_ZP ||
         first_codepoint == '\n' || first_codepoint == '\r';
}

bool Graphem::is_alpha() const {
  auto r = utf8proc_category(first_codepoint);

  return r == UTF8PROC_CATEGORY_LU || r == UTF8PROC_CATEGORY_LL || r == UTF8PROC_CATEGORY_LT ||
         r == UTF8PROC_CATEGORY_LM || r == UTF8PROC_CATEGORY_LO;
}

bool Graphem::is_num() const {
  auto r = utf8proc_category(first_codepoint);

  return r == UTF8PROC_CATEGORY_ND;
}

bool Graphem::is_alphanum() const {
  auto r = utf8proc_category(first_codepoint);

  return r == UTF8PROC_CATEGORY_LU || r == UTF8PROC_CATEGORY_LL || r == UTF8PROC_CATEGORY_LT ||
         r == UTF8PROC_CATEGORY_LM || r == UTF8PROC_CATEGORY_LO || r == UTF8PROC_CATEGORY_ND;
}
