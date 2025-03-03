#pragma once

#include "auto/td/telegram/td_api.h"
#include "auto/td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/common.h"
#include "td/utils/overloaded.h"
#include "td/utils/port/Stat.h"
#include "windows/Markup.hpp"
#include "windows/Output.hpp"
#include <map>
#include <array>
#include <vector>

namespace tdcurses {

enum NotificationScope : td::uint32 {
  NotificationScopePrivateChats = 0,
  NotificationScopeGroupChats = 1,
  NotificationScopeChannelChats = 2,
  NotificationScopeCount = 3
};

class GlobalParameters {
 public:
  std::string version() const {
    return "0.4.0";
  }

  void process_update(td::td_api::updateScopeNotificationSettings &update) {
    td::uint32 scope = 0;
    td::td_api::downcast_call(
        *update.scope_,
        td::overloaded(
            [&](td::td_api::notificationSettingsScopePrivateChats &) { scope = NotificationScopePrivateChats; },
            [&](td::td_api::notificationSettingsScopeGroupChats &) { scope = NotificationScopeGroupChats; },
            [&](td::td_api::notificationSettingsScopeChannelChats &) { scope = NotificationScopeChannelChats; }));
    scope_notification_settings_[scope] = std::move(update.notification_settings_);
  }

  void process_update(td::td_api::updateReactionNotificationSettings &update) {
  }

  void process_update(td::td_api::updateChatFolders &update) {
    chat_folders_ = std::move(update.chat_folders_);
  }

  void process_update(td::td_api::updateFileDownloads &update) {
    download_list_bytes_ = update.total_size_;
    download_list_files_ = update.total_count_;
    downloaded_bytes_ = update.downloaded_size_;
  }

  void process_update(td::td_api::updateUserPrivacySettingRules &update) {
  }

  void process_update(td::td_api::updateSavedAnimations &update) {
    saved_animations_ = std::move(update.animation_ids_);
  }

  void process_update(td::td_api::updateDefaultBackground &update) {
    (update.for_dark_theme_ ? default_dark_background_ : default_light_background_) = std::move(update.background_);
  }

  void process_update(td::td_api::updateChatThemes &update) {
    chat_themes_ = std::move(update.chat_themes_);
  }

  void process_update(td::td_api::updateAccentColors &update) {
    std::map<td::int32, td::tl_object_ptr<td::td_api::accentColor>> n;
    for (auto &x : update.colors_) {
      auto id = x->id_;
      n[id] = std::move(x);
    }
    accent_colors_ = std::move(n);
    available_accent_color_ids_ = std::move(update.available_accent_color_ids_);
  }

  void process_update(td::td_api::updateProfileAccentColors &update) {
    profile_accent_colors_ = std::move(update.colors_);
    profile_available_accent_color_ids_ = std::move(update.available_accent_color_ids_);
  }

  void process_update(td::td_api::updateLanguagePackStrings &update) {
  }

  void process_update(td::td_api::updateConnectionState &update) {
    connection_state_ = std::move(update.state_);
  }

  void process_update(td::td_api::updateAttachmentMenuBots &update) {
    attachment_menu_bots_ = std::move(update.bots_);
  }

  void process_update(td::td_api::updateActiveEmojiReactions &update) {
    active_emoji_reactions_ = std::move(update.emojis_);
  }

  void process_update(td::td_api::updateAvailableMessageEffects &update) {
    avaliable_message_reaction_effect_ids_ = std::move(update.reaction_effect_ids_);
    avaliable_message_sticker_effect_ids_ = std::move(update.sticker_effect_ids_);
  }

  void process_update(td::td_api::updateDefaultReactionType &update) {
    default_reaction_type_ = std::move(update.reaction_type_);
  }

  void process_update(td::td_api::updateDefaultPaidReactionType &update) {
    default_paid_reaction_type_ = std::move(update.type_);
  }

  void process_update(td::td_api::updateOwnedStarCount &update) {
    stars_owned_ = std::move(update.star_amount_);
  }

  void process_update(td::td_api::updateDiceEmojis &update) {
    dice_emojis_ = std::move(update.emojis_);
  }

  void process_update(td::td_api::updateAnimationSearchParameters &update) {
    animation_search_provider_ = std::move(update.provider_);
    animation_search_suggested_emojis_ = std::move(update.emojis_);
  }

  void process_update(td::td_api::updateAutosaveSettings &update) {
  }

  const auto &chat_folders() const {
    return chat_folders_;
  }

  auto download_list_bytes() const {
    return download_list_bytes_;
  }
  auto download_list_files() const {
    return download_list_files_;
  }
  auto downloaded_bytes() const {
    return downloaded_bytes_;
  }

  const auto &stars_owned() const {
    return stars_owned_;
  }

