#include "Input.hpp"
#include "td/utils/Slice-decl.h"
#include "td/utils/Slice.h"
#include "td/utils/logging.h"
#include <cstring>
#include <memory>
#include <notcurses/notcurses.h>

namespace windows {

bool InputEvent::is_key(const char *s) const {
  if (!s) {
    return false;
  }
  auto len = strlen(s);
  return is_key(td::Slice(s, s + len));
}

InputEvent *handle_input() {
  return nullptr;
}

std::unique_ptr<InputEvent> parse_ignore_input_event() {
  return std::make_unique<CommonInputEvent>();
}

bool InputEvent::operator==(const char *s) const {
  return is_key(td::CSlice(s));
}

}  // namespace windows
