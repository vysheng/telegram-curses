#include "Input.hpp"
#include "td/utils/Slice-decl.h"
#include "td/utils/Slice.h"
#include "td/utils/logging.h"
#include <cstring>
#include <memory>
#include <tickit.h>
#include <notcurses/notcurses.h>

namespace windows {

bool InputEvent::is_key(const char *s) const {
  if (!s) {
    return false;
  }
  auto len = strlen(s);
  return is_key(td::Slice(s, s + len));
}

class CommonInputEvent : public InputEvent {
 public:
  CommonInputEvent() : ignore_(true) {
  }
  CommonInputEvent(bool special, bool alt, bool ctrl, td::CSlice utf8)
      : ignore_(false), special_(special), alt_(alt), ctrl_(ctrl), utf8_(utf8) {
  }
  bool is_end_of_input() const override {
    return is_key("T-EOL");
  }
  bool is_key(td::Slice s) const override {
    if (ignore_) {
      return false;
    }
    bool special = false, alt = false, ctrl = false;
    while (true) {
      if (s.size() <= 1) {
        break;
      }
      if (s[1] != '-') {
        break;
      }
      if (s[0] == 'C') {
        ctrl = true;
        s.remove_prefix(2);
        continue;
      }
      if (s[0] == 'M') {
        alt = true;
        s.remove_prefix(2);
        continue;
      }
      if (s[0] == 'T') {
        special = true;
        s.remove_prefix(2);
        break;
      }
      break;
    }
    if (special != special_ || alt != alt_ || ctrl_ != ctrl) {
      return false;
    }
    return s == utf8_;
  }
  bool is_keyboard_event() const override {
    return !ignore_;
  }
  bool is_text_key() const override {
    return !ignore_ && !special_ && !alt_ && !ctrl_;
  }
  const char *get_utf8_str() const override {
    return utf8_.begin();
  }

 private:
  bool ignore_{false};
  bool special_{false};
  bool alt_{false};
  bool ctrl_{false};
  td::CSlice utf8_;
};

InputEvent *handle_input() {
  return nullptr;
}

std::unique_ptr<InputEvent> parse_ignore_input_event() {
  return std::make_unique<CommonInputEvent>();
}

std::unique_ptr<InputEvent> parse_tickit_input_event(td::CSlice utf8, bool is_special) {
  if (!is_special) {
    return std::make_unique<CommonInputEvent>(false, false, false, utf8);
  }

  bool alt = false, ctrl = false;
  while (true) {
    if (utf8.size() <= 1) {
      break;
    }
    if (utf8[1] != '-') {
      break;
    }
    if (utf8[0] == 'C') {
      ctrl = true;
      utf8.remove_prefix(2);
      continue;
    }
    if (utf8[0] == 'M') {
      alt = true;
      utf8.remove_prefix(2);
      continue;
    }
    break;
  }
  return std::make_unique<CommonInputEvent>(utf8.size() >= 2, alt, ctrl, utf8);
}

bool InputEvent::operator==(const char *s) const {
  return is_key(td::CSlice(s));
}

std::unique_ptr<InputEvent> parse_notcurses_input_event(td::int32 code, td::MutableCSlice utf8, bool alt, bool ctrl,
                                                        bool shift) {
  if (code >= 'A' && code <= 'Z' && !shift) {
    CHECK(utf8[0] == code);
    code += 'a' - 'A';
    utf8[0] += 'a' - 'A';
  }
  switch (code) {
    case NCKEY_UP:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Up");
    case NCKEY_RIGHT:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Right");
    case NCKEY_DOWN:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Down");
    case NCKEY_LEFT:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Left");
    case NCKEY_INS:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Insert");
    case NCKEY_DEL:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Delete");
    case NCKEY_BACKSPACE:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Backspace");
    case NCKEY_PGDOWN:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "PageDown");
    case NCKEY_PGUP:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "PageUp");
    case NCKEY_HOME:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Home");
    case NCKEY_END:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "End");
    case NCKEY_F00 ... NCKEY_F09: {
      static char s[3];
      s[0] = 'F';
      s[1] = (char)('0' + (code - NCKEY_F00));
      s[2] = 0;
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, td::CSlice(s, s + 2));
    }
    case NCKEY_F10 ... NCKEY_F40: {
      static char s[4];
      s[0] = 'F';
      auto v = (code - NCKEY_F00);
      s[1] = (char)('0' + (v / 10));
      s[2] = (char)('0' + (v % 10));
      s[3] = 0;
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, td::CSlice(s, s + 3));
    }
    case NCKEY_ENTER:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Enter");
    case NCKEY_TAB:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Tab");
    case NCKEY_ESC:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Escape");
  }

  LOG(ERROR) << "code=" << code << " utf8=" << utf8.data();

  if (!nckey_pua_p(code) && !nckey_supppuaa_p(code) && !nckey_supppuab_p(code) &&
      code <= 1114112 /* beyond plane 17 */) {
    return std::make_unique<CommonInputEvent>(false, alt, ctrl, utf8);
  }

  return nullptr;
}

}  // namespace windows
