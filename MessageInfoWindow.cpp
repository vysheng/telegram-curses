#include "MessageInfoWindow.hpp"
#include "ReactionSelectionWindow.hpp"
#include "ChatManager.hpp"
#include "ChatWindow.hpp"
#include "ChatInfoWindow.hpp"
#include "ChatSearchWindow.hpp"
#include "GlobalParameters.hpp"
#include "FileManager.hpp"
#include "TdObjectsOutput.h"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/Markup.hpp"
#include "windows/unicode.h"
#include "YesNoWindow.hpp"
#include "LoadingWindow.hpp"
#include <utility>
#include <vector>

namespace tdcurses {

MessageInfoWindow::~MessageInfoWindow() {
  for (auto x : subscription_ids_) {
    file_manager().unsubscribe_from_file_updates(x.first, x.second.first);
  }
}

void MessageInfoWindow::process_message() {
  add_action_forward(message_->chat_id_, message_->id_);
  add_action_reply(message_->chat_id_, message_->id_);
  add_action_reactions(message_->chat_id_, message_->id_);
  add_action_message_goto(message_->chat_id_, message_->id_, message_.get());

  if (message_->is_outgoing_) {
    add_action_chat_info(message_->chat_id_);
  } else {
    if (message_->sender_id_) {
      td::td_api::downcast_call(
          *message_->sender_id_,
          td::overloaded([&](td::td_api::messageSenderUser &sender) { add_action_user_chat_info(sender.user_id_); },
                         [&](td::td_api::messageSenderChat &sender) { add_action_chat_info(sender.chat_id_); }));
    }
  }

  if (message_->reply_to_) {
    td::td_api::downcast_call(*message_->reply_to_, td::overloaded(
                                                        [&](td::td_api::messageReplyToMessage &info) {
                                                          auto chat_window = root()->chat_window();
                                                          const td::td_api::message *msg = nullptr;
                                                          if (chat_window) {
                                                            msg = chat_window->get_message_as_message(info.chat_id_,
                                                                                                      info.message_id_);
                                                          }
                                                          add_action_message_goto(info.chat_id_, info.message_id_, msg);
                                                        },
                                                        [&](td::td_api::messageReplyToStory &info) {}));
  }

  if (message_->forward_info_ && message_->forward_info_->origin_) {
    td::td_api::downcast_call(
        *message_->forward_info_->origin_,
        td::overloaded(
            [&](td::td_api::messageOriginUser &sender) { add_action_user_chat_info(sender.sender_user_id_); },
            [&](td::td_api::messageOriginChat &sender) { add_action_chat_info(sender.sender_chat_id_); },
            [&](td::td_api::messageOriginChannel &sender) { add_action_chat_info(sender.chat_id_); },
            [&](td::td_api::messageOriginHiddenUser &sender) {}));
  }

  if (message_->via_bot_user_id_) {
    add_action_user_chat_info(message_->via_bot_user_id_);
  }

  auto process_formatted_text = [&](const td::td_api::formattedText &content) {
    auto &text = content.text_;

    if (text.size() > 0) {
      add_action_copy_primary(text);
      add_action_copy(text);
    }

    for (auto &ent : content.entities_) {
      td::td_api::downcast_call(
          const_cast<td::td_api::TextEntityType &>(*ent->type_),
          td::overloaded(
              [&](const td::td_api::textEntityTypeMention &e) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                add_action_chat_info_by_username(us.str());
              },
              [&](const td::td_api::textEntityTypeHashtag &e) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                add_action_search_pattern(us.str());
              },
              [&](const td::td_api::textEntityTypeCashtag &) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                add_action_search_pattern(us.str());
              },
              [&](const td::td_api::textEntityTypeBotCommand &) {},
              [&](const td::td_api::textEntityTypeUrl &) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                add_action_link(us.str());
              },
              [&](const td::td_api::textEntityTypeEmailAddress &) {},
              [&](const td::td_api::textEntityTypePhoneNumber &) {},
              [&](const td::td_api::textEntityTypeBankCardNumber &) {}, [&](const td::td_api::textEntityTypeBold &) {},
              [&](const td::td_api::textEntityTypeItalic &) {},
              [&](const td::td_api::textEntityTypeUnderline &) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                add_action_search_pattern(us.str());
              },
              [&](const td::td_api::textEntityTypeStrikethrough &) {},
              [&](const td::td_api::textEntityTypeSpoiler &) {}, [&](const td::td_api::textEntityTypeCode &) {},
              [&](const td::td_api::textEntityTypePre &) {}, [&](const td::td_api::textEntityTypePreCode &) {},
              [&](const td::td_api::textEntityTypeBlockQuote &e) {},
              [&](const td::td_api::textEntityTypeExpandableBlockQuote &) {},
              [&](const td::td_api::textEntityTypeTextUrl &e) { add_action_link(e.url_); },
              [&](const td::td_api::textEntityTypeMentionName &e) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                add_action_user_chat_info(e.user_id_, us.str());
              },
              [&](const td::td_api::textEntityTypeCustomEmoji &) {},
              [&](const td::td_api::textEntityTypeMediaTimestamp &) {}));
    }
  };

  if (message_->content_) {
    td::td_api::downcast_call(
        *message_->content_,
        td::overloaded(
            [&](const td::td_api::messageText &content) { process_formatted_text(*content.text_); },
            [&](const td::td_api::messageAnimation &content) {
              process_formatted_text(*content.caption_);
              add_action_open_file(*content.animation_->animation_);
            },
            [&](const td::td_api::messageAudio &content) {
              process_formatted_text(*content.caption_);
              add_action_open_file(*content.audio_->audio_);
            },
            [&](const td::td_api::messageDocument &content) {
              process_formatted_text(*content.caption_);
              add_action_open_file(*content.document_->document_);
            },
            [&](const td::td_api::messagePaidMedia &content) { process_formatted_text(*content.caption_); },
            [&](const td::td_api::messagePhoto &content) {
              process_formatted_text(*content.caption_);
              add_action_open_file(*content.photo_->sizes_.back()->photo_);
            },
            [&](const td::td_api::messageSticker &content) {},
            [&](const td::td_api::messageVideo &content) {
              process_formatted_text(*content.caption_);
              add_action_open_file(*content.video_->video_);
            },
            [&](const td::td_api::messageVideoNote &content) { add_action_open_file(*content.video_note_->video_); },
            [&](const td::td_api::messageVoiceNote &content) { add_action_open_file(*content.voice_note_->voice_); },
            [&](const td::td_api::messageExpiredPhoto &content) {},
            [&](const td::td_api::messageExpiredVideo &content) {},
            [&](const td::td_api::messageExpiredVideoNote &content) {},
            [&](const td::td_api::messageExpiredVoiceNote &content) {},
            [&](const td::td_api::messageLocation &content) {}, [&](const td::td_api::messageVenue &content) {},
            [&](const td::td_api::messageContact &content) {}, [&](const td::td_api::messageAnimatedEmoji &content) {},
            [&](const td::td_api::messageDice &content) {}, [&](const td::td_api::messageGame &content) {},
            [&](const td::td_api::messagePoll &content) {
              td::int32 idx = 0;
              for (const auto &opt : content.poll_->options_) {
                add_action_poll(message_->chat_id_, message_->id_, *opt->text_,
                                opt->is_chosen_ || opt->is_being_chosen_, idx++);
              }
            },
            [&](const td::td_api::messageStory &content) {}, [&](const td::td_api::messageInvoice &content) {},
            [&](const td::td_api::messageCall &content) {}, [&](const td::td_api::messageGroupCall &content) {},
            [&](const td::td_api::messageVideoChatScheduled &content) {},
            [&](const td::td_api::messageVideoChatStarted &content) {},
            [&](const td::td_api::messageVideoChatEnded &content) {},
            [&](const td::td_api::messageInviteVideoChatParticipants &content) {},
            [&](const td::td_api::messageBasicGroupChatCreate &content) {},
            [&](const td::td_api::messageSupergroupChatCreate &content) {},
            [&](const td::td_api::messageChatChangeTitle &content) {},
            [&](const td::td_api::messageChatChangePhoto &content) {},
            [&](const td::td_api::messageChatDeletePhoto &content) {},
            [&](const td::td_api::messageChatAddMembers &content) {
              for (auto x : content.member_user_ids_) {
                add_action_user_chat_info(x);
              }
            },
            [&](const td::td_api::messageChatJoinByLink &content) {},
            [&](const td::td_api::messageChatJoinByRequest &content) {},
            [&](const td::td_api::messageChatDeleteMember &content) { add_action_user_chat_info(content.user_id_); },
            [&](const td::td_api::messageChatUpgradeTo &content) {},
            [&](const td::td_api::messageChatUpgradeFrom &content) {},
            [&](const td::td_api::messagePinMessage &content) {
              auto chat_window = root()->chat_window();
              const td::td_api::message *msg = nullptr;
              if (chat_window) {
                msg = chat_window->get_message_as_message(chat_id_, content.message_id_);
              }
              add_action_message_goto(chat_id_, content.message_id_, msg);
            },
            [&](const td::td_api::messageScreenshotTaken &content) {},
            [&](const td::td_api::messageChatSetBackground &content) {},
            [&](const td::td_api::messageChatSetTheme &content) {},
            [&](const td::td_api::messageChatSetMessageAutoDeleteTime &content) {},
            [&](const td::td_api::messageChatBoost &content) {},
            [&](const td::td_api::messageForumTopicCreated &content) {},
            [&](const td::td_api::messageForumTopicEdited &content) {},
            [&](const td::td_api::messageForumTopicIsClosedToggled &content) {},
            [&](const td::td_api::messageForumTopicIsHiddenToggled &content) {},
            [&](const td::td_api::messageSuggestProfilePhoto &content) {},
            [&](const td::td_api::messageCustomServiceAction &content) {},
            [&](const td::td_api::messageGameScore &content) {},
            [&](const td::td_api::messagePaymentSuccessful &content) {},
            [&](const td::td_api::messagePaymentSuccessfulBot &content) {},
            [&](const td::td_api::messagePaymentRefunded &content) {},
            [&](const td::td_api::messageGiftedPremium &content) {},
            [&](const td::td_api::messagePremiumGiftCode &content) {},
            [&](const td::td_api::messageGiveawayCreated &content) {},
            [&](const td::td_api::messageGiveaway &content) {},
            [&](const td::td_api::messageGiveawayCompleted &content) {},
            [&](const td::td_api::messageGiveawayWinners &content) {},
            [&](const td::td_api::messageGiveawayPrizeStars &content) {},
            [&](const td::td_api::messageGift &content) {}, [&](const td::td_api::messageUpgradedGift &content) {},
            [&](const td::td_api::messageRefundedUpgradedGift &content) {},
            [&](const td::td_api::messagePaidMessagesRefunded &content) {},
            [&](const td::td_api::messagePaidMessagePriceChanged &content) {},
            [&](const td::td_api::messageGiftedStars &content) {},
            [&](const td::td_api::messageContactRegistered &content) {},
            [&](const td::td_api::messageUsersShared &content) {}, [&](const td::td_api::messageChatShared &content) {},
            [&](const td::td_api::messageBotWriteAccessAllowed &content) {},
            [&](const td::td_api::messageWebAppDataSent &content) {},
            [&](const td::td_api::messageWebAppDataReceived &content) {},
            [&](const td::td_api::messagePassportDataSent &content) {},
            [&](const td::td_api::messagePassportDataReceived &content) {},
            [&](const td::td_api::messageProximityAlertTriggered &content) {},
            [&](const td::td_api::messageUnsupported &content) {}));
  }

  add_action_delete_message(message_->chat_id_, message_->id_);
  if (message_properties_->can_get_message_thread_ && message_thread_info_) {
    add_action_view_thread(message_thread_info_->chat_id_, message_thread_info_->message_thread_id_,
                           message_thread_info_->message_thread_id_);
  }

  if (message_properties_->can_recognize_speech_) {
    add_action_recognize_speech(message_->chat_id_, message_->id_);
  }

  set_need_refresh();
}

