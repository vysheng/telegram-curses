#include "FileManager.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"

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

td::tl_object_ptr<td::td_api::file> clone_td_file(const td::td_api::file &file_in) {
  td::tl_object_ptr<td::td_api::localFile> l;
  if (file_in.local_) {
    l = td::make_tl_object<td::td_api::localFile>(
        file_in.local_->path_, file_in.local_->can_be_downloaded_, file_in.local_->can_be_deleted_,
        file_in.local_->is_downloading_active_, file_in.local_->is_downloading_completed_,
        file_in.local_->download_offset_, file_in.local_->downloaded_prefix_size_, file_in.local_->downloaded_size_);
  }
  td::tl_object_ptr<td::td_api::remoteFile> r;
  if (file_in.remote_) {
    r = td::make_tl_object<td::td_api::remoteFile>(
        file_in.remote_->id_, file_in.remote_->unique_id_, file_in.remote_->is_uploading_active_,
        file_in.remote_->is_uploading_completed_, file_in.remote_->uploaded_size_);
  }
  return td::make_tl_object<td::td_api::file>(file_in.id_, file_in.size_, file_in.expected_size_, std::move(l),
                                              std::move(r));
}

}  // namespace tdcurses
