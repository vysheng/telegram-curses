#pragma once

#include "Tdcurses.hpp"
#include "td/telegram/td_api.h"
#include <memory>

namespace tdcurses {

class NotificationManager {
 public:
  NotificationManager();
  struct NotificationId {
    td::int32 group_id;
    td::int32 notification_id;
    bool operator<(const NotificationId &other) const {
      return std::tie(group_id, notification_id) < std::tie(other.group_id, other.notification_id);
    }
  };
  class Notification {
   public:
    Notification(td::int64 chat_id, const td::td_api::notification &message);
    ~Notification();

    void update_info(const td::td_api::notification &n);
    void process_update(td::td_api::notification &update);
    void delete_notification();

   private:
    td::int64 chat_id_;
    std::string header_;
    std::string text_;
    void *notification_{nullptr};
  };

  void process_update(td::td_api::updateNotification &update);
  void process_update(td::td_api::updateNotificationGroup &update);

 private:
  std::map<NotificationId, std::unique_ptr<Notification>> notifications_;
};

NotificationManager &notification_manager();

}  // namespace tdcurses