void MessageInfoWindow::add_action_user_chat_info(td::int64 user_id, td::Slice custom_user_name) {
  auto user = chat_manager().get_user(user_id);
  if (!user) {
    return;
  }

  Outputter out;
  out << Color::Red;
  if (custom_user_name.size()) {
    out << custom_user_name;
  } else {
    out << user;
  }
  out << Color::Revert;
  add_element("info", out.as_str(), out.markup(), create_menu_window_spawn_function<ChatInfoWindow>(user));
}

void MessageInfoWindow::add_action_user_chat_open(td::int64 user_id, td::Slice custom_user_name) {
  auto user = chat_manager().get_user(user_id);
  if (!user) {
    return;
  }

  Outputter out;
  out << Color::Red;
  if (custom_user_name.size()) {
    out << custom_user_name;
  } else {
    out << user;
  }
  out << Color::Revert;
  add_element("open", out.as_str(), out.markup(), [self = this, user]() {
    auto req = td::make_tl_object<td::td_api::createPrivateChat>(user->user_id(), true);
    self->send_request(std::move(req), [self](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
      DROP_IF_DELETED(R);
      if (R.is_ok()) {
        auto chat = R.move_as_ok();
        self->root()->open_chat(chat->id_);
      }
      self->exit();
    });
    return true;
  });
}

