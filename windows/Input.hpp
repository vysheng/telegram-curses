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
};

std::unique_ptr<InputEvent> parse_tickit_input_event(td::CSlice utf8, bool is_special);
std::unique_ptr<InputEvent> parse_ignore_input_event();

}  // namespace windows
