#pragma once

#include "td/tl/TlObject.h"
#include "td/utils/common.h"
#include "td/generate/auto/td/telegram/td_api.h"
#include "td/generate/auto/td/telegram/td_api.hpp"
#include "td/utils/overloaded.h"
#include <memory>
#include <map>

namespace tdcurses {

inline bool eq_source(const td::td_api::ChatList &l, const td::td_api::ChatList &r) {
  if (l.get_id() != r.get_id()) {
    return false;
  }

  bool res = false;
  td::td_api::downcast_call(
      const_cast<td::td_api::ChatList &>(l),
      td::overloaded([&](td::td_api::chatListMain &s) { res = true; },
                     [&](td::td_api::chatListArchive &s) { res = true; },
                     [&](td::td_api::chatListFolder &s) {
                       res = (s.chat_folder_id_ == static_cast<const td::td_api::chatListFolder &>(r).chat_folder_id_);
                     }));
  return res;
}

enum class ChatType { User, Channel, Supergroup, Basicgroup, SecretChat, Unknown };

class Chat {
 public:
  Chat(td::tl_object_ptr<td::td_api::chat> chat) : chat_(std::move(chat)) {
  }
  td::int64 chat_id() const {
    return chat_->id_;
  }
  ChatType chat_type() const {
    if (!chat_ || !chat_->type_) {
      return ChatType::Unknown;
    }
    ChatType r;
    td::td_api::downcast_call(const_cast<td::td_api::ChatType &>(*chat_->type_),
                              td::overloaded([&](const td::td_api::chatTypePrivate &p) { r = ChatType::User; },
                                             [&](const td::td_api::chatTypeSecret &p) { r = ChatType::SecretChat; },
                                             [&](const td::td_api::chatTypeBasicGroup &p) { r = ChatType::Basicgroup; },
                                             [&](const td::td_api::chatTypeSupergroup &p) {
                                               if (p.is_channel_) {
                                                 r = ChatType::Channel;
                                               } else {
                                                 r = ChatType::Supergroup;
                                               }
                                             }));
    return r;
  }
  td::int64 chat_base_id() const {
    if (!chat_ || !chat_->type_) {
      return 0;
    }
    td::int64 r;
    td::td_api::downcast_call(const_cast<td::td_api::ChatType &>(*chat_->type_),
                              td::overloaded([&](const td::td_api::chatTypePrivate &p) { r = p.user_id_; },
                                             [&](const td::td_api::chatTypeSecret &p) { r = p.user_id_; },
                                             [&](const td::td_api::chatTypeBasicGroup &p) { r = p.basic_group_id_; },
                                             [&](const td::td_api::chatTypeSupergroup &p) { r = p.supergroup_id_; }));
    return r;
  }

  const auto &title() const {
    return chat_->title_;
  }

  const auto &permissions() const {
    return chat_->permissions_;
  }

  const auto &positions() const {
    return chat_->permissions_;
  }

  bool has_protected_content() const {
    return chat_->has_protected_content_;
  }

  bool is_marked_unread() const {
    return chat_->is_marked_as_unread_;
  }

  bool is_blocked() const {
    return chat_->block_list_ != nullptr;
  }

  bool has_scheduled_messages() const {
    return chat_->has_scheduled_messages_;
  }

  bool can_be_deleted_only_for_self() const {
    return chat_->can_be_deleted_only_for_self_;
  }

  bool can_be_deleted_for_all_users() const {
    return chat_->can_be_deleted_for_all_users_;
  }

  bool can_be_reported() const {
    return chat_->can_be_reported_;
  }

  bool default_disable_notification() const {
    return chat_->default_disable_notification_;
  }

  auto unread_count() const {
    return chat_->unread_count_;
  }

  td::int64 last_read_inbox_message_id() const {
    return chat_->last_read_inbox_message_id_;
  }

  td::int64 last_read_outbox_message_id() const {
    return chat_->last_read_outbox_message_id_;
  }

  auto unread_message_count() const {
    return chat_->unread_mention_count_;
  }

  auto unread_reaction_count() const {
    return chat_->unread_reaction_count_;
  }

  const auto &notification_settins() const {
    return chat_->notification_settings_;
  }

  const auto &available_reactions() const {
    return chat_->available_reactions_;
  }

  auto message_ttl() const {
    return chat_->message_auto_delete_time_;
    //return chat_->message_ttl_;
  }

