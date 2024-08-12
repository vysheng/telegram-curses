#include "ChatWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Random.h"
#include "td/utils/Slice.h"
#include "td/utils/Status.h"
#include "td/utils/overloaded.h"
#include "TdObjectsOutput.h"
#include "MessageActionWindow.hpp"
#include "CommandLineWindow.hpp"
#include "windows/EditorWindow.hpp"
#include "windows/Markup.hpp"
#include "windows/TextEdit.hpp"
#include "windows/unicode.h"
#include <limits>
#include <memory>
#include <tuple>
#include <memory>
#include <vector>
#include <unistd.h>
#include "td/utils/utf8.h"

namespace tdcurses {

void ChatWindow::send_open() {
  auto req = td::make_tl_object<td::td_api::openChat>(main_chat_id());
  send_request(std::move(req), {});
}

void ChatWindow::send_close() {
  auto req = td::make_tl_object<td::td_api::closeChat>(main_chat_id());
  send_request(std::move(req), {});
}

std::string ChatWindow::draft_message_text() {
  auto C = chat_manager().get_chat(main_chat_id());
  if (!C) {
    return "";
  }
  if (!C->draft_message()) {
    return "";
  }
  if (!C->draft_message()->input_message_text_) {
    return "";
  }
  if (C->draft_message()->input_message_text_->get_id() != td::td_api::inputMessageText::ID) {
    return "";
  }
  const auto &text = static_cast<const td::td_api::inputMessageText &>(*C->draft_message()->input_message_text_);
  if (!text.text_) {
    return "";
  }
  return text.text_->text_;
}

void ChatWindow::show_message_actions() {
  auto cur_element = get_active_element();
  if (!cur_element) {
    return;
  }
  auto el = static_cast<Element *>(cur_element.get());
  MessageActionWindowBuilder builder(root(), this);

  builder.add_action_forward(el->message->chat_id_, el->message->id_);
  builder.add_action_reply(el->message->chat_id_, el->message->id_);
  builder.add_action_message_goto("current", el->message->chat_id_, el->message->id_, el->message.get());
  builder.add_action_message_debug_info("current", el->message->chat_id_, el->message->id_, el->message.get());

  if (el->message->is_outgoing_) {
    builder.add_action_chat_info("destination", el->message->chat_id_);
    builder.add_action_chat_open("destination", el->message->chat_id_);
  } else {
    if (el->message->sender_id_) {
      td::td_api::downcast_call(*el->message->sender_id_, td::overloaded(
                                                              [&](td::td_api::messageSenderUser &sender) {
                                                                builder.add_action_user_info("source", sender.user_id_);
                                                                builder.add_action_user_chat_open("source",
                                                                                                  sender.user_id_);
                                                              },
                                                              [&](td::td_api::messageSenderChat &sender) {
                                                                builder.add_action_chat_info("source", sender.chat_id_);
                                                                builder.add_action_chat_open("source", sender.chat_id_);
                                                              }));
    }
  }

  if (el->message->reply_to_) {
    td::td_api::downcast_call(*el->message->reply_to_,
                              td::overloaded(
                                  [&](td::td_api::messageReplyToMessage &info) {
                                    auto msg = get_message_as_message(info.chat_id_, info.message_id_);
                                    builder.add_action_message_goto("replied", info.chat_id_, info.message_id_, msg);
                                  },
                                  [&](td::td_api::messageReplyToStory &info) {}));
  }

  if (el->message->forward_info_ && el->message->forward_info_->origin_) {
    td::td_api::downcast_call(*el->message->forward_info_->origin_,
                              td::overloaded(
                                  [&](td::td_api::messageOriginUser &sender) {
                                    builder.add_action_user_info("forward source", sender.sender_user_id_);
                                    builder.add_action_user_chat_open("forward_source", sender.sender_user_id_);
                                  },
                                  [&](td::td_api::messageOriginChat &sender) {
                                    builder.add_action_chat_info("forward source", sender.sender_chat_id_);
                                    builder.add_action_chat_open("forward source", sender.sender_chat_id_);
                                  },
                                  [&](td::td_api::messageOriginChannel &sender) {
                                    builder.add_action_chat_info("forward source", sender.chat_id_);
                                    builder.add_action_chat_open("forward source", sender.chat_id_);
                                  },
                                  [&](td::td_api::messageOriginHiddenUser &sender) {}));
  }

  if (el->message->via_bot_user_id_) {
    builder.add_action_user_info("forward source", el->message->via_bot_user_id_);
    builder.add_action_user_chat_open("forward_source", el->message->via_bot_user_id_);
  }

  auto process_formatted_text = [&](const td::td_api::formattedText &content) {
    auto &text = content.text_;

    if (text.size() > 0) {
      builder.add_action_copy_primary(text);
      builder.add_action_copy(text);
    }

    for (auto &ent : content.entities_) {
      td::td_api::downcast_call(
          const_cast<td::td_api::TextEntityType &>(*ent->type_),
          td::overloaded(
              [&](const td::td_api::textEntityTypeMention &e) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                builder.add_action_chat_by_username("mentionen chat", us.str());
                builder.add_action_chat_open_by_username("mentionen chat", us.str());
              },
              [&](const td::td_api::textEntityTypeHashtag &e) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                builder.add_action_search_pattern(us.str());
              },
              [&](const td::td_api::textEntityTypeCashtag &) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                builder.add_action_search_pattern(us.str());
              },
              [&](const td::td_api::textEntityTypeBotCommand &) {},
              [&](const td::td_api::textEntityTypeUrl &) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                builder.add_action_link(us.str());
              },
              [&](const td::td_api::textEntityTypeEmailAddress &) {},
              [&](const td::td_api::textEntityTypePhoneNumber &) {},
              [&](const td::td_api::textEntityTypeBankCardNumber &) {}, [&](const td::td_api::textEntityTypeBold &) {},
              [&](const td::td_api::textEntityTypeItalic &) {},
              [&](const td::td_api::textEntityTypeUnderline &) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                builder.add_action_search_pattern(us.str());
              },
              [&](const td::td_api::textEntityTypeStrikethrough &) {},
              [&](const td::td_api::textEntityTypeSpoiler &) {}, [&](const td::td_api::textEntityTypeCode &) {},
              [&](const td::td_api::textEntityTypePre &) {}, [&](const td::td_api::textEntityTypePreCode &) {},
              [&](const td::td_api::textEntityTypeBlockQuote &e) {},
              [&](const td::td_api::textEntityTypeExpandableBlockQuote &) {},
              [&](const td::td_api::textEntityTypeTextUrl &e) { builder.add_action_link(e.url_); },
              [&](const td::td_api::textEntityTypeMentionName &e) {
                auto us = get_utf8_string_substring_utf16_codepoints(text, ent->offset_, ent->offset_ + ent->length_);
                builder.add_action_user_info("mentioned chat", e.user_id_, us.str());
                builder.add_action_user_chat_open("mentioned chat", e.user_id_, us.str());
              },
              [&](const td::td_api::textEntityTypeCustomEmoji &) {},
              [&](const td::td_api::textEntityTypeMediaTimestamp &) {}));
    }
  };

  if (el->message->content_) {
    td::td_api::downcast_call(
        *el->message->content_,
        td::overloaded(
            [&](const td::td_api::messageText &content) { process_formatted_text(*content.text_); },
            [&](const td::td_api::messageAnimation &content) {
              process_formatted_text(*content.caption_);
              builder.add_action_open_file(*content.animation_->animation_);
            },
            [&](const td::td_api::messageAudio &content) {
              process_formatted_text(*content.caption_);
              builder.add_action_open_file(*content.audio_->audio_);
            },
            [&](const td::td_api::messageDocument &content) {
              process_formatted_text(*content.caption_);
              builder.add_action_open_file(*content.document_->document_);
            },
            [&](const td::td_api::messagePaidMedia &content) { process_formatted_text(*content.caption_); },
            [&](const td::td_api::messagePhoto &content) {
              process_formatted_text(*content.caption_);
              builder.add_action_open_file(*content.photo_->sizes_.back()->photo_);
            },
            [&](const td::td_api::messageSticker &content) {},
            [&](const td::td_api::messageVideo &content) {
              process_formatted_text(*content.caption_);
              builder.add_action_open_file(*content.video_->video_);
            },
            [&](const td::td_api::messageVideoNote &content) {
              builder.add_action_open_file(*content.video_note_->video_);
            },
            [&](const td::td_api::messageVoiceNote &content) {
              builder.add_action_open_file(*content.voice_note_->voice_);
            },
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
                builder.add_action_poll(el->message->chat_id_, el->message->id_, *opt->text_,
                                        opt->is_chosen_ || opt->is_being_chosen_, idx++);
              }
            },
            [&](const td::td_api::messageStory &content) {}, [&](const td::td_api::messageInvoice &content) {},
            [&](const td::td_api::messageCall &content) {},
            [&](const td::td_api::messageVideoChatScheduled &content) {},
            [&](const td::td_api::messageVideoChatStarted &content) {},
            [&](const td::td_api::messageVideoChatEnded &content) {},
            [&](const td::td_api::messageInviteVideoChatParticipants &content) {},
            [&](const td::td_api::messageBasicGroupChatCreate &content) {},
            [&](const td::td_api::messageSupergroupChatCreate &content) {},
            [&](const td::td_api::messageChatChangeTitle &content) {},
            [&](const td::td_api::messageChatChangePhoto &content) {},
            [&](const td::td_api::messageChatDeletePhoto &content) {},
            [&](const td::td_api::messageChatAddMembers &content) {},
            [&](const td::td_api::messageChatJoinByLink &content) {},
            [&](const td::td_api::messageChatJoinByRequest &content) {},
            [&](const td::td_api::messageChatDeleteMember &content) {},
            [&](const td::td_api::messageChatUpgradeTo &content) {},
            [&](const td::td_api::messageChatUpgradeFrom &content) {},
            [&](const td::td_api::messagePinMessage &content) {},
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
            [&](const td::td_api::messagePremiumGiveawayCreated &content) {},
            [&](const td::td_api::messagePremiumGiveaway &content) {},
            [&](const td::td_api::messagePremiumGiveawayCompleted &content) {},
            [&](const td::td_api::messagePremiumGiveawayWinners &content) {},
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

  root()->add_popup_window(builder.build(), 2);
}

