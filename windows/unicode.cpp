#include "unicode.h"
#include "td/utils/utf8.h"
#include <tickit.h>
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
    auto width = my_wcwidth(code);
    if (width < 0) {
      if (is_first) {
        return Graphem{.data = td::Slice(first, next), .width = -1, .unicode_codepoints = 1};
      } else {
        break;
      }
    } else if (width == 0) {
      if (code == 0x200b /* ZWSP */) {
        if (!is_first) {
          break;
        }
      }
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

  return Graphem{.data = td::Slice(first, cur), .width = cur_width, .unicode_codepoints = cur_codepoints};
}

Graphem prev_graphem(td::Slice data, size_t pos) {
  auto cur = data.ubegin() + pos;
  auto first = cur;
  auto last = data.ubegin();
  td::int32 cur_width = 0;
  td::int32 cur_codepoints = 0;
  while (cur > last) {
    cur--;

    if (!td::is_utf8_character_first_code_unit(*cur)) {
      continue;
    }

    td::uint32 code;
    next_utf8(cur, first, code);
    if (code == 0) {
      return Graphem{.data = td::Slice(first, cur), .width = -2, .unicode_codepoints = 1};
    }
    bool is_first = cur_codepoints == 0;
    auto width = my_wcwidth(code);
    if (width < 0) {
      if (is_first) {
        return Graphem{.data = td::Slice(cur, first), .width = -1, .unicode_codepoints = 1};
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
      break;
    }
  }

  return Graphem{.data = td::Slice(cur, first), .width = cur_width, .unicode_codepoints = cur_codepoints};
}

size_t tickit_utf8_ncountmore_override(const char *str_tr, size_t len, TickitStringPos *pos,
                                       const TickitStringPos *limit, bool tickit_compatible) {
  TickitStringPos here = *pos;
  size_t start_bytes = pos->bytes;

  auto S = td::Slice(str_tr, len == (size_t)-1 ? strlen(str_tr) : len);

  size_t cur_pos = start_bytes;

  while (cur_pos < S.size() && S[cur_pos] != 0) {
    auto g = next_graphem(S, cur_pos);
    if (g.width < 0) {
      if (!tickit_compatible) {
        *pos = here;
      }
      return -1;
    }

    CHECK(g.data.size() > 0);

    *pos = here;

    if (limit && limit->bytes != (size_t)-1 && cur_pos + g.data.size() > limit->bytes) {
      break;
    }
    if (limit && limit->codepoints != -1 && here.codepoints + g.unicode_codepoints > limit->codepoints) {
      break;
    }
    if (limit && limit->graphemes != -1 && here.graphemes + 1 > limit->graphemes) {
      break;
    }
    if (limit && limit->columns != -1 && here.columns + g.width > limit->columns) {
      break;
    }

    cur_pos += g.data.size();
    here.bytes = cur_pos;
    here.codepoints += g.unicode_codepoints;
    here.graphemes += 1;
    here.columns += g.width;
  }

  *pos = here;

  return pos->bytes - start_bytes;
}

void set_tickit_wrap() {
  set_tickit_utf8_ncountmore_fn(
      [](const char *str, size_t len, TickitStringPos *pos, const TickitStringPos *limit) -> size_t {
        return tickit_utf8_ncountmore_override(str, len, pos, limit, true);
      });
}

size_t utf8_string_advance(const char *str, size_t len, TickitStringPos *pos, const TickitStringPos *limit) {
  return tickit_utf8_ncountmore_override(str, len, pos, limit, false);
}

Graphem next_graphems(td::Slice data, size_t pos, size_t limit_bytes, td::int32 limit_width, td::int32 limit_graphems,
                      td::int32 limit_codepoints) {
  limit_bytes = std::min<size_t>(data.size() - pos, limit_bytes);
  TickitStringPos p = {.bytes = 0, .codepoints = 0, .graphemes = 0, .columns = 0};
  TickitStringPos lim = {
      .bytes = limit_bytes, .codepoints = limit_codepoints, .graphemes = limit_graphems, .columns = limit_width};

  auto r = tickit_utf8_ncountmore_override(data.data() + pos, data.size() - pos, &p, &lim, false);
  if (r == (size_t)-1) {
    return Graphem{.data = data.remove_prefix(pos).truncate(1), .width = 0, .unicode_codepoints = 1};
  } else if (r == (size_t)-2) {
    return Graphem{.data = td::Slice(), .width = 0, .unicode_codepoints = 0};
  } else {
    return Graphem{.data = data.remove_prefix(pos).truncate(r), .width = p.columns, .unicode_codepoints = p.codepoints};
  }
}
