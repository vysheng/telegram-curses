#include "GlobalParameters.hpp"
#include "td/utils/Slice-decl.h"

#include <unistd.h>

namespace tdcurses {

GlobalParameters &global_parameters() {
  static GlobalParameters ptr;
  return ptr;
}

void GlobalParameters::copy_to_primary_buffer(td::CSlice text) {
  auto p = vfork();
  if (!p) {
    execlp(copy_command_.c_str(), copy_command_.c_str(), "--primary", "--", text.c_str(), NULL);
  }
}

void GlobalParameters::copy_to_clipboard(td::CSlice text) {
  auto p = vfork();
  if (!p) {
    execlp(copy_command_.c_str(), copy_command_.c_str(), "--", text.c_str(), NULL);
  }
}

void GlobalParameters::open_document(td::CSlice file_path) {
  auto p = vfork();
  if (!p) {
    execlp(file_open_command_.c_str(), file_open_command_.c_str(), "--", file_path.c_str(), NULL);
  }
}

void GlobalParameters::open_link(td::CSlice url) {
  auto p = vfork();
  if (!p) {
    execlp(link_open_command_.c_str(), link_open_command_.c_str(), url.c_str(), (char *)NULL);
  }
}

}  // namespace tdcurses
