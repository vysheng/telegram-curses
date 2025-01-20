#pragma once

#include "MenuWindowCommon.hpp"
#include "GlobalParameters.hpp"

namespace tdcurses {

class ScopeNotificationsSettingsWindow : public MenuWindowCommon {
 public:
  ScopeNotificationsSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, NotificationScope scope)
      : MenuWindowCommon(root, std::move(root_actor)), scope_(scope) {
    send_requests();
  }
  void send_requests();
  void got_notification_settings(td::tl_object_ptr<td::td_api::scopeNotificationSettings> res) {
    settings_ = std::move(res);
    if (exceptions_) {
      build_menu();
    }
  }
  void got_exceptions(td::tl_object_ptr<td::td_api::chats> res) {
    exceptions_ = std::move(res);
    if (settings_) {
      build_menu();
    }
  }
  void build_menu();
  td::tl_object_ptr<td::td_api::scopeNotificationSettings> settings_tl();
  td::tl_object_ptr<td::td_api::NotificationSettingsScope> scope_tl();

 private:
  NotificationScope scope_;
  td::tl_object_ptr<td::td_api::scopeNotificationSettings> settings_;
  td::tl_object_ptr<td::td_api::chats> exceptions_;
};

}  // namespace tdcurses
