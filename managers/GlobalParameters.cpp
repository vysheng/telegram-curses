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
  int pipe_fds[2];
  auto r = pipe(pipe_fds);
  if (r < 0) {
    LOG(ERROR) << "failed to create pipe: " << strerror(errno);
    return;
  }
  auto p = vfork();
  if (!p) {
    close_fds();
    close(pipe_fds[0]);
    dup2(pipe_fds[1], 0);
    close(pipe_fds[1]);
    execlp(copy_command_.c_str(), copy_command_.c_str(), "--primary", "--", text.c_str(), NULL);
  } else {
    close(pipe_fds[1]);
    write(pipe_fds[0], text.data(), text.size());
    close(pipe_fds[0]);
  }
}

void GlobalParameters::copy_to_clipboard(td::CSlice text) {
  int pipe_fds[2];
  auto r = pipe(pipe_fds);
  if (r < 0) {
    LOG(ERROR) << "failed to create pipe: " << strerror(errno);
    return;
  }
  auto p = vfork();
  if (!p) {
    close_fds();
    close(pipe_fds[0]);
    dup2(pipe_fds[1], 0);
    close(pipe_fds[1]);
    execlp(copy_command_.c_str(), copy_command_.c_str(), NULL);
  } else {
    close(pipe_fds[1]);
    write(pipe_fds[0], text.data(), text.size());
    close(pipe_fds[0]);
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