void ChatWindow::handle_input(TickitKeyEventInfo *info) {
  set_need_refresh();
  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "Escape")) {
      set_search_pattern("");
    }
    if (!strcmp(info->str, "Enter")) {
      show_message_actions();
      return;
    }
  } else {
    if (!strcmp(info->str, " ")) {
      auto el = get_active_element();
      if (!el) {
        return;
      }
      auto &e = static_cast<Element &>(*el);
      e.run(this);
      return;
    } else if (!strcmp(info->str, "i")) {
      root()->open_compose_window(main_chat_id_, 0);
      return;
    } else if (!strcmp(info->str, "I")) {
      show_message_actions();
      return;
    } else if (!strcmp(info->str, "r")) {
      auto el = get_active_element();
      if (!el) {
        return;
      }
      auto &e = static_cast<Element &>(*el);

      root()->open_compose_window(main_chat_id_, e.message_id().message_id);
      return;
    } else if (!strcmp(info->str, "/") || !strcmp(info->str, ":")) {
      root()->command_line_window()->handle_input(info);
      return;
    }
  }
  windows::PadWindow::handle_input(info);
  {
    auto el = get_active_element();
    if (el) {
      auto &e = static_cast<Element &>(*el);
      auto req = td::make_tl_object<td::td_api::viewMessages>(e.message->chat_id_,
                                                              std::vector<td::int64>{e.message->id_}, nullptr, false);
      send_request(std::move(req), {});
    }
  }
}

