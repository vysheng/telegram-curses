#pragma once

#include "MenuWindowCommon.hpp"
#include "GlobalParameters.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include <functional>
#include <limits>
#include <memory>
#include <vector>

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

  virtual std::vector<std::string> values(bool add_default) {
    return {"TEST"};
  }

  virtual void set_scope_value(td::td_api::scopeNotificationSettings &settings, size_t value) {
  }
  virtual void set_chat_value(td::td_api::chatNotificationSettings &settings, size_t value) {
  }

  static td::tl_object_ptr<td::td_api::chatNotificationSettings> clone_chat_notification_settings(
      const td::td_api::chatNotificationSettings &tl);

 private:
  NotificationScope scope_;
  td::tl_object_ptr<td::td_api::scopeNotificationSettings> settings_;
  td::tl_object_ptr<td::td_api::chats> exceptions_;
};

class ChangeScopeNotificationsSettingsWindowBool : public ChangeScopeNotificationsSettingsWindow {
 public:
  ChangeScopeNotificationsSettingsWindowBool(Tdcurses *root, td::ActorId<Tdcurses> root_actor, NotificationScope scope,
                                             bool td::td_api::scopeNotificationSettings::*scope_value,
                                             bool td::td_api::chatNotificationSettings::*is_default,
                                             bool td::td_api::chatNotificationSettings::*chat_value)
      : ChangeScopeNotificationsSettingsWindow(root, root_actor, scope)
      , scope_value_(scope_value)
      , is_default_(std::move(is_default))
      , chat_value_(chat_value) {
  }

  bool is_default(const td::td_api::chatNotificationSettings &tl) override {
    return tl.*is_default_;
  }
  void output_chat_value(Outputter &out, const td::td_api::chatNotificationSettings &tl) override {
    out << (tl.*chat_value_ ? "YES" : "NO");
  }
  void output_scope_value(Outputter &out, const td::td_api::scopeNotificationSettings &tl) override {
    out << (tl.*scope_value_ ? "YES" : "NO");
  }

  std::vector<std::string> values(bool add_default) override {
    if (add_default) {
      return {"enabled", "disabled", "default"};
    } else {
      return {"enabled", "disabled"};
    }
  }

  void set_scope_value(td::td_api::scopeNotificationSettings &tl, size_t value) override {
    tl.*scope_value_ = (value == 0 ? true : false);
  }
  void set_chat_value(td::td_api::chatNotificationSettings &tl, size_t value) override {
    if (value == 2) {
      tl.*is_default_ = true;
    } else {
      tl.*chat_value_ = (value == 0 ? true : false);
    }
  }

 private:
  bool td::td_api::scopeNotificationSettings::*scope_value_;
  bool td::td_api::chatNotificationSettings::*is_default_;
  bool td::td_api::chatNotificationSettings::*chat_value_;
};

class ChangeScopeNotificationsSettingsWindowMute : public ChangeScopeNotificationsSettingsWindow {
 public:
  ChangeScopeNotificationsSettingsWindowMute(Tdcurses *root, td::ActorId<Tdcurses> root_actor, NotificationScope scope,
                                             td::int32 td::td_api::scopeNotificationSettings::*scope_value,
                                             bool td::td_api::chatNotificationSettings::*is_default,
                                             td::int32 td::td_api::chatNotificationSettings::*chat_value)
      : ChangeScopeNotificationsSettingsWindow(root, root_actor, scope)
      , scope_value_(scope_value)
      , is_default_(std::move(is_default))
      , chat_value_(chat_value) {
  }