  const auto &connection_state() const {
    return connection_state_;
  }

  void set_copy_command(std::string command) {
    copy_command_ = std::move(command);
  }

  const auto &copy_command() const {
    return copy_command_;
  }

  void set_link_open_command(std::string command) {
    link_open_command_ = std::move(command);
  }

  const auto &link_open_command() const {
    return link_open_command_;
  }

  void set_file_open_command(std::string command) {
    file_open_command_ = std::move(command);
  }

  const auto &file_open_command() const {
    return file_open_command_;
  }

  void update_my_user_id(td::int64 id) {
    my_user_id_ = id;
  }

  td::int64 my_user_id() const {
    return my_user_id_;
  }

  void copy_to_primary_buffer(td::CSlice text);
  void copy_to_clipboard(td::CSlice text);
  void copy_to_buffer(td::CSlice text, bool primary) {
    if (primary) {
      copy_to_primary_buffer(text);
    } else {
      copy_to_clipboard(text);
    }
  }
  void open_document(td::CSlice file_path);
  void open_link(td::CSlice url);

  void update_tdlib_version(std::string version) {
    tdlib_version_ = version;
  }
  td::CSlice tdlib_version() {
    return tdlib_version_;
  }

  void set_backend_type(std::string backend_type) {
    backend_type_ = backend_type;
  }

  td::CSlice backend_type() const {
    return backend_type_;
  }

  bool notifications_enabled() const {
    return notifications_enabled_;
  }

  void set_notifications_enabled(bool value) {
    notifications_enabled_ = value;
  }

  const auto &scope_notification_settings(NotificationScope scope) const {
    return scope_notification_settings_[(td::int32)scope];
  }

  void update_notification_sounds(td::tl_object_ptr<td::td_api::notificationSounds> s) {
    notification_sounds_.clear();
    for (auto &e : s->notification_sounds_) {
      auto id = e->id_;
      notification_sounds_[id] = std::move(e);
    }
  }

  const auto &notification_sounds() const {
    return notification_sounds_;
  }

  const td::td_api::accentColor *get_accent_color(td::int32 color_id) const {
    auto it = accent_colors_.find(color_id);
    if (it == accent_colors_.end()) {
      return nullptr;
    } else {
      return it->second.get();
    }
  }

  bool use_markdown() const {
    return use_markdown_;
  }

  void set_use_markdown(bool value) {
    use_markdown_ = value;
  }

  td::CSlice control_key() {
    return "C-a";
  }

 private:
  std::array<td::tl_object_ptr<td::td_api::scopeNotificationSettings>, NotificationScopeCount>
      scope_notification_settings_{};
  std::vector<td::tl_object_ptr<td::td_api::chatFolderInfo>> chat_folders_;
  td::int64 download_list_bytes_{0}, download_list_files_{0}, downloaded_bytes_{0};
  std::vector<td::int32> saved_animations_;
  td::tl_object_ptr<td::td_api::background> default_dark_background_;
  td::tl_object_ptr<td::td_api::background> default_light_background_;
  std::vector<td::tl_object_ptr<td::td_api::chatTheme>> chat_themes_;
  std::map<td::int32, td::tl_object_ptr<td::td_api::accentColor>> accent_colors_;
  std::vector<td::int32> available_accent_color_ids_;
  std::vector<td::tl_object_ptr<td::td_api::profileAccentColor>> profile_accent_colors_;
  std::vector<td::int32> profile_available_accent_color_ids_;
  td::tl_object_ptr<td::td_api::ConnectionState> connection_state_ =
      td::make_tl_object<td::td_api::connectionStateWaitingForNetwork>();
  std::vector<td::tl_object_ptr<td::td_api::attachmentMenuBot>> attachment_menu_bots_;
  std::vector<std::string> active_emoji_reactions_;
  std::vector<td::int64> avaliable_message_reaction_effect_ids_;
  std::vector<td::int64> avaliable_message_sticker_effect_ids_;
  td::tl_object_ptr<td::td_api::ReactionType> default_reaction_type_;
  td::tl_object_ptr<td::td_api::PaidReactionType> default_paid_reaction_type_;
  td::tl_object_ptr<td::td_api::starAmount> stars_owned_;
  std::vector<std::string> dice_emojis_;
  std::string animation_search_provider_;
  std::vector<std::string> animation_search_suggested_emojis_;
  std::map<td::int64, td::tl_object_ptr<td::td_api::notificationSound>> notification_sounds_;

  std::string copy_command_;
  std::string link_open_command_;
  std::string file_open_command_;

  td::int64 my_user_id_{0};

  bool notifications_enabled_{true};
  bool use_markdown_;

  std::string tdlib_version_;
  std::string backend_type_;
};

extern GlobalParameters &global_parameters();

};  // namespace tdcurses