td::int32 ChatWindow::Element::render(windows::PadWindow &root, TickitRenderBuffer *rb, bool is_selected) {
  td::int32 cursor_x, cursor_y;
  TickitCursorShape cursor_shape;
  Outputter out;
  out.set_chat(static_cast<ChatWindow *>(&root));
  out << message;
  return windows::TextEdit::render(rb, cursor_x, cursor_y, cursor_shape, width(), out.as_str(), 0, out.markup(),
                                   is_selected, false);
}

void ChatWindow::request_bottom_elements_ex(td::int32 message_id) {
  if (running_req_bottom_ || messages_.size() == 0 || is_completed_bottom_ || search_pattern_.size() > 0) {
    return;
  }

  running_req_bottom_ = true;

  auto max_message_id = get_newest_message_id();

  if (search_pattern_.size() == 0) {
    auto req = td::make_tl_object<td::td_api::getChatHistory>(max_message_id.chat_id, max_message_id.message_id, -10,
                                                              10, false);
    send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
      received_bottom_elements(std::move(R));
    });
  } else {
    //searchChatMessages chat_id:int53 query:string sender_id:MessageSender from_message_id:int53 offset:int32 limit:int32 filter:SearchMessagesFilter message_thread_id:int53 saved_messages_topic_id:int53 = FoundChatMessages;
    auto req = td::make_tl_object<td::td_api::searchChatMessages>(
        max_message_id.chat_id, search_pattern_, /* sender_id */ nullptr,
        /* from_message_id */ max_message_id.message_id, /* offset */ -10, /* limit */ 11,
        /*filter */ nullptr, /* message_thread_id */ 0, /* message_topic_id */ 0);
    send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R) {
      received_bottom_search_elements(std::move(R));
    });
  }
}