  const auto &theme_name() const {
    return chat_->theme_name_;
  }

  const auto &action_bar() const {
    return chat_->action_bar_;
  }

  const auto &video_chat() const {
    return chat_->video_chat_;
  }

  const auto &pending_join_requests() const {
    return chat_->pending_join_requests_;
  }

  auto reply_markup_message_id() const {
    return chat_->reply_markup_message_id_;
  }

  const auto &draft_message() const {
    return chat_->draft_message_;
  }

  const auto &client_data() const {
    return chat_->client_data_;
  }

  auto online_count() const {
    return online_;
  }

  const auto *get_order_full(const td::td_api::ChatList &l) {
    for (auto &x : chat_->positions_) {
      if (eq_source(*x->list_, l)) {
        return x.get();
      }
    }
    return (const td::td_api::chatPosition *)nullptr;
  }

  td::int64 get_order(const td::td_api::ChatList &l) {
    auto r = get_order_full(l);
    return r ? r->order_ : 0;
  }

  void full_update(td::tl_object_ptr<td::td_api::chat> chat) {
    chat_ = std::move(chat);
  }

  bool is_muted() {
    if (!chat_->notification_settings_) {
      return false;
    }
    if (chat_->notification_settings_->use_default_mute_for_) {
      return false;
    }
    return chat_->notification_settings_->mute_for_ > 0;
  }

 private:
  td::tl_object_ptr<td::td_api::chat> chat_;
  td::int32 online_{0};

 public:
  //@description The title of a chat was changed @chat_id Chat identifier @title The new chat title
  //updateChatTitle chat_id:int53 title:string = Update;
  void process_update(td::td_api::updateChatTitle &update) {
    chat_->title_ = update.title_;
  }

  //@description A chat photo was changed @chat_id Chat identifier @photo The new chat photo; may be null
  //updateChatPhoto chat_id:int53 photo:chatPhotoInfo = Update;
  void process_update(td::td_api::updateChatPhoto &update) {
    chat_->photo_ = std::move(update.photo_);
  }

  //@description Chat accent colors have changed
  //@chat_id Chat identifier
  //@accent_color_id The new chat accent color identifier
  //@background_custom_emoji_id The new identifier of a custom emoji to be shown on the reply header and link preview background; 0 if none
  //@profile_accent_color_id The new chat profile accent color identifier; -1 if none
  //@profile_background_custom_emoji_id The new identifier of a custom emoji to be shown on the profile background; 0 if none
  //updateChatAccentColors chat_id:int53 accent_color_id:int32 background_custom_emoji_id:int64 profile_accent_color_id:int32 profile_background_custom_emoji_id:int64 = Update;
  void process_update(td::td_api::updateChatAccentColors &update) {
    chat_->accent_color_id_ = update.accent_color_id_;
    chat_->background_custom_emoji_id_ = update.background_custom_emoji_id_;
    chat_->profile_accent_color_id_ = update.profile_accent_color_id_;
    chat_->profile_background_custom_emoji_id_ = update.profile_background_custom_emoji_id_;
  }

  //@description Chat permissions were changed @chat_id Chat identifier @permissions The new chat permissions
  //updateChatPermissions chat_id:int53 permissions:chatPermissions = Update;
  void process_update(td::td_api::updateChatPermissions &update) {
    chat_->permissions_ = std::move(update.permissions_);
  }

  //@description The last message of a chat was changed
  //@chat_id Chat identifier
  //@last_message The new last message in the chat; may be null if the last message became unknown. While the last message is unknown, new messages can be added to the chat without corresponding updateNewMessage update
  //@positions The new chat positions in the chat lists
  //updateChatLastMessage chat_id:int53 last_message:message positions:vector<chatPosition> = Update;
  void process_update(td::td_api::updateChatLastMessage &update) {
    chat_->last_message_ = std::move(update.last_message_);
    chat_->positions_ = std::move(update.positions_);
  }

