#include "Input.hpp"
#include "td/utils/Slice-decl.h"
#include <cstring>
#include <memory>
#include <tickit.h>

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

}  // namespace windows