  bool is_default(const td::td_api::chatNotificationSettings &tl) override {
    return tl.*is_default_;
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
  void output_chat_value(Outputter &out, const td::td_api::chatNotificationSettings &tl) override {
    output(out, tl.*chat_value_);
  }
  void output_scope_value(Outputter &out, const td::td_api::scopeNotificationSettings &tl) override {
    output(out, tl.*scope_value_);
  }

  std::vector<std::string> values(bool add_default) override {
    auto v = std::vector<std::string>{"unmute",  "forever", "1 minute", "10 minutes", "1 hour",
                                      "8 hours", "1 day",   "1 week",   "1 month",    "1 year"};
    if (add_default) {
      v.push_back("default");
    }
    return v;
  }

  td::int32 value_idx_to_int(size_t idx) {
    static const std::vector<td::int32> P{
        0, std::numeric_limits<td::int32>::max(), 60, 600, 3600, 3600 * 8, 86400, 7 * 86400, 30 * 86400, 365 * 86400,
        -1};
    return P[idx];
  }

  void set_scope_value(td::td_api::scopeNotificationSettings &tl, size_t value) override {
    tl.*scope_value_ = value_idx_to_int(value);
  }

  void set_chat_value(td::td_api::chatNotificationSettings &tl, size_t value) override {
    auto v = value_idx_to_int(value);
    if (v < 0) {
      tl.*is_default_ = true;
    } else {
      tl.*chat_value_ = v;
    }
  }

 private:
  td::int32 td::td_api::scopeNotificationSettings::*scope_value_;
  bool td::td_api::chatNotificationSettings::*is_default_;
  td::int32 td::td_api::chatNotificationSettings::*chat_value_;
};

class ChangeScopeNotificationsSettingsWindowSound : public ChangeScopeNotificationsSettingsWindow {
 public:
  ChangeScopeNotificationsSettingsWindowSound(Tdcurses *root, td::ActorId<Tdcurses> root_actor, NotificationScope scope,
                                              td::int64 td::td_api::scopeNotificationSettings::*scope_value,
                                              bool td::td_api::chatNotificationSettings::*is_default,
                                              td::int64 td::td_api::chatNotificationSettings::*chat_value)
      : ChangeScopeNotificationsSettingsWindow(root, root_actor, scope)
      , scope_value_(scope_value)
      , is_default_(std::move(is_default))
      , chat_value_(chat_value) {
  }

  bool is_default(const td::td_api::chatNotificationSettings &tl) override {
    return tl.*is_default_;
  }
  void output(Outputter &out, td::int64 sound_id) {
    auto &x = global_parameters().notification_sounds();
    if (sound_id == 0) {
      out << "no sound";
    } else {
      auto it = x.find(sound_id);
      if (it != x.end()) {
        out << it->second->title_;
      } else {
        out << "system default {" << sound_id << "}";
      }
    }
  }
  void output_chat_value(Outputter &out, const td::td_api::chatNotificationSettings &tl) override {
    output(out, tl.*chat_value_);
  }
  void output_scope_value(Outputter &out, const td::td_api::scopeNotificationSettings &tl) override {
    output(out, tl.*scope_value_);
  }

  std::vector<std::string> values(bool add_default) override {
    auto v = std::vector<std::string>{"no sound", "system default"};
    auto &x = global_parameters().notification_sounds();
    for (auto &e : x) {
      v.push_back(e.second->title_);
    }
    if (add_default) {
      v.push_back("default");
    }
    return v;
  }

  td::int64 value_idx_to_int(size_t idx) {
    auto &x = global_parameters().notification_sounds();
    if (idx == 0) {
      return 0;
    } else if (idx == 1) {
      return 1;
    } else if (idx < x.size() + 2) {
      auto it = x.begin();
      for (size_t i = 2; i < idx; i++) {
        it++;
      }
      return it->first;
    } else {
      return -999;
    }
  }

  void set_scope_value(td::td_api::scopeNotificationSettings &tl, size_t value) override {
    tl.*scope_value_ = value_idx_to_int(value);
  }

  void set_chat_value(td::td_api::chatNotificationSettings &tl, size_t value) override {
    auto v = value_idx_to_int(value);
    if (v == -999) {
      tl.*is_default_ = true;
    } else {
      tl.*chat_value_ = v;
    }
  }

 private:
  td::int64 td::td_api::scopeNotificationSettings::*scope_value_;
  bool td::td_api::chatNotificationSettings::*is_default_;
  td::int64 td::td_api::chatNotificationSettings::*chat_value_;
};

}  // namespace tdcurses