  //@description The position of a chat in a chat list has changed. An updateChatLastMessage or updateChatDraftMessage update might be sent instead of the update
  //@chat_id Chat identifier
  //@position New chat position. If new order is 0, then the chat needs to be removed from the list
  //updateChatPosition chat_id:int53 position:chatPosition = Update;
  void process_update(td::td_api::updateChatPosition &update) {
    auto it = chat_->positions_.begin();
    for (; it != chat_->positions_.end(); it++) {
      if (eq_source(*(*it)->list_, *update.position_->list_)) {
        break;
      }
    }
    if (it != chat_->positions_.end()) {
      if (update.position_->order_ != 0) {
        (*it) = std::move(update.position_);
      } else {
        chat_->positions_.erase(it);
      }
    } else {
      if (update.position_->order_ != 0) {
        chat_->positions_.push_back(std::move(update.position_));
      }
    }
  }

  //@description A chat was added to a chat list @chat_id Chat identifier @chat_list The chat list to which the chat was added
  //updateChatAddedToList chat_id:int53 chat_list:ChatList = Update;
  void process_update(td::td_api::updateChatAddedToList &update) {
  }

  //@description A chat was removed from a chat list @chat_id Chat identifier @chat_list The chat list from which the chat was removed
  //updateChatRemovedFromList chat_id:int53 chat_list:ChatList = Update;
  void process_update(td::td_api::updateChatRemovedFromList &update) {
  }

  //@description Incoming messages were read or the number of unread messages has been changed @chat_id Chat identifier @last_read_inbox_message_id Identifier of the last read incoming message @unread_count The number of unread messages left in the chat
  //updateChatReadInbox chat_id:int53 last_read_inbox_message_id:int53 unread_count:int32 = Update;
  void process_update(td::td_api::updateChatReadInbox &update) {
    chat_->last_read_inbox_message_id_ = update.last_read_inbox_message_id_;
    chat_->unread_count_ = update.unread_count_;
  }

  //@description Outgoing messages were read @chat_id Chat identifier @last_read_outbox_message_id Identifier of last read outgoing message
  //updateChatReadOutbox chat_id:int53 last_read_outbox_message_id:int53 = Update;
  void process_update(td::td_api::updateChatReadOutbox &update) {
    chat_->last_read_outbox_message_id_ = update.last_read_outbox_message_id_;
  }

  //@description The chat action bar was changed @chat_id Chat identifier @action_bar The new value of the action bar; may be null
  //updateChatActionBar chat_id:int53 action_bar:ChatActionBar = Update;
  void process_update(td::td_api::updateChatActionBar &update) {
    chat_->action_bar_ = std::move(update.action_bar_);
  }

  //@description The bar for managing business bot was changed in a chat @chat_id Chat identifier @business_bot_manage_bar The new value of the business bot manage bar; may be null
  //updateChatBusinessBotManageBar chat_id:int53 business_bot_manage_bar:businessBotManageBar = Update;
  void process_update(td::td_api::updateChatBusinessBotManageBar &update) {
    chat_->business_bot_manage_bar_ = std::move(update.business_bot_manage_bar_);
  }

  //@description The chat available reactions were changed @chat_id Chat identifier @available_reactions The new list of reactions, available in the chat
  //updateChatAvailableReactions chat_id:int53 available_reactions:vector<string> = Update;
  void process_update(td::td_api::updateChatAvailableReactions &update) {
    chat_->available_reactions_ = std::move(update.available_reactions_);
  }

  //@description A chat draft has changed. Be aware that the update may come in the currently opened chat but with old content of the draft. If the user has changed the content of the draft, this update mustn't be applied
  //@chat_id Chat identifier
  //@draft_message The new draft message; may be null if none
  //@positions The new chat positions in the chat lists
  //updateChatDraftMessage chat_id:int53 draft_message:draftMessage positions:vector<chatPosition> = Update;
  void process_update(td::td_api::updateChatDraftMessage &update) {
    chat_->draft_message_ = std::move(update.draft_message_);
    chat_->positions_ = std::move(update.positions_);
  }

  //@description Chat emoji status has changed
  //@chat_id Chat identifier
  //@emoji_status The new chat emoji status; may be null
  //updateChatEmojiStatus chat_id:int53 emoji_status:emojiStatus = Update;
  void process_update(td::td_api::updateChatEmojiStatus &update) {
    chat_->emoji_status_ = std::move(update.emoji_status_);
  }

  //@description The message sender that is selected to send messages in a chat has changed
  //@chat_id Chat identifier
  //@message_sender_id New value of message_sender_id; may be null if the user can't change message sender
  //updateChatMessageSender chat_id:int53 message_sender_id:MessageSender = Update;
  void process_update(td::td_api::updateChatMessageSender &update) {
    chat_->message_sender_id_ = std::move(update.message_sender_id_);
  }

