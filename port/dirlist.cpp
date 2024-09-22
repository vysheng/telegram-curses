#include "dirlist.h"
#include "td/utils/Status.h"
#include "td/utils/common.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

td::Status iterate_files_in_directory(std::string dir, std::function<void(const std::string &, const FileInfo &)> cb) {
  CHECK(dir.size() > 0);
  if (dir.back() != '/') {
    dir += '/';
  }

  auto d = opendir(dir.c_str());
  if (!d) {
    return OS_ERROR("opendir");
  }

  SCOPE_EXIT {
    closedir(d);
  };

  auto size = dir.size();

  while (true) {
    errno = 0;
    auto *entry = readdir(d);
    if (entry == nullptr) {
      return td::Status::OK();
    }
    auto name = td::Slice(static_cast<const char *>(entry->d_name));
    if (name == ".") {
      continue;
    }
    dir.append(name.begin(), name.size());
    SCOPE_EXIT {
      dir.resize(size);
    };

    FileInfo info;
    info.is_dir = false;
    info.size = 0;
    if (entry->d_type == DT_DIR) {
      info.is_dir = true;
    }

    auto r = access(dir.c_str(), info.is_dir ? (R_OK | X_OK) : R_OK);
    info.has_access = r >= 0;

    struct stat buf;
    r = stat(dir.c_str(), &buf);
    if (r >= 0) {
      info.size = buf.st_size;
      info.modified_at = buf.st_mtim.tv_sec;
    }

    cb(dir, info);
  }

  return td::Status::OK();
}