void MessageInfoWindow::add_action_chat_info(td::int64 chat_id, td::Slice custom_chat_name) {
  auto chat = chat_manager().get_chat(chat_id);
  if (!chat) {
    return;
  }

  Outputter out;
  out << Color::Red;
  if (custom_chat_name.size()) {
    out << custom_chat_name;
  } else {
    out << chat;
  }
  out << Color::Revert;
  add_element("info", out.as_str(), out.markup(), create_menu_window_spawn_function<ChatInfoWindow>(chat));
}

void MessageInfoWindow::add_action_chat_open(td::int64 chat_id, td::Slice custom_chat_name) {
  auto chat = chat_manager().get_chat(chat_id);
  if (!chat) {
    return;
  }

  Outputter out;
  out << Color::Red;
  if (custom_chat_name.size()) {
    out << custom_chat_name;
  } else {
    out << chat;
  }
  out << Color::Revert;
  add_element("open", out.as_str(), out.markup(), [self = this, chat]() {
    self->root()->open_chat(chat->chat_id());
    return true;
  });
}

void MessageInfoWindow::add_action_chat_info_by_username(std::string username) {
  Outputter out;
  out << Color::Red << username << Color::Revert;
  add_element("info", out.as_str(), out.markup(), [self = this, username]() {
    auto req = td::make_tl_object<td::td_api::searchPublicChat>(username);
    self->send_request(std::move(req), [self](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
      DROP_IF_DELETED(R);
      if (R.is_ok()) {
        auto chat_tl = R.move_as_ok();
        auto chat = chat_manager().get_chat(chat_tl->id_);
        self->spawn_submenu<ChatInfoWindow>(chat);
      } else {
        self->exit();
      }
    });
    return false;
  });
}

