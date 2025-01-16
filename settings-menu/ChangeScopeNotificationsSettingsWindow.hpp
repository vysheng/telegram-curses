#pragma once

#include "MenuWindowCommon.hpp"
#include "td/telegram/td_api.h"
#include <functional>
#include <memory>

namespace tdcurses {

class ChangeScopeNotificationsSettingsWindow : public MenuWindowCommon {
 public:
  ChangeScopeNotificationsSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, NotificationScope scope)
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

  virtual bool is_default(const td::td_api::chatNotificationSettings &settings) {
    return false;
  }
  virtual void output_chat_value(Outputter &out, const td::td_api::chatNotificationSettings &settings) {
    out << "UNKNOWN";
  }
  virtual void output_scope_value(Outputter &out, const td::td_api::scopeNotificationSettings &tl) {
    out << "UNKNOWN";
  }

 private:
  NotificationScope scope_;
  td::tl_object_ptr<td::td_api::scopeNotificationSettings> settings_;
  td::tl_object_ptr<td::td_api::chats> exceptions_;
};

class ChangeScopeNotificationsSettingsWindowBool : public ChangeScopeNotificationsSettingsWindow {
 public:
  ChangeScopeNotificationsSettingsWindowBool(
      Tdcurses *root, td::ActorId<Tdcurses> root_actor, NotificationScope scope,
      std::function<bool(const td::td_api::chatNotificationSettings &)> is_default,
      std::function<bool(const td::td_api::chatNotificationSettings &)> get_chat_value,
      std::function<bool(const td::td_api::scopeNotificationSettings &)> get_scope_value,
      std::function<void(td::td_api::chatNotificationSettings &, bool)> set_chat_value,
      std::function<void(td::td_api::scopeNotificationSettings &, bool)> set_scope_value)
      : ChangeScopeNotificationsSettingsWindow(root, root_actor, scope)
      , is_default_(std::move(is_default))
      , get_chat_value_(std::move(get_chat_value))
      , get_scope_value_(std::move(get_scope_value))
      , set_chat_value_(std::move(set_chat_value))
      , set_scope_value_(std::move(set_scope_value)) {
  }

  bool is_default(const td::td_api::chatNotificationSettings &settings) override {
    return is_default_(settings);
  }
  void output_chat_value(Outputter &out, const td::td_api::chatNotificationSettings &settings) override {
    out << (get_chat_value_(settings) ? "YES" : "NO");
  }
  void output_scope_value(Outputter &out, const td::td_api::scopeNotificationSettings &settings) override {
    out << (get_scope_value_(settings) ? "YES" : "NO");
  }

 private:
  std::function<bool(const td::td_api::chatNotificationSettings &)> is_default_;
  std::function<bool(const td::td_api::chatNotificationSettings &)> get_chat_value_;
  std::function<bool(const td::td_api::scopeNotificationSettings &)> get_scope_value_;
  std::function<void(td::td_api::chatNotificationSettings &, bool)> set_chat_value_;
  std::function<void(td::td_api::scopeNotificationSettings &, bool)> set_scope_value_;
};

class ChangeScopeNotificationsSettingsWindowMute : public ChangeScopeNotificationsSettingsWindow {
 public:
  ChangeScopeNotificationsSettingsWindowMute(
      Tdcurses *root, td::ActorId<Tdcurses> root_actor, NotificationScope scope,
      std::function<bool(const td::td_api::chatNotificationSettings &)> is_default,
      std::function<td::int32(const td::td_api::chatNotificationSettings &)> get_chat_value,
      std::function<td::int32(const td::td_api::scopeNotificationSettings &)> get_scope_value,
      std::function<void(td::td_api::chatNotificationSettings &, td::int32)> set_chat_value,
      std::function<void(td::td_api::scopeNotificationSettings &, td::int32)> set_scope_value)
      : ChangeScopeNotificationsSettingsWindow(root, root_actor, scope)
      , is_default_(std::move(is_default))
      , get_chat_value_(std::move(get_chat_value))
      , get_scope_value_(std::move(get_scope_value))
      , set_chat_value_(std::move(set_chat_value))
      , set_scope_value_(std::move(set_scope_value)) {
  }

  bool is_default(const td::td_api::chatNotificationSettings &settings) override {
    return is_default_(settings);
  }
  void output(Outputter &out, td::int32 date) {
    if (date <= 0) {
      out << "unmuted";
    } else if (date >= 10 * 365 * 86400) {
      out << "muted forever";
    } else {
      if (date >= 86400 * 365) {
        out << "muted for " << (date / (86400 * 365)) << " years";
      } else if (date >= 86400 * 30) {
        out << "muted for " << (date / (86400 * 30)) << " months";
      } else if (date >= 86400) {
        out << "muted for " << (date / (86400)) << " days";
      } else if (date >= 3600) {
        out << "muted for " << (date / (3600)) << " hours";
      } else if (date >= 60) {
        out << "muted for " << (date / (60)) << " minutes";
      } else {
        out << "muted for " << (date / (1)) << " seconds";
      }
    }
  }
  void output_chat_value(Outputter &out, const td::td_api::chatNotificationSettings &settings) override {
    output(out, get_chat_value_(settings));
  }
  void output_scope_value(Outputter &out, const td::td_api::scopeNotificationSettings &settings) override {
    output(out, get_scope_value_(settings));
  }

 private:
  std::function<bool(const td::td_api::chatNotificationSettings &)> is_default_;
  std::function<bool(const td::td_api::chatNotificationSettings &)> get_chat_value_;
  std::function<bool(const td::td_api::scopeNotificationSettings &)> get_scope_value_;
  std::function<void(td::td_api::chatNotificationSettings &, bool)> set_chat_value_;
  std::function<void(td::td_api::scopeNotificationSettings &, bool)> set_scope_value_;
};

}  // namespace tdcurses
