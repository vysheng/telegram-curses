#include "FileManager.hpp"

namespace tdcurses {

FileManager &file_manager() {
  static FileManager file_manager;
  return file_manager;
}

td::int64 FileManager::subscribe_to_file_updates(td::int64 file_id, FileUpdatedCallback callback) {
  auto id = ++last_subscription_id_;
  subscriptions_[file_id].emplace(id, callback);
  return id;
}

void FileManager::unsubscribe_from_file_updates(td::int64 file_id, td::int64 subscription_id) {
  auto it = subscriptions_.find(file_id);
  CHECK(it != subscriptions_.end());
  CHECK(it->second.erase(subscription_id));
  if (!it->second.size()) {
    subscriptions_.erase(it);
  }
}

void FileManager::process_update(const td::td_api::updateFile &update) {
  auto it = subscriptions_.find(update.file_->id_);
  if (it != subscriptions_.end()) {
    auto copy = it->second;
    for (auto &x : copy) {
      x.second(update);
    }
  }
}

}  // namespace tdcurses