void ChatWindow::received_bottom_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
  if (R.is_error() && R.error().code() == ErrorCodeWindowDeleted) {
    return;
  }
  CHECK(running_req_bottom_);
  running_req_bottom_ = false;
  R.ensure();
  auto res = R.move_as_ok();
  if (res->messages_.size() == 0) {
    is_completed_bottom_ = true;
  }
  add_messages(std::move(res->messages_));
}

void ChatWindow::received_bottom_search_elements(td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R) {
  if (R.is_error() && R.error().code() == ErrorCodeWindowDeleted) {
    return;
  }
  CHECK(running_req_bottom_);
  running_req_bottom_ = false;
  R.ensure();
  auto res = R.move_as_ok();
  if (res->messages_.size() == 0) {
    is_completed_bottom_ = true;
  }
  add_messages(std::move(res->messages_));
}

void ChatWindow::request_top_elements_ex(td::int32 message_id) {
  if (running_req_top_ || is_completed_top_) {
    return;
  }
  running_req_top_ = true;

  auto min_message_id = get_oldest_message_id();
  auto msg = get_message_as_message(min_message_id);

  if (msg && msg->content_->get_id() == td::td_api::messageChatUpgradeFrom::ID) {
    auto content = static_cast<const td::td_api::messageChatUpgradeFrom *>(msg->content_.get());
    auto req0 = td::make_tl_object<td::td_api::createBasicGroupChat>(content->basic_group_id_, true);
    send_request(std::move(req0), [&](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
      if (R.is_error()) {
        received_top_elements(R.move_as_error());
        return;
      }
      auto chat = R.move_as_ok();
      auto req = td::make_tl_object<td::td_api::getChatHistory>(chat->id_, std::numeric_limits<td::int64>::max(), 0, 10,
                                                                false);
      send_request(std::move(req),
                   [&](td::Result<td::tl_object_ptr<td::td_api::messages>> R) { received_top_elements(std::move(R)); });
    });
    return;
  }
  if (search_pattern_.size() == 0) {
    auto req =
        td::make_tl_object<td::td_api::getChatHistory>(min_message_id.chat_id, min_message_id.message_id, 0, 10, false);
    send_request(std::move(req),
                 [&](td::Result<td::tl_object_ptr<td::td_api::messages>> R) { received_top_elements(std::move(R)); });
  } else {
    //searchChatMessages chat_id:int53 query:string sender_id:MessageSender from_message_id:int53 offset:int32 limit:int32 filter:SearchMessagesFilter message_thread_id:int53 saved_messages_topic_id:int53 = FoundChatMessages;
    auto req = td::make_tl_object<td::td_api::searchChatMessages>(
        min_message_id.chat_id, search_pattern_, /* sender_id */ nullptr,
        /* from_message_id */ min_message_id.message_id, /* offset */ 0, /* limit */ 10,
        /*filter */ nullptr, /* message_thread_id */ 0, /* message_topic_id */ 0);
    send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R) {
      received_top_search_elements(std::move(R));
    });
  }
}
void ChatWindow::received_top_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
  if (R.is_error() && R.error().code() == ErrorCodeWindowDeleted) {
    return;
  }
  if (R.is_error()) {
    running_req_top_ = false;
    return;
  }
  CHECK(running_req_top_);
  running_req_top_ = false;
  R.ensure();
  auto res = R.move_as_ok();
  if (res->messages_.size() == 0) {
    is_completed_top_ = true;
  }
  add_messages(std::move(res->messages_));
}