void MessageInfoWindow::add_action_chat_open_by_username(std::string username) {
  Outputter out;
  out << Color::Red << username << Color::Revert;
  add_element("open", out.as_str(), out.markup(), [self = this, username]() {
    auto req = td::make_tl_object<td::td_api::searchPublicChat>(username);
    self->send_request(std::move(req), [self](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
      DROP_IF_DELETED(R);
      if (R.is_ok()) {
        auto chat = R.move_as_ok();
        self->root()->open_chat(chat->id_);
      }
      self->exit();
    });
    return false;
  });
}

void MessageInfoWindow::add_action_link(std::string link) {
  Outputter out;
  out << "'" << Outputter::Underline(true) << link << Outputter::Underline(false) << "'";
  add_element("open", out.as_str(), out.markup(), [link]() {
    global_parameters().open_link(link);
    return true;
  });
}

void MessageInfoWindow::add_action_search_pattern(std::string pattern) {
  Outputter out;
  out << Outputter::NoLb{true} << "'" << Color::Navy << pattern << Color::Revert << "'" << Outputter::NoLb{false};
  add_element("search", out.as_str(), out.markup(), [pattern, self = this]() {
    auto chat_window = self->root()->chat_window();
    if (chat_window) {
      chat_window->set_search_pattern(pattern);
    }
    return true;
  });
}

void MessageInfoWindow::add_action_message_goto(td::int64 chat_id, td::int64 message_id,
                                                const td::td_api::message *message) {
  Outputter out;
  out << Outputter::NoLb{true} << Color::Navy << "message" << Color::Revert;
  if (message) {
    out << " " << *message->content_;
  }
  out << Outputter::NoLb{false};
  auto message_full_id = ChatWindow::build_message_id(chat_id, message_id);
  add_element("goto", out.as_str(), out.markup(), [self = this, message_full_id]() {
    auto chat = self->root()->chat_window();
    if (chat && chat->main_chat_id() == message_full_id.chat_id) {
      chat->seek(message_full_id);
    } else {
      self->root()->open_chat(message_full_id.chat_id);
      self->root()->chat_window()->seek(message_full_id);
    }
    return true;
  });
}