  //@description The message auto-delete or self-destruct timer setting for a chat was changed
  //@chat_id Chat identifier
  //@message_auto_delete_time New value of message_auto_delete_time
  //updateChatMessageAutoDeleteTime chat_id:int53 message_auto_delete_time:int32 = Update;
  void process_update(td::td_api::updateChatMessageAutoDeleteTime &update) {
    chat_->message_auto_delete_time_ = update.message_auto_delete_time_;
  }

  //@description Notification settings for a chat were changed
  //@chat_id Chat identifier
  //@notification_settings The new notification settings
  //updateChatNotificationSettings chat_id:int53 notification_settings:chatNotificationSettings = Update;
  void process_update(td::td_api::updateChatNotificationSettings &update) {
    chat_->notification_settings_ = std::move(update.notification_settings_);
  }

  //@description The chat pending join requests were changed
  //@chat_id Chat identifier
  //@pending_join_requests The new data about pending join requests; may be null
  //updateChatPendingJoinRequests chat_id:int53 pending_join_requests:chatJoinRequestsInfo = Update;
  void process_update(td::td_api::updateChatPendingJoinRequests &update) {
    chat_->pending_join_requests_ = std::move(update.pending_join_requests_);
  }

  //@description The default chat reply markup was changed. Can occur because new messages with reply markup were received or because an old reply markup was hidden by the user
  //@chat_id Chat identifier
  //@reply_markup_message_id Identifier of the message from which reply markup needs to be used; 0 if there is no default custom reply markup in the chat
  //updateChatReplyMarkup chat_id:int53 reply_markup_message_id:int53 = Update;
  void process_update(td::td_api::updateChatReplyMarkup &update) {
    chat_->reply_markup_message_id_ = update.reply_markup_message_id_;
  }

  //@description The chat background was changed
  //@chat_id Chat identifier
  //@background The new chat background; may be null if background was reset to default
  //updateChatBackground chat_id:int53 background:chatBackground = Update;
  void process_update(td::td_api::updateChatBackground &update) {
    chat_->background_ = std::move(update.background_);
  }

  //@description The chat theme was changed
  //@chat_id Chat identifier
  //@theme_name The new name of the chat theme; may be empty if theme was reset to default
  //updateChatTheme chat_id:int53 theme_name:string = Update;
  void process_update(td::td_api::updateChatTheme &update) {
    chat_->theme_name_ = std::move(update.theme_name_);
  }

  //@description The chat unread_mention_count has changed
  //@chat_id Chat identifier
  //@unread_mention_count The number of unread mention messages left in the chat
  //updateChatUnreadMentionCount chat_id:int53 unread_mention_count:int32 = Update;
  void process_update(td::td_api::updateChatUnreadMentionCount &update) {
    chat_->unread_mention_count_ = std::move(update.unread_mention_count_);
  }

  //@description The chat unread_reaction_count has changed
  //@chat_id Chat identifier
  //@unread_reaction_count The number of messages with unread reactions left in the chat
  //updateChatUnreadReactionCount chat_id:int53 unread_reaction_count:int32 = Update;
  void process_update(td::td_api::updateChatUnreadReactionCount &update) {
    chat_->unread_reaction_count_ = std::move(update.unread_reaction_count_);
  }

  //@description A chat video chat state has changed
  //@chat_id Chat identifier
  //@video_chat New value of video_chat
  //updateChatVideoChat chat_id:int53 video_chat:videoChat = Update;
  void process_update(td::td_api::updateChatVideoChat &update) {
    chat_->video_chat_ = std::move(update.video_chat_);
  }

  //@description The value of the default disable_notification parameter, used when a message is sent to the chat, was changed
  //@chat_id Chat identifier
  //@default_disable_notification The new default_disable_notification value
  //updateChatDefaultDisableNotification chat_id:int53 default_disable_notification:Bool = Update;
  void process_update(td::td_api::updateChatDefaultDisableNotification &update) {
    chat_->default_disable_notification_ = std::move(update.default_disable_notification_);
  }

  //@description A chat content was allowed or restricted for saving
  //@chat_id Chat identifier
  //@has_protected_content New value of has_protected_content
  //updateChatHasProtectedContent chat_id:int53 has_protected_content:Bool = Update;
  void process_update(td::td_api::updateChatHasProtectedContent &update) {
    chat_->has_protected_content_ = std::move(update.has_protected_content_);
  }