void ChatWindow::received_top_search_elements(td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R) {
  if (R.is_error() && R.error().code() == ErrorCodeWindowDeleted) {
    return;
  }
  CHECK(running_req_top_);
  running_req_top_ = false;
  R.ensure();
  auto res = R.move_as_ok();
  if (res->messages_.size() == 0) {
    is_completed_top_ = true;
  }
  add_messages(std::move(res->messages_));
}

void ChatWindow::add_messages(std::vector<td::tl_object_ptr<td::td_api::message>> msgs) {
  for (auto &m : msgs) {
    auto id = build_message_id(*m);
    auto it = messages_.find(id);
    if (it != messages_.end()) {
    } else {
      auto el = std::make_shared<Element>(std::move(m), get_chat_generation(m->chat_id_));
      messages_.emplace(id, el);
      add_file_message_pair(id, get_file_id(*el->message));
      add_element(std::move(el));
    }
  }
}

//@description A new message was received; can also be an outgoing message @message The new message
//updateNewMessage message:message = Update;
void ChatWindow::process_update(td::td_api::updateNewMessage &update) {
  if (!is_completed_bottom_) {
    return;
  }
  auto m = std::move(update.message_);
  auto id = build_message_id(*m);
  auto it = messages_.find(id);
  if (it != messages_.end()) {
  } else {
    auto el = std::make_shared<Element>(std::move(m), get_chat_generation(m->chat_id_));
    messages_.emplace(id, el);
    add_file_message_pair(id, get_file_id(*el->message));
    add_element(std::move(el));
  }
}

void ChatWindow::process_update_sent_message(td::tl_object_ptr<td::td_api::message> message) {
  auto id = build_message_id(*message);
  auto el = std::make_shared<Element>(std::move(message), get_chat_generation(id.chat_id));
  messages_.emplace(id, el);
  add_file_message_pair(id, get_file_id(*el->message));
  add_element(std::move(el));
}

//@description A request to send a message has reached the Telegram server. This doesn't mean that the message will be sent successfully or even that the send message request will be processed. This update will be sent only if the option "use_quick_ack" is set to true. This update may be sent multiple times for the same message
//@chat_id The chat identifier of the sent message @message_id A temporary message identifier
//updateMessageSendAcknowledged chat_id:int53 message_id:int53 = Update;
void ChatWindow::process_update(td::td_api::updateMessageSendAcknowledged &update) {
}

//@description A message has been successfully sent @message Information about the sent message. Usually only the message identifier, date, and content are changed, but almost all other fields can also change @old_message_id The previous temporary message identifier
//updateMessageSendSucceeded message:message old_message_id:int53 = Update;
void ChatWindow::process_update(td::td_api::updateMessageSendSucceeded &update) {
  auto new_id = build_message_id(*update.message_);
  auto old_id = MessageId{update.message_->chat_id_, update.old_message_id_};
  auto it = messages_.find(old_id);
  if (it != messages_.end()) {
    del_file_message_pair(old_id, get_file_id(*it->second->message));
    delete_element(it->second.get());
    messages_.erase(it);
    auto m = std::move(update.message_);
    auto el = std::make_shared<Element>(std::move(m), get_chat_generation(new_id.chat_id));
    messages_.emplace(new_id, el);
    add_file_message_pair(new_id, get_file_id(*el->message));
    add_element(std::move(el));
  }
}

//@description A message failed to send. Be aware that some messages being sent can be irrecoverably deleted, in which case updateDeleteMessages will be received instead of this update
//@message Contains information about the message that failed to send @old_message_id The previous temporary message identifier @error_code An error code @error_message Error message
//updateMessageSendFailed message:message old_message_id:int53 error_code:int32 error_message:string = Update;
void ChatWindow::process_update(td::td_api::updateMessageSendFailed &update) {
  auto new_id = build_message_id(*update.message_);
  auto old_id = MessageId{update.message_->chat_id_, update.old_message_id_};
  auto it = messages_.find(old_id);
  if (it != messages_.end()) {
    del_file_message_pair(old_id, get_file_id(*it->second->message));
    delete_element(it->second.get());
    messages_.erase(it);
    auto m = std::move(update.message_);
    auto el = std::make_shared<Element>(std::move(m), get_chat_generation(new_id.chat_id));
    messages_.emplace(new_id, el);
    add_file_message_pair(new_id, get_file_id(*el->message));
    add_element(std::move(el));
  }
}

