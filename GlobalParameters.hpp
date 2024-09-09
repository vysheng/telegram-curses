#pragma once

#include "auto/td/telegram/td_api.h"
#include "auto/td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/common.h"
#include "td/utils/overloaded.h"
#include "td/utils/port/Stat.h"
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
    return "0.3";
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

  void process_update(td::td_api::updateSavedNotificationSounds &update) {
    saved_notification_sounds_ = std::move(update.notification_sound_ids_);
  }

  void process_update(td::td_api::updateDefaultBackground &update) {
    (update.for_dark_theme_ ? default_dark_background_ : default_light_background_) = std::move(update.background_);
  }

  void process_update(td::td_api::updateChatThemes &update) {
    chat_themes_ = std::move(update.chat_themes_);
  }

  void process_update(td::td_api::updateAccentColors &update) {
    accent_colors_ = std::move(update.colors_);
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

  void process_update(td::td_api::updateOwnedStarCount &update) {
    stars_owned_ = update.star_count_;
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

  auto stars_owned() const {
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
  void open_document(td::CSlice file_path);
  void open_link(td::CSlice url);

 private:
  std::array<td::tl_object_ptr<td::td_api::scopeNotificationSettings>, NotificationScopeCount>
      scope_notification_settings_{};
  std::vector<td::tl_object_ptr<td::td_api::chatFolderInfo>> chat_folders_;
  td::int64 download_list_bytes_{0}, download_list_files_{0}, downloaded_bytes_{0};
  std::vector<td::int32> saved_animations_;
  std::vector<td::int64> saved_notification_sounds_;
  td::tl_object_ptr<td::td_api::background> default_dark_background_;
  td::tl_object_ptr<td::td_api::background> default_light_background_;
  std::vector<td::tl_object_ptr<td::td_api::chatTheme>> chat_themes_;
  std::vector<td::tl_object_ptr<td::td_api::accentColor>> accent_colors_;
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
  td::int64 stars_owned_{0};
  std::vector<std::string> dice_emojis_;
  std::string animation_search_provider_;
  std::vector<std::string> animation_search_suggested_emojis_;

  std::string copy_command_;
  std::string link_open_command_;
  std::string file_open_command_;

  td::int64 my_user_id_{0};
};

extern GlobalParameters &global_parameters();

};  // namespace tdcurses
