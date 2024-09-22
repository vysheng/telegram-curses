#pragma once

#include <string>
#include <functional>
#include "td/utils/Status.h"

struct FileInfo {
  bool is_dir;
  bool has_access;
  size_t size;
  td::int64 modified_at;
};

td::Status iterate_files_in_directory(std::string dir, std::function<void(const std::string &, const FileInfo &)> cb);