//@description The message content has changed @chat_id Chat identifier @message_id Message identifier @new_content New message content
//updateMessageContent chat_id:int53 message_id:int53 new_content:MessageContent = Update;
void ChatWindow::process_update(td::td_api::updateMessageContent &update) {
  auto id = build_message_id(update.chat_id_, update.message_id_);
  auto it = messages_.find(id);
  if (it != messages_.end()) {
    auto old_f = get_file_id(*it->second->message);
    it->second->message->content_ = std::move(update.new_content_);
    auto new_f = get_file_id(*it->second->message);
    if (old_f != new_f) {
      del_file_message_pair(id, old_f);
      add_file_message_pair(id, new_f);
    }
    change_element(it->second.get());
  }
}

//@description A message was edited. Changes in the message content will come in a separate updateMessageContent @chat_id Chat identifier @message_id Message identifier @edit_date Point in time (Unix timestamp) when the message was edited @reply_markup New message reply markup; may be null
//updateMessageEdited chat_id : int53 message_id : int53 edit_date : int32 reply_markup : ReplyMarkup = Update;
void ChatWindow::process_update(td::td_api::updateMessageEdited &update) {
  auto id = build_message_id(update.chat_id_, update.message_id_);
  auto it = messages_.find(id);
  if (it != messages_.end()) {
    it->second->message->edit_date_ = update.edit_date_;
    it->second->message->reply_markup_ = std::move(update.reply_markup_);
    change_element(it->second.get());
  }
}

//@description The message pinned state was changed @chat_id Chat identifier @message_id The message identifier @is_pinned True, if the message is pinned
//updateMessageIsPinned chat_id:int53 message_id:int53 is_pinned:Bool = Update;
void ChatWindow::process_update(td::td_api::updateMessageIsPinned &update) {
  auto id = build_message_id(update.chat_id_, update.message_id_);
  auto it = messages_.find(id);
  if (it != messages_.end()) {
    it->second->message->is_pinned_ = update.is_pinned_;
    change_element(it->second.get());
  }
}

//@description The information about interactions with a message has changed @chat_id Chat identifier @message_id Message identifier @interaction_info New information about interactions with the message; may be null
//updateMessageInteractionInfo chat_id:int53 message_id:int53 interaction_info:messageInteractionInfo = Update;
void ChatWindow::process_update(td::td_api::updateMessageInteractionInfo &update) {
  auto id = build_message_id(update.chat_id_, update.message_id_);
  auto it = messages_.find(id);
  if (it != messages_.end()) {
    it->second->message->interaction_info_ = std::move(update.interaction_info_);
    change_element(it->second.get());
  }
}

//@description The message content was opened. Updates voice note messages to "listened", video note messages to "viewed" and starts the TTL timer for self-destructing messages @chat_id Chat identifier @message_id Message identifier
//updateMessageContentOpened chat_id : int53 message_id : int53 = Update;
void ChatWindow::process_update(td::td_api::updateMessageContentOpened &update) {
  auto id = build_message_id(update.chat_id_, update.message_id_);
  auto it = messages_.find(id);
  if (it != messages_.end()) {
    // ???
    change_element(it->second.get());
  }
}

//@description A message with an unread mention was read @chat_id Chat identifier @message_id Message identifier @unread_mention_count The new number of unread mention messages left in the chat
//updateMessageMentionRead chat_id : int53 message_id : int53 unread_mention_count : int32 = Update;
void ChatWindow::process_update(td::td_api::updateMessageMentionRead &update) {
  auto id = build_message_id(update.chat_id_, update.message_id_);
  auto it = messages_.find(id);
  if (it != messages_.end()) {
    it->second->message->contains_unread_mention_ = false;
    change_element(it->second.get());
  }
}