void MessageInfoWindow::add_action_poll(td::int64 chat_id, td::int64 message_id, const td::td_api::formattedText &text,
                                        bool is_selected, td::int32 idx) {
  Outputter out;
  out << text;
  add_element(is_selected ? "unvote" : "vote", out.as_str(), out.markup(),
              [idx, is_selected, chat_id, message_id, self = this]() {
                td::tl_object_ptr<td::td_api::setPollAnswer> req;
                if (is_selected) {
                  req = td::make_tl_object<td::td_api::setPollAnswer>(chat_id, message_id, std::vector<td::int32>());
                } else {
                  req = td::make_tl_object<td::td_api::setPollAnswer>(chat_id, message_id, std::vector<td::int32>{idx});
                }
                self->send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                  DROP_IF_DELETED(R);
                  self->exit();
                });
                return true;
              });
}

void MessageInfoWindow::add_action_forward(td::int64 chat_id, td::int64 message_id) {
  add_element("forward", "this message", {}, [chat_id, message_id, self = this]() {
    spawn_chat_search_window(
        *self, ChatSearchWindow::Mode::Local, [chat_id, message_id, self](td::Result<std::shared_ptr<Chat>> R) {
          if (R.is_error()) {
            return;
          }
          auto dst = R.move_as_ok();

          //sendMessage chat_id:int53 message_thread_id:int53 reply_to:InputMessageReplyTo options:messageSendOptions reply_markup:ReplyMarkup input_message_content:InputMessageContent = Message;
          //inputMessageForwarded from_chat_id:int53 message_id:int53 in_game_share:Bool replace_video_start_timestamp:Bool new_video_start_timestamp:int32 copy_options:messageCopyOptions = InputMessageContent;
          auto req = td::make_tl_object<td::td_api::sendMessage>(
              dst->chat_id(), 0, nullptr /*replay_to*/, nullptr /*options*/, nullptr /*reply_markup*/,
              td::make_tl_object<td::td_api::inputMessageForwarded>(chat_id, message_id, false, false, 0, nullptr));
          self->send_request(std::move(req),
                             [self, chat_id = dst->chat_id()](td::Result<td::tl_object_ptr<td::td_api::message>> R) {
                               DROP_IF_DELETED(R);
                               self->root()->open_chat(chat_id);
                               self->exit();
                             });
        });
    return false;
  });
}

void MessageInfoWindow::add_action_copy(std::string text) {
  add_element("copy", "to clipboard", {}, [text = std::move(text)]() {
    global_parameters().copy_to_clipboard(text);
    return false;
  });
}

void MessageInfoWindow::add_action_copy_primary(std::string text) {
  add_element("copy", "to primary buffer", {}, [text = std::move(text)]() {
    global_parameters().copy_to_primary_buffer(text);
    return false;
  });
}

void MessageInfoWindow::add_action_open_file(std::string file_path) {
  open_file_el_ = add_element("open", "document", {}, [file_path = std::move(file_path)]() {
    global_parameters().open_document(file_path);
    return true;
  });
}

void MessageInfoWindow::add_action_download_file(const td::td_api::file &file) {
  if (subscription_ids_.count(file.id_) == 0) {
    auto sub_id = file_manager().subscribe_to_file_updates(
        file.id_, [self = this, self_id = window_unique_id(), root = root()](const td::td_api::updateFile &upd) {
          if (!root->window_exists(self_id)) {
            return;
          }
          self->handle_file_update(upd);
        });
    auto el = add_element("download", "document", {},
                          [self = this, self_id = window_unique_id(), root = root(), file_id = file.id_]() {
                            if (!root->window_exists(self_id)) {
                              return false;
                            }
                            //downloadFile file_id:int32 priority:int32 offset:int53 limit:int53 synchronous:Bool = File;
                            self->send_request(td::make_tl_object<td::td_api::downloadFile>(file_id, 20, 0, 0, false),
                                               [](td::Result<td::tl_object_ptr<td::td_api::file>> R) {});
                            return false;
                          });
    subscription_ids_.emplace(file.id_, std::make_pair(sub_id, std::move(el)));
  }
}

