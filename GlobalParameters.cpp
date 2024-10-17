#include "GlobalParameters.hpp"
#include "td/utils/Slice.h"

#include <unistd.h>
#include <fcntl.h>

namespace tdcurses {

static void close_fds() {
  int fd = open("/dev/null", O_RDWR);

  close(0);
  close(1);
  close(2);

  dup2(fd, 0);
  dup2(fd, 1);
  dup2(fd, 2);

  close(fd);
}

GlobalParameters &global_parameters() {
  static GlobalParameters ptr;
  return ptr;
}

void GlobalParameters::copy_to_primary_buffer(td::CSlice text) {
  auto p = vfork();
  if (!p) {
    close_fds();
    execlp(copy_command_.c_str(), copy_command_.c_str(), "--primary", "--", text.c_str(), NULL);
  }
}

void GlobalParameters::copy_to_clipboard(td::CSlice text) {
  auto p = vfork();
  if (!p) {
    close_fds();
    execlp(copy_command_.c_str(), copy_command_.c_str(), "--", text.c_str(), NULL);
  }
}

void GlobalParameters::open_document(td::CSlice file_path) {
  auto p = vfork();
  if (!p) {
    close_fds();
    execlp(file_open_command_.c_str(), file_open_command_.c_str(), "--", file_path.c_str(), NULL);
  }
}

void GlobalParameters::open_link(td::CSlice url) {
  if (url.size() == 0 || url[0] == '-') {
    return;
  }
  auto p = vfork();
  if (!p) {
    close_fds();
    execlp(link_open_command_.c_str(), link_open_command_.c_str(), url.c_str(), (char *)NULL);
  }
}

}  // namespace tdcurses
