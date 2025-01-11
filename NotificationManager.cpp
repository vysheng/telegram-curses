#include "NotificationManager.hpp"
#include "td/telegram/TdDb.h"
#include "td/telegram/td_api.h"
#include <libnotify/notify.h>
#include <memory>
#include "GlobalParameters.hpp"
#include "ChatManager.hpp"
#include "TdObjectsOutput.h"

namespace tdcurses {

NotificationManager::Notification::Notification(td::int64 chat_id, const td::td_api::notification &message)
    : chat_id_(chat_id) {
  update_info(message);

  auto notification = notify_notification_new(header_.c_str(), text_.c_str(), nullptr);
  if (!notification) {
    LOG(ERROR) << "failed to create notification";
    return;
  }

  GError *error = nullptr;

  if (!notify_notification_show(notification, &error)) {
    LOG(ERROR) << "failed to show notification";
    if (error) {
      g_error_free(error);
    }
    g_free(notification);
    return;
  }

  notification_ = notification;
}

NotificationManager::Notification::~Notification() {
  delete_notification();
}

void NotificationManager::Notification::update_info(const td::td_api::notification &n) {
  auto chat = chat_manager().get_chat(chat_id_);
  header_ = (chat ? chat->title() : "UPDATE");

  if (n.type_->get_id() != td::td_api::notificationTypeNewMessage::ID) {
    return;
  }

  auto &notif = static_cast<const td::td_api::notificationTypeNewMessage &>(*n.type_);
  if (!notif.show_preview_) {
    text_ = "NEW MESSAGE";
    return;
  }

  Outputter out;
  out << *notif.message_;
  text_ = out.as_str();
}

void NotificationManager::Notification::process_update(td::td_api::notification &update) {
  update_info(update);
  if (!notification_) {
    return;
  }
  auto ptr = static_cast<::NotifyNotification *>(notification_);
  notify_notification_update(ptr, header_.c_str(), text_.c_str(), nullptr);
}

void NotificationManager::Notification::delete_notification() {
  if (!notification_) {
    return;
  }
  auto ptr = static_cast<::NotifyNotification *>(notification_);
  GError *error = nullptr;
  notify_notification_close(ptr, &error);
  g_free(ptr);
  if (error) {
    g_error_free(error);
  }
  notification_ = nullptr;
}

void NotificationManager::process_update(td::td_api::updateNotification &update) {
  if (!global_parameters().notifications_enabled()) {
    return;
  }
  NotificationId id;
  id.group_id = update.notification_group_id_;
  id.notification_id = update.notification_->id_;
  auto it = notifications_.find(id);
  if (it == notifications_.end()) {
    return;
  }
  it->second->process_update(*update.notification_);
}

void NotificationManager::process_update(td::td_api::updateNotificationGroup &update) {
  if (!global_parameters().notifications_enabled()) {
    return;
  }
  if (update.type_->get_id() != td::td_api::notificationGroupTypeMessages::ID) {
    return;
  }
  for (auto &u : update.added_notifications_) {
    NotificationId id;
    id.group_id = update.notification_group_id_;
    id.notification_id = u->id_;
    auto it = notifications_.find(id);
    if (it != notifications_.end()) {
      return;
    }
    auto n = std::make_unique<Notification>(update.chat_id_, *u);
    notifications_.emplace(id, std::move(n));
  }
  for (auto &u : update.removed_notification_ids_) {
    NotificationId id;
    id.group_id = update.notification_group_id_;
    id.notification_id = u;
    auto it = notifications_.find(id);
    if (it != notifications_.end()) {
      notifications_.erase(it);
      return;
    }
  }
}

NotificationManager::NotificationManager() {
  notify_init("telegram-curses");
}

NotificationManager &notification_manager() {
  static NotificationManager notification_manager;
  return notification_manager;
}

}  // namespace tdcurses
