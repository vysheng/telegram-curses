#include "ChangeScopeNotificationsSettingsWindow.hpp"
#include "ChatManager.hpp"

namespace tdcurses {

td::tl_object_ptr<td::td_api::scopeNotificationSettings> ChangeScopeNotificationsSettingsWindow::settings_tl() {
  return td::make_tl_object<td::td_api::scopeNotificationSettings>(
      settings_->mute_for_, settings_->sound_id_, settings_->show_preview_, settings_->use_default_mute_stories_,
      settings_->mute_stories_, settings_->story_sound_id_, settings_->show_story_sender_,
      settings_->disable_pinned_message_notifications_, settings_->disable_mention_notifications_);
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
    add_element("default", out.as_str(), out.markup());
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
    add_element(chat->title(), out.as_str(), out.markup());
  }
}

}  // namespace tdcurses
