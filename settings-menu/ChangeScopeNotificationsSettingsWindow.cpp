#include "ChangeScopeNotificationsSettingsWindow.hpp"
#include "ChatManager.hpp"
#include "SelectionWindow.hpp"
#include "LoadingWindow.hpp"
#include "ErrorWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"

namespace tdcurses {

td::tl_object_ptr<td::td_api::scopeNotificationSettings> ChangeScopeNotificationsSettingsWindow::settings_tl() {
  return td::make_tl_object<td::td_api::scopeNotificationSettings>(
      settings_->mute_for_, settings_->sound_id_, settings_->show_preview_, settings_->use_default_mute_stories_,
      settings_->mute_stories_, settings_->story_sound_id_, settings_->show_story_sender_,
      settings_->disable_pinned_message_notifications_, settings_->disable_mention_notifications_);
}

td::tl_object_ptr<td::td_api::chatNotificationSettings>
ChangeScopeNotificationsSettingsWindow::clone_chat_notification_settings(
    const td::td_api::chatNotificationSettings &tl) {
  return td::make_tl_object<td::td_api::chatNotificationSettings>(
      tl.use_default_mute_for_, tl.mute_for_, tl.use_default_sound_, tl.sound_id_, tl.use_default_show_preview_,
      tl.show_preview_, tl.use_default_mute_stories_, tl.mute_stories_, tl.use_default_story_sound_, tl.story_sound_id_,
      tl.use_default_show_story_sender_, tl.show_story_sender_, tl.use_default_disable_pinned_message_notifications_,
      tl.disable_pinned_message_notifications_, tl.use_default_disable_mention_notifications_,
      tl.disable_mention_notifications_);
}

td::tl_object_ptr<td::td_api::NotificationSettingsScope> ChangeScopeNotificationsSettingsWindow::scope_tl() {
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

void ChangeScopeNotificationsSettingsWindow::send_requests() {
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

void ChangeScopeNotificationsSettingsWindow::build_menu() {
  {
    Outputter out;
    output_scope_value(out, *settings_);
    out << Outputter::RightPad{"<change>"};
    add_element("default", out.as_str(), out.markup(), [](MenuWindowCommon &w) -> bool {
      auto &self = static_cast<ChangeScopeNotificationsSettingsWindow &>(w);
      spawn_selection_window(self, "change mute for", self.values(false), [&self](td::Result<size_t> R) {
        if (R.is_error()) {
          return;
        }

        auto value = R.move_as_ok();
        auto scope = self.scope_tl();
        auto tl = self.settings_tl();
        self.set_scope_value(*tl, value);
        loading_window_send_request(
            self, "updating notification settings", {},
            td::make_tl_object<td::td_api::setScopeNotificationSettings>(std::move(scope), std::move(tl)),
            [&self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
              if (R.is_error()) {
                if (R.error().code() != ErrorCodeWindowDeleted) {
                  spawn_error_window(self, PSTRING() << "failed to update notification settings " << R.move_as_error(),
                                     {});
                }
              }
            });
      });
      return false;
    });
  }
  for (auto c : exceptions_->chat_ids_) {
    auto chat = chat_manager().get_chat(c);
    if (!chat || !chat->notification_settins()) {
      continue;
    }
    if (is_default(*chat->notification_settins())) {
      continue;
    }
    Outputter out;
    output_chat_value(out, *chat->notification_settins());
    out << Outputter::RightPad{"<change>"};
    add_element(chat->title(), out.as_str(), out.markup(), [chat](MenuWindowCommon &w) -> bool {
      auto &self = static_cast<ChangeScopeNotificationsSettingsWindow &>(w);
      spawn_selection_window(self, "change mute for", self.values(true), [&self, chat](td::Result<size_t> R) {
        if (R.is_error()) {
          return;
        }

        auto value = R.move_as_ok();
        auto tl = self.clone_chat_notification_settings(*chat->notification_settins());
        self.set_chat_value(*tl, value);
        loading_window_send_request(
            self, "updating notification settings", {},
            td::make_tl_object<td::td_api::setChatNotificationSettings>(chat->chat_id(), std::move(tl)),
            [&self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
              if (R.is_error()) {
                if (R.error().code() != ErrorCodeWindowDeleted) {
                  spawn_error_window(self, PSTRING() << "failed to update notification settings " << R.move_as_error(),
                                     {});
                }
              }
            });
      });
      return false;
    });
  }
}

}  // namespace tdcurses