//@description A fact-check added to a message was changed
//@chat_id Chat identifier
//@message_id Message identifier
//@fact_check The new fact-check
//updateMessageFactCheck chat_id:int53 message_id:int53 fact_check:factCheck = Update;
void ChatWindow::process_update(td::td_api::updateMessageFactCheck &update) {
  auto id = build_message_id(update.chat_id_, update.message_id_);
  auto it = messages_.find(id);
  if (it != messages_.end()) {
    it->second->message->fact_check_ = std::move(update.fact_check_);
    change_element(it->second.get());
  }
}

//@description The list of unread reactions added to a message was changed @chat_id Chat identifier @message_id Message identifier @unread_reactions The new list of unread reactions @unread_reaction_count The new number of messages with unread reactions left in the chat
//updateMessageUnreadReactions chat_id:int53 message_id:int53 unread_reactions:vector<unreadReaction> unread_reaction_count:int32 = Update;
void ChatWindow::process_update(td::td_api::updateMessageUnreadReactions &update) {
  auto id = build_message_id(update.chat_id_, update.message_id_);
  auto it = messages_.find(id);
  if (it != messages_.end()) {
    it->second->message->unread_reactions_ = std::move(update.unread_reactions_);
    change_element(it->second.get());
  }
}

//@description A message with a live location was viewed. When the update is received, the client is supposed to update the live location
//@chat_id Identifier of the chat with the live location message @message_id Identifier of the message with live location
void ChatWindow::process_update(td::td_api::updateMessageLiveLocationViewed &update) {
}

void ChatWindow::process_update(td::td_api::updateDeleteMessages &update) {
  if (!update.is_permanent_) {
    return;
  }
  for (auto x : update.message_ids_) {
    auto id = build_message_id(update.chat_id_, x);
    auto it = messages_.find(id);
    if (it != messages_.end()) {
      delete_element(it->second.get());
      messages_.erase(it);
    }
  }
}

//@description Some messages were deleted
//@chat_id Chat identifier
//@message_ids Identifiers of the deleted messages
//@is_permanent True, if the messages are permanently deleted by a user (as opposed to just becoming inaccessible)
//@from_cache True, if the messages are deleted only from the cache and can possibly be retrieved again in the future
//updateDeleteMessages chat_id:int53 message_ids:vector<int53> is_permanent:Bool from_cache:Bool = Update;
void ChatWindow::process_update(td::td_api::updateFile &update) {
  auto it = file_id_2_messages_.find(update.file_->id_);
  if (it != file_id_2_messages_.end()) {
    for (auto x : it->second) {
      update_file(x, *update.file_);
    }
  }
}

void ChatWindow::process_update(td::td_api::updateChatAction &update) {
}

td::int32 ChatWindow::get_file_id(const td::td_api::message &message) {
  td::int32 res = 0;
  td::td_api::downcast_call(
      const_cast<td::td_api::MessageContent &>(*message.content_),
      td::overloaded([&](const td::td_api::messageAnimation &content) { res = content.animation_->animation_->id_; },
                     [&](const td::td_api::messageAudio &content) { res = content.audio_->audio_->id_; },
                     [&](const td::td_api::messageDocument &content) { res = content.document_->document_->id_; },
                     [&](const td::td_api::messagePhoto &content) { res = content.photo_->sizes_.back()->photo_->id_; },
                     [&](const td::td_api::messageSticker &content) { res = content.sticker_->sticker_->id_; },
                     [&](const td::td_api::messageVideo &content) { res = content.video_->video_->id_; },
                     [&](const td::td_api::messageVideoNote &content) { res = content.video_note_->video_->id_; },
                     [&](const td::td_api::messageVoiceNote &content) { res = content.voice_note_->voice_->id_; },
                     [&](const auto &x) {}));
  return res;
}