  //@description Translation of chat messages was enabled or disabled
  //@chat_id Chat identifier
  //@is_translatable New value of is_translatable
  //updateChatIsTranslatable chat_id:int53 is_translatable:Bool = Update;
  void process_update(td::td_api::updateChatIsTranslatable &update) {
    chat_->is_translatable_ = update.is_translatable_;
  }

  //@description A chat was marked as unread or was read
  //@chat_id Chat identifier
  //@is_marked_as_unread New value of is_marked_as_unread
  //updateChatIsMarkedAsUnread chat_id:int53 is_marked_as_unread:Bool = Update;
  void process_update(td::td_api::updateChatIsMarkedAsUnread &update) {
    chat_->is_marked_as_unread_ = std::move(update.is_marked_as_unread_);
  }

  //@description A chat default appearance has changed
  //@chat_id Chat identifier
  //@view_as_topics New value of view_as_topics
  //updateChatViewAsTopics chat_id:int53 view_as_topics:Bool = Update;
  void process_update(td::td_api::updateChatViewAsTopics &update) {
    chat_->view_as_topics_ = std::move(update.view_as_topics_);
  }

  //@description A chat was blocked or unblocked
  //@chat_id Chat identifier
  //@block_list Block list to which the chat is added; may be null if none
  //updateChatBlockList chat_id:int53 block_list:BlockList = Update;
  void process_update(td::td_api::updateChatBlockList &update) {
    chat_->block_list_ = std::move(update.block_list_);
  }

  //@description A chat's has_scheduled_messages field has changed
  //@chat_id Chat identifier
  //@has_scheduled_messages New value of has_scheduled_messages
  //updateChatHasScheduledMessages chat_id:int53 has_scheduled_messages:Bool = Update;
  void process_update(td::td_api::updateChatHasScheduledMessages &update) {
    chat_->has_scheduled_messages_ = std::move(update.has_scheduled_messages_);
  }

  //@description The list of chat folders or a chat folder has changed
  //@chat_folders The new list of chat folders
  //@main_chat_list_position Position of the main chat list among chat folders, 0-based
  //@are_tags_enabled True, if folder tags are enabled
  //updateChatFolders chat_folders:vector<chatFolderInfo> main_chat_list_position:int32 are_tags_enabled:Bool = Update;
  void process_update(td::td_api::updateChatFolders &update) {
  }

  //@description The number of online group members has changed. This update with non-zero number of online group members is sent only for currently opened chats.
  //-There is no guarantee that it is sent just after the number of online users has changed
  //@chat_id Identifier of the chat
  //@online_member_count New number of online members in the chat, or 0 if unknown
  //updateChatOnlineMemberCount chat_id:int53 online_member_count:int32 = Update;
  void process_update(td::td_api::updateChatOnlineMemberCount &update) {
    online_ = update.online_member_count_;
  }

  //@description Basic information about a topic in a forum chat was changed @chat_id Chat identifier @info New information about the topic
  void process_update(td::td_api::updateForumTopicInfo &update) {
  }

  //@description The revenue earned from sponsored messages in a chat has changed. If chat revenue screen is opened, then getChatRevenueTransactions may be called to fetch new transactions
  //@chat_id Identifier of the chat
  //@revenue_amount New amount of earned revenue
  //updateChatRevenueAmount chat_id:int53 revenue_amount:chatRevenueAmount = Update;
  void process_update(td::td_api::updateChatRevenueAmount &update) {
  }
};

class User {
 public:
  User(td::tl_object_ptr<td::td_api::user> user) : user_(std::move(user)) {
  }

  auto user_id() const {
    return user_->id_;
  }
  const auto &first_name() const {
    return user_->first_name_;
  }
  const auto &last_name() const {
    return user_->last_name_;
  }
  const std::string &username() const {
    if (!user_->usernames_) {
      static const std::string empty;
      return empty;
    }
    return user_->usernames_->editable_username_;
    //return user_->username_;
  }
  const auto &phone_number() const {
    return user_->phone_number_;
  }
  const auto &status() const {
    return user_->status_;
  }
  const auto &photo() const {
    return user_->profile_photo_;
  }
  auto is_contact() const {
    return user_->is_contact_;
  }
  auto is_mutual_contact() const {
    return user_->is_mutual_contact_;
  }
  auto is_verified() const {
    return user_->is_verified_;
  }
  auto is_support() const {
    return user_->is_support_;
  }
  const auto &restriction_reason() const {
    return user_->restriction_reason_;
  }
  auto is_scam() const {
    return user_->is_scam_;
  }
  auto is_fake() const {
    return user_->is_fake_;
  }
  auto have_access() const {
    return user_->have_access_;
  }
  const auto &type() const {
    return user_->type_;
  }
  const auto &language_code() const {
    return user_->language_code_;
  }

