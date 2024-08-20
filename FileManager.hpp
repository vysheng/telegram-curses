#pragma once

#include "auto/td/telegram/td_api.h"
#include "td/utils/common.h"
#include <functional>
#include <vector>
#include <map>

namespace tdcurses {

using FileUpdatedCallback = std::function<void(const td::td_api::updateFile &)>;

class FileManager {
 public:
  td::int64 subscribe_to_file_updates(td::int64 file_id, FileUpdatedCallback callback);
  void unsubscribe_from_file_updates(td::int64 file_id, td::int64 subscription_id);
  void process_update(const td::td_api::updateFile &update);

 private:
  std::map<td::int64, std::map<td::int64, FileUpdatedCallback>> subscriptions_;
  td::int64 last_subscription_id_{0};
};

FileManager &file_manager();

}  // namespace tdcurses