void ChatWindow::update_message_file(td::td_api::message &message, const td::td_api::file &file_in) {
  td::tl_object_ptr<td::td_api::localFile> l;
  if (file_in.local_) {
    l = td::make_tl_object<td::td_api::localFile>(
        file_in.local_->path_, file_in.local_->can_be_downloaded_, file_in.local_->can_be_deleted_,
        file_in.local_->is_downloading_active_, file_in.local_->is_downloading_completed_,
        file_in.local_->download_offset_, file_in.local_->downloaded_prefix_size_, file_in.local_->downloaded_size_);
  }
  td::tl_object_ptr<td::td_api::remoteFile> r;
  if (file_in.remote_) {
    r = td::make_tl_object<td::td_api::remoteFile>(
        file_in.remote_->id_, file_in.remote_->unique_id_, file_in.remote_->is_uploading_active_,
        file_in.remote_->is_uploading_completed_, file_in.remote_->uploaded_size_);
  }
  auto file = td::make_tl_object<td::td_api::file>(file_in.id_, file_in.size_, file_in.expected_size_, std::move(l),
                                                   std::move(r));
  //file id:int32 size:int53 expected_size:int53 local:localFile remote:remoteFile = File;
  td::tl_object_ptr<td::td_api::file> *f = nullptr;
  td::td_api::downcast_call(
      *message.content_,
      td::overloaded([&](td::td_api::messageAnimation &content) { f = &content.animation_->animation_; },
                     [&](td::td_api::messageAudio &content) { f = &content.audio_->audio_; },
                     [&](td::td_api::messageDocument &content) { f = &content.document_->document_; },
                     [&](td::td_api::messagePhoto &content) { f = &content.photo_->sizes_.back()->photo_; },
                     [&](td::td_api::messageSticker &content) { f = &content.sticker_->sticker_; },
                     [&](td::td_api::messageVideo &content) { f = &content.video_->video_; },
                     [&](td::td_api::messageVideoNote &content) { f = &content.video_note_->video_; },
                     [&](td::td_api::messageVoiceNote &content) { f = &content.voice_note_->voice_; },
                     [&](auto &x) {}));
  if (f && (*f)->id_ == file->id_) {
    *f = std::move(file);
  }
}

void ChatWindow::update_file(MessageId message_id, td::td_api::file &file) {
  auto it = messages_.find(message_id);
  if (it != messages_.end()) {
    update_message_file(*it->second->message, file);
    change_element(it->second.get());
  }
}

void ChatWindow::Element::run(ChatWindow *window) {
  auto file_id = ChatWindow::get_file_id(*message);
  if (file_id) {
    window->send_request(td::make_tl_object<td::td_api::downloadFile>(file_id, 16, 0, 0, true),
                         [id = build_message_id(*message), window](td::Result<td::tl_object_ptr<td::td_api::file>> R) {
                           if (R.is_error()) {
                             return;
                           }
                           window->update_file(id, *R.move_as_ok());
                         });
    //downloadFile file_id:int32 priority:int32 offset:int53 limit:int53 synchronous:Bool = File;
  }
}

void ChatWindow::set_search_pattern(std::string pattern) {
  if (pattern == search_pattern_) {
    return;
  }
  std::shared_ptr<Element> cur;
  if (pattern == "") {
    cur = std::static_pointer_cast<Element>(get_active_element());
  }

  PadWindow::clear();
  change_unique_id();

  file_id_2_messages_.clear();
  search_pattern_ = std::move(pattern);
  running_req_top_ = false;
  running_req_bottom_ = false;
  is_completed_top_ = false;
  is_completed_bottom_ = false;

  messages_.clear();

  if (cur) {
    messages_.emplace(cur->message_id(), cur);
    add_element(cur);
    unglue();
  }

  set_need_refresh();
  request_top_elements_ex(0);
  if (cur) {
    request_bottom_elements_ex(0);
  }

  root()->update_status_line();
}

void ChatWindow::seek(td::int64 chat_id, td::int64 message_id) {
  return seek(build_message_id(chat_id, message_id));
}

void ChatWindow::seek(MessageId message_id) {
  auto it = messages_.find(message_id);
  if (it != messages_.end()) {
    scroll_to_element(it->second.get(), true);
    set_search_pattern("");
  }
}

ChatWindow::MessageId ChatWindow::get_oldest_message_id() {
  auto element = PadWindow::first_element();
  if (!element) {
    return MessageId{main_chat_id_, std::numeric_limits<td::int64>::max()};
  }
  return static_cast<const Element *>(element)->message_id();
}

ChatWindow::MessageId ChatWindow::get_newest_message_id() {
  auto element = PadWindow::last_element();
  if (!element) {
    return MessageId{main_chat_id_, std::numeric_limits<td::int64>::max()};
  }
  return static_cast<const Element *>(element)->message_id();
}

}  // namespace tdcurses
