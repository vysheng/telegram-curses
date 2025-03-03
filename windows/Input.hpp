#pragma once

#include "td/utils/Slice-decl.h"
#include "td/utils/Slice.h"
#include <memory>

namespace windows {

class InputEvent {
 public:
  virtual ~InputEvent() = default;
  virtual bool is_end_of_input() const = 0;
  virtual bool is_key(const char *s) const;
  virtual bool is_key(td::Slice s) const = 0;
  virtual bool is_keyboard_event() const = 0;
  virtual bool is_text_key() const = 0;
  virtual const char *get_utf8_str() const = 0;
  bool operator==(const char *s) const;
  bool operator==(td::CSlice s) const {
    return *this == s.data();
  }
};

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

std::unique_ptr<InputEvent> parse_ignore_input_event();

}  // namespace windows
