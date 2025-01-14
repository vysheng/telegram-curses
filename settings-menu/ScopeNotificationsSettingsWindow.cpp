#include "ScopeNotificationsSettingsWindow.hpp"
#include "ChatManager.hpp"
#include "LoadingWindow.hpp"
#include "SelectionWindow.hpp"
#include "ErrorWindow.hpp"

namespace tdcurses {

td::tl_object_ptr<td::td_api::scopeNotificationSettings> ScopeNotificationsSettingsWindow::settings_tl() {
  return td::make_tl_object<td::td_api::scopeNotificationSettings>(
      settings_->mute_for_, settings_->sound_id_, settings_->show_preview_, settings_->use_default_mute_stories_,
      settings_->mute_stories_, settings_->story_sound_id_, settings_->show_story_sender_,
      settings_->disable_pinned_message_notifications_, settings_->disable_mention_notifications_);
}

td::tl_object_ptr<td::td_api::NotificationSettingsScope> ScopeNotificationsSettingsWindow::scope_tl() {
  if (scope_ == NotificationScope::NotificationScopePrivateChats) {
    return td::make_tl_object<td::td_api::notificationSettingsScopePrivateChats>();
  } else if (scope_ == NotificationScope::NotificationScopeGroupChats) {
    return td::make_tl_object<td::td_api::notificationSettingsScopeGroupChats>();
  } else if (scope_ == NotificationScope::NotificationScopeChannelChats) {
    return td::make_tl_object<td::td_api::notificationSettingsScopeChannelChats>();
  } else {
    UNREACHABLE();
  }
}

void ScopeNotificationsSettingsWindow::send_requests() {
  send_request(td::make_tl_object<td::td_api::getScopeNotificationSettings>(scope_tl()),
               [window = this](td::Result<td::tl_object_ptr<td::td_api::scopeNotificationSettings>> R) {
                 if (R.is_error()) {
                   if (R.error().code() != ErrorCodeWindowDeleted) {
                     R.ensure();
                   }
                   return;
                 }
                 window->got_notification_settings(R.move_as_ok());
               });
  send_request(td::make_tl_object<td::td_api::getChatNotificationSettingsExceptions>(scope_tl(), true),
               [window = this](td::Result<td::tl_object_ptr<td::td_api::chats>> R) {
                 if (R.is_error()) {
                   if (R.error().code() != ErrorCodeWindowDeleted) {
                     R.ensure();
                   }
                   return;
                 }
                 window->got_exceptions(R.move_as_ok());
               });
}

void ScopeNotificationsSettingsWindow::build_menu() {
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_mute_for_) {
        ex++;
      }
    }
    Outputter out;
    if (settings_->mute_for_) {
      out << "mute for " << settings_->mute_for_;
    } else {
      out << "unmuted";
    }
    out << " (" << ex << " exceptions)" << Outputter::RightPad{"<change>"};
    add_element("mute", out.as_str(), out.markup(), [](MenuWindowCommon &w) -> bool {
      spawn_selection_window(
          w, "change mute for", {"unmute", "mute forever", "30 minutes", "1 hour", "8 hours"},
          [&self = static_cast<ScopeNotificationsSettingsWindow &>(w)](td::Result<size_t> R) {
            if (R.is_error()) {
              return;
            }
            auto tl = self.settings_tl();
            auto scope = self.scope_tl();
            switch (R.move_as_ok()) {
              case 0:
                tl->mute_for_ = 0;
                break;
              case 1:
                tl->mute_for_ = std::numeric_limits<td::int32>::max();
                break;
              case 2:
                tl->mute_for_ = 30 * 60;
                break;
              case 3:
                tl->mute_for_ = 60 * 60;
                break;
              case 4:
                tl->mute_for_ = 8 * 60 * 60;
                break;
            }
            loading_window_send_request(
                self, "updating notification settings", {},
                td::make_tl_object<td::td_api::setScopeNotificationSettings>(std::move(scope), std::move(tl)),
                [&self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                  if (R.is_error()) {
                    if (R.error().code() != ErrorCodeWindowDeleted) {
                      spawn_error_window(
                          self, PSTRING() << "failed to update notification settings " << R.move_as_error(), {});
                    }
                  }
                });
          });
      return false;
    });
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_sound_) {
        ex++;
      }
    }
    Outputter out;
    out << settings_->sound_id_;
    out << " (" << ex << " exceptions)";
    add_element("sound_id", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_show_preview_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->show_preview_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("show preview", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_mute_stories_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->use_default_mute_stories_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("default mute stories", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_mute_stories_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->mute_stories_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("mute stories", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_story_sound_) {
        ex++;
      }
    }
    Outputter out;
    out << settings_->story_sound_id_;
    out << " (" << ex << " exceptions)";
    add_element("stories sound", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_show_story_sender_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->show_story_sender_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("show story sender", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_disable_pinned_message_notifications_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->disable_pinned_message_notifications_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("disable pin notification", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_disable_mention_notifications_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->disable_mention_notifications_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("disable mention notification", out.as_str(), out.markup());
  }
}

}  // namespace tdcurses