  void full_update(td::tl_object_ptr<td::td_api::user> user) {
    user_ = std::move(user);
  }

  void process_update(td::td_api::updateUserStatus &upd) {
    user_->status_ = std::move(upd.status_);
  }

  const auto &emoji_status() const {
    return user_->emoji_status_;
  }

 private:
  //user id:int53 first_name:string last_name:string username:string phone_number:string status:UserStatus profile_photo:profilePhoto is_contact:Bool is_mutual_contact:Bool is_verified:Bool is_support:Bool restriction_reason:string is_scam:Bool is_fake:Bool have_access:Bool type:UserType language_code:string = User;
  td::tl_object_ptr<td::td_api::user> user_;
};

class BasicGroup {
 public:
  BasicGroup(td::tl_object_ptr<td::td_api::basicGroup> group) : group_(std::move(group)) {
  }

  //basicGroup id:int53 member_count:int32 status:ChatMemberStatus is_active:Bool upgraded_to_supergroup_id:int53 = BasicGroup;
  auto group_id() const {
    return group_->id_;
  }
  auto member_count() const {
    return group_->member_count_;
  }
  const auto &status() const {
    return group_->status_;
  }
  bool is_active() const {
    return group_->is_active_;
  }
  auto upgraded_to_supergroup_id() const {
    return group_->upgraded_to_supergroup_id_;
  }
  void full_update(td::tl_object_ptr<td::td_api::basicGroup> group) {
    group_ = std::move(group);
  }

 private:
  td::tl_object_ptr<td::td_api::basicGroup> group_;
};

class Supergroup {
 public:
  Supergroup(td::tl_object_ptr<td::td_api::supergroup> group) : group_(std::move(group)) {
  }

  //basicGroup id:int53 member_count:int32 status:ChatMemberStatus is_active:Bool upgraded_to_supergroup_id:int53 = BasicGroup;
  auto group_id() const {
    return group_->id_;
  }
  const std::string &username() const {
    if (!group_->usernames_) {
      static const std::string empty;
      return empty;
    }
    return group_->usernames_->editable_username_;
    //return user_->username_;
  }
  auto date() const {
    return group_->date_;
  }
  const auto &status() const {
    return group_->status_;
  }
  auto member_count() const {
    return group_->member_count_;
  }
  auto boost_leven() const {
    return group_->boost_level_;
  }
  auto has_location() const {
    return group_->has_location_;
  }
  auto has_linked_chat() const {
    return group_->has_linked_chat_;
  }
  auto sign_messages() const {
    return group_->sign_messages_;
  }
  auto join_to_send_messages() const {
    return group_->join_to_send_messages_;
  }
  auto join_by_request() const {
    return group_->join_by_request_;
  }
  auto is_slow_mode_enabled() const {
    return group_->is_slow_mode_enabled_;
  }
  bool is_channel() const {
    return group_->is_channel_;
  }
  auto is_broadcast_group() const {
    return group_->is_broadcast_group_;
  }
  auto is_forum() const {
    return group_->is_forum_;
  }
  auto is_verified() const {
    return group_->is_verified_;
  }
  const auto &restriction_reason() const {
    return group_->restriction_reason_;
  }
  auto is_scam() const {
    return group_->is_scam_;
  }
  auto is_fake() const {
    return group_->is_fake_;
  }
  auto has_active_stories() const {
    return group_->has_active_stories_;
  }
  auto has_unread_active_stories() const {
    return group_->has_unread_active_stories_;
  }
  bool has_sensitive_content() const {
    return group_->has_sensitive_content_;
  }

  void full_update(td::tl_object_ptr<td::td_api::supergroup> group) {
    group_ = std::move(group);
  }

 private:
  td::tl_object_ptr<td::td_api::supergroup> group_;
};

class SecretChat {
 public:
  SecretChat(td::tl_object_ptr<td::td_api::secretChat> group) : group_(std::move(group)) {
  }