void MessageInfoWindow::add_action_reactions(td::int64 chat_id, td::int64 message_id) {
  Outputter out;
  if (message_->interaction_info_ && message_->interaction_info_->reactions_) {
    for (const auto &r : message_->interaction_info_->reactions_->reactions_) {
      if (r->is_chosen_) {
        out << Outputter::Reverse{true};
      }
      out << "[" << *r->type_ << r->total_count_ << "]";
      if (r->is_chosen_) {
        out << Outputter::Reverse{false};
      }
    }
  }
  add_element("reactions", out.as_str(), out.markup(),
              create_menu_window_spawn_function<ReactionSelectionWindow>(message_->chat_id_, message_->id_));
}

void MessageInfoWindow::add_action_reply(td::int64 chat_id, td::int64 message_id) {
  add_element("reply", "create", {}, [chat_id, message_id, self = this]() {
    self->root()->open_compose_window(chat_id, 0, message_id);
    return true;
  });
}

void MessageInfoWindow::add_action_delete_message(td::int64 chat_id, td::int64 message_id) {
  auto C = chat_manager().get_chat(chat_id);
  if (C->chat_type() == ChatType::Basicgroup || C->chat_type() == ChatType::User) {
    add_element("delete", "delete for self", {}, [chat_id, message_id, self = this]() {
      spawn_yes_no_window(*self, "delete one message for self", {}, [chat_id, message_id, self](td::Result<bool> R) {
        if (R.is_error() || !R.move_as_ok()) {
          return;
        }
        //deleteMessages chat_id:int53 message_ids:vector<int53> revoke:Bool = Ok;
        auto req = td::make_tl_object<td::td_api::deleteMessages>(chat_id, std::vector<td::int64>{message_id}, false);
        loading_window_send_request(*self, "deleting...", {}, std::move(req),
                                    [self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                                      DROP_IF_DELETED(R);
                                      self->exit();
                                    });
      });
      return false;
    });
  }
  add_element("delete", "delete for all", {}, [chat_id, message_id, self = this]() {
    spawn_yes_no_window(*self, "delete one message for all", {}, [chat_id, message_id, self](td::Result<bool> R) {
      if (R.is_error() || !R.move_as_ok()) {
        return;
      }
      //deleteMessages chat_id:int53 message_ids:vector<int53> revoke:Bool = Ok;
      auto req = td::make_tl_object<td::td_api::deleteMessages>(chat_id, std::vector<td::int64>{message_id}, true);
      loading_window_send_request(*self, "deleting...", {}, std::move(req),
                                  [self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                                    DROP_IF_DELETED(R);
                                    self->exit();
                                  });
    });
    return false;
  });
}

void MessageInfoWindow::add_action_view_thread(td::int64 chat_id, td::int64 message_id, td::int64 thread_id) {
  add_element("view thread", "view thread", {}, [chat_id, message_id, thread_id, self = this]() {
    auto w = self->root()->chat_window();
    if (w) {
      w->set_comments_mode(ChatWindow::MessageId{chat_id, message_id}, thread_id);
    }
    return true;
  });
}

void MessageInfoWindow::add_action_recognize_speech(td::int64 chat_id, td::int64 message_id) {
  add_element("recognize audio", "recognize audio", {}, [chat_id, message_id, self = this]() {
    self->send_request(td::make_tl_object<td::td_api::recognizeSpeech>(chat_id, message_id),
                       [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                         DROP_IF_DELETED(R);
                         if (R.is_error()) {
                           LOG(ERROR) << "failed to start recognizing speech: " << R.move_as_error();
                         }
                       });
    return true;
  });
}

void MessageInfoWindow::handle_file_update(const td::td_api::updateFile &f) {
  auto it = subscription_ids_.find(f.file_->id_);
  if (it != subscription_ids_.end()) {
    auto el = it->second.second;
    if (f.file_->local_->is_downloading_completed_) {
      auto new_el = std::make_shared<MenuWindowElementRun>("open", "document", std::vector<windows::MarkupElement>{},
                                                           [file_path = f.file_->local_->path_]() -> bool {
                                                             global_parameters().open_document(file_path);
                                                             return true;
                                                           });
      change_element(el, [&]() { el->update_element(new_el); });
    } else {
      auto percent =
          (int)(f.file_->size_ ? (double)f.file_->local_->downloaded_prefix_size_ * 100.0 / (double)f.file_->size_
                               : 0.0);
      change_element(el, [&]() { el->menu_element()->data = PSTRING() << "document (downloaded " << percent << "%)"; });
    }
  }
}

}  // namespace tdcurses