  //basicGroup id:int53 member_count:int32 status:ChatMemberStatus is_active:Bool upgraded_to_supergroup_id:int53 = BasicGroup;
  auto user_id() const {
    return group_->user_id_;
  }
  void full_update(td::tl_object_ptr<td::td_api::secretChat> group) {
    group_ = std::move(group);
  }

 private:
  td::tl_object_ptr<td::td_api::secretChat> group_;
};

class ChatManager {
 public:
  virtual ~ChatManager() = default;

  ChatManager() {
  }

  void add_chat(std::shared_ptr<Chat> chat) {
    auto chat_id = chat->chat_id();
    CHECK(chats_.emplace(chat_id, std::move(chat)).second);
  }
  std::shared_ptr<Chat> get_chat(td::int64 chat_id) {
    auto it = chats_.find(chat_id);
    if (it != chats_.end()) {
      return it->second;
    } else {
      return nullptr;
    }
  }
  std::shared_ptr<User> get_user(td::int64 user_id) {
    auto it = users_.find(user_id);
    if (it != users_.end()) {
      return it->second;
    } else {
      return nullptr;
    }
  }
  std::shared_ptr<BasicGroup> get_basic_group(td::int64 basic_group_id) {
    auto it = basic_groups_.find(basic_group_id);
    if (it != basic_groups_.end()) {
      return it->second;
    } else {
      return nullptr;
    }
  }
  std::shared_ptr<Supergroup> get_supergroup(td::int64 supergroup_id) {
    auto it = supergroups_.find(supergroup_id);
    if (it != supergroups_.end()) {
      return it->second->is_channel() ? nullptr : it->second;
    } else {
      return nullptr;
    }
  }
  std::shared_ptr<Supergroup> get_channel(td::int64 channel_id) {
    auto it = supergroups_.find(channel_id);
    if (it != supergroups_.end()) {
      return it->second->is_channel() ? it->second : nullptr;
    } else {
      return nullptr;
    }
  }

  template <typename T>
  void process_update(T &upd) {
    auto chat = get_chat(upd.chat_id_);
    if (chat) {
      chat->process_update(upd);
    }
  }

  void process_update(td::td_api::updateUser &upd) {
    auto it = users_.find(upd.user_->id_);
    if (it != users_.end()) {
      it->second->full_update(std::move(upd.user_));
    } else {
      auto user = std::make_shared<User>(std::move(upd.user_));
      users_.emplace(user->user_id(), std::move(user));
    }
  }
  void process_update(td::td_api::updateUserStatus &upd) {
    auto u = get_user(upd.user_id_);
    if (u) {
      u->process_update(upd);
    }
  }
  void process_update(td::td_api::updateBasicGroup &upd) {
    auto it = basic_groups_.find(upd.basic_group_->id_);
    if (it != basic_groups_.end()) {
      it->second->full_update(std::move(upd.basic_group_));
    } else {
      auto group = std::make_shared<BasicGroup>(std::move(upd.basic_group_));
      basic_groups_.emplace(group->group_id(), std::move(group));
    }
  }
  void process_update(td::td_api::updateSupergroup &upd) {
    auto it = supergroups_.find(upd.supergroup_->id_);
    if (it != supergroups_.end()) {
      it->second->full_update(std::move(upd.supergroup_));
    } else {
      auto supergroup = std::make_shared<Supergroup>(std::move(upd.supergroup_));
      supergroups_.emplace(supergroup->group_id(), std::move(supergroup));
    }
  }
  void process_update(td::td_api::updateSecretChat &upd) {
  }
  void process_update(td::td_api::updateUserFullInfo &upd) {
  }
  void process_update(td::td_api::updateBasicGroupFullInfo &upd) {
  }
  void process_update(td::td_api::updateSupergroupFullInfo &upd) {
  }

  template <typename T>
  void iterate_all_chats(T &&cb) {
    for (auto &c : chats_) {
      cb(c.second);
    }
  }

 private:
  std::map<td::int64, std::shared_ptr<Chat>> chats_;
  std::map<td::int64, std::shared_ptr<User>> users_;
  std::map<td::int64, std::shared_ptr<BasicGroup>> basic_groups_;
  std::map<td::int64, std::shared_ptr<Supergroup>> supergroups_;
};

ChatManager &chat_manager();

}  // namespace tdcurses
