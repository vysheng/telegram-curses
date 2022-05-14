#include "chat-window.h"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "telegram-curses-output.h"
#include "windows/text.h"
#include <limits>
#include <memory>

namespace tdcurses {

void ChatWindow::send_open() {
  auto req = td::make_tl_object<td::td_api::openChat>(chat_id_);
  send_request(std::move(req), {});
}

void ChatWindow::send_close() {
  auto req = td::make_tl_object<td::td_api::closeChat>(chat_id_);
  send_request(std::move(req), {});
}

std::string ChatWindow::draft_message_text() {
  auto C = ChatManager::instance->get_chat(chat_id_);
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

void ChatWindow::handle_input(TickitKeyEventInfo *info) {
  set_need_refresh();
  if (info->type == TICKIT_KEYEV_KEY) {
  } else {
    if (!strcmp(info->str, "i")) {
      LOG(ERROR) << "opening compose window";
      root()->open_compose_window();
      return;
    }
  }
  windows::PadWindow::handle_input(info);
  {
    auto el = get_active_element();
    if (el) {
      LOG(ERROR) << "view message";
      auto &e = static_cast<Element &>(*el);
      LOG(ERROR) << "view message " << e.message->id_;
      auto req =
          td::make_tl_object<td::td_api::viewMessages>(chat_id_, 0, std::vector<td::int64>{e.message->id_}, false);
      send_request(std::move(req), {});
    }
  }
}

td::int32 ChatWindow::Element::render(TickitRenderBuffer *rb, bool is_selected) {
  Outputter out;
  out << message;
  td::int32 cursor_x, cursor_y;
  TickitCursorShape cursor_shape;
  return windows::TextEdit::render(rb, cursor_x, cursor_y, cursor_shape, width(), out.as_str(), 0, out.markup(),
                                   is_selected, false);
}

void ChatWindow::request_bottom_elements() {
  if (running_req_bottom_ || messages_.size() == 0 || is_completed_bottom_) {
    return;
  }
  running_req_bottom_ = true;
  LOG(ERROR) << "requesting bottom";
  auto m = messages_.rbegin()->first;
  auto req = td::make_tl_object<td::td_api::getChatHistory>(chat_id_, m, -10, 10, false);
  send_request(std::move(req),
               [&](td::Result<td::tl_object_ptr<td::td_api::messages>> R) { received_top_elements(std::move(R)); });
}
void ChatWindow::received_bottom_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
  CHECK(running_req_bottom_);
  running_req_bottom_ = false;
  R.ensure();
  auto res = R.move_as_ok();
  if (res->messages_.size() == 0) {
    is_completed_bottom_ = true;
  }
  add_messages(std::move(res));
}

void ChatWindow::request_top_elements() {
  if (running_req_top_ || is_completed_top_) {
    return;
  }
  LOG(ERROR) << "requesting top";
  running_req_top_ = true;
  td::int64 m;
  if (messages_.size() != 0) {
    m = messages_.begin()->first;
  } else {
    m = std::numeric_limits<td::int64>::max();
    is_completed_bottom_ = true;
  }
  auto req = td::make_tl_object<td::td_api::getChatHistory>(chat_id_, m, 0, 10, false);
  send_request(std::move(req),
               [&](td::Result<td::tl_object_ptr<td::td_api::messages>> R) { received_top_elements(std::move(R)); });
}
void ChatWindow::received_top_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
  CHECK(running_req_top_);
  running_req_top_ = false;
  R.ensure();
  auto res = R.move_as_ok();
  if (res->messages_.size() == 0) {
    is_completed_top_ = true;
  }
  add_messages(std::move(res));
}

void ChatWindow::add_messages(td::tl_object_ptr<td::td_api::messages> msgs) {
  for (auto &m : msgs->messages_) {
    auto id = m->id_;
    auto it = messages_.find(id);
    if (it != messages_.end()) {
    } else {
      auto el = std::make_shared<Element>(std::move(m));
      messages_.emplace(id, el);
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
  auto id = m->id_;
  auto it = messages_.find(id);
  if (it != messages_.end()) {
  } else {
    auto el = std::make_shared<Element>(std::move(m));
    messages_.emplace(id, el);
    add_element(std::move(el));
  }
}

//@description A request to send a message has reached the Telegram server. This doesn't mean that the message will be sent successfully or even that the send message request will be processed. This update will be sent only if the option "use_quick_ack" is set to true. This update may be sent multiple times for the same message
//@chat_id The chat identifier of the sent message @message_id A temporary message identifier
//updateMessageSendAcknowledged chat_id:int53 message_id:int53 = Update;
void ChatWindow::process_update(td::td_api::updateMessageSendAcknowledged &update) {
}

//@description A message has been successfully sent @message Information about the sent message. Usually only the message identifier, date, and content are changed, but almost all other fields can also change @old_message_id The previous temporary message identifier
//updateMessageSendSucceeded message:message old_message_id:int53 = Update;
void ChatWindow::process_update(td::td_api::updateMessageSendSucceeded &update) {
  auto it = messages_.find(update.old_message_id_);
  if (it != messages_.end()) {
    delete_element(it->second.get());
    messages_.erase(it);
    auto m = std::move(update.message_);
    auto id = m->id_;
    auto el = std::make_shared<Element>(std::move(m));
    messages_.emplace(id, el);
    add_element(std::move(el));
  }
}

//@description A message failed to send. Be aware that some messages being sent can be irrecoverably deleted, in which case updateDeleteMessages will be received instead of this update
//@message Contains information about the message that failed to send @old_message_id The previous temporary message identifier @error_code An error code @error_message Error message
//updateMessageSendFailed message:message old_message_id:int53 error_code:int32 error_message:string = Update;
void ChatWindow::process_update(td::td_api::updateMessageSendFailed &update) {
  auto it = messages_.find(update.old_message_id_);
  if (it != messages_.end()) {
    delete_element(it->second.get());
    messages_.erase(it);
    auto m = std::move(update.message_);
    auto id = m->id_;
    auto el = std::make_shared<Element>(std::move(m));
    messages_.emplace(id, el);
    add_element(std::move(el));
  }
}

//@description The message content has changed @chat_id Chat identifier @message_id Message identifier @new_content New message content
//updateMessageContent chat_id:int53 message_id:int53 new_content:MessageContent = Update;
void ChatWindow::process_update(td::td_api::updateMessageContent &update) {
  auto it = messages_.find(update.message_id_);
  if (it != messages_.end()) {
    it->second->message->content_ = std::move(update.new_content_);
    change_element(it->second.get());
  }
}

//@description A message was edited. Changes in the message content will come in a separate updateMessageContent @chat_id Chat identifier @message_id Message identifier @edit_date Point in time (Unix timestamp) when the message was edited @reply_markup New message reply markup; may be null
//updateMessageEdited chat_id : int53 message_id : int53 edit_date : int32 reply_markup : ReplyMarkup = Update;
void ChatWindow::process_update(td::td_api::updateMessageEdited &update) {
  auto it = messages_.find(update.message_id_);
  if (it != messages_.end()) {
    it->second->message->edit_date_ = update.edit_date_;
    it->second->message->reply_markup_ = std::move(update.reply_markup_);
    change_element(it->second.get());
  }
}

//@description The message pinned state was changed @chat_id Chat identifier @message_id The message identifier @is_pinned True, if the message is pinned
//updateMessageIsPinned chat_id:int53 message_id:int53 is_pinned:Bool = Update;
void ChatWindow::process_update(td::td_api::updateMessageIsPinned &update) {
  auto it = messages_.find(update.message_id_);
  if (it != messages_.end()) {
    it->second->message->is_pinned_ = update.is_pinned_;
    change_element(it->second.get());
  }
}

//@description The information about interactions with a message has changed @chat_id Chat identifier @message_id Message identifier @interaction_info New information about interactions with the message; may be null
//updateMessageInteractionInfo chat_id:int53 message_id:int53 interaction_info:messageInteractionInfo = Update;
void ChatWindow::process_update(td::td_api::updateMessageInteractionInfo &update) {
  auto it = messages_.find(update.message_id_);
  if (it != messages_.end()) {
    it->second->message->interaction_info_ = std::move(update.interaction_info_);
    change_element(it->second.get());
  }
}

//@description The message content was opened. Updates voice note messages to "listened", video note messages to "viewed" and starts the TTL timer for self-destructing messages @chat_id Chat identifier @message_id Message identifier
//updateMessageContentOpened chat_id : int53 message_id : int53 = Update;
void ChatWindow::process_update(td::td_api::updateMessageContentOpened &update) {
  auto it = messages_.find(update.message_id_);
  if (it != messages_.end()) {
    // ???
    change_element(it->second.get());
  }
}

//@description A message with an unread mention was read @chat_id Chat identifier @message_id Message identifier @unread_mention_count The new number of unread mention messages left in the chat
//updateMessageMentionRead chat_id : int53 message_id : int53 unread_mention_count : int32 = Update;
void ChatWindow::process_update(td::td_api::updateMessageMentionRead &update) {
  auto it = messages_.find(update.message_id_);
  if (it != messages_.end()) {
    it->second->message->contains_unread_mention_ = false;
    change_element(it->second.get());
  }
}

//@description The list of unread reactions added to a message was changed @chat_id Chat identifier @message_id Message identifier @unread_reactions The new list of unread reactions @unread_reaction_count The new number of messages with unread reactions left in the chat
//updateMessageUnreadReactions chat_id:int53 message_id:int53 unread_reactions:vector<unreadReaction> unread_reaction_count:int32 = Update;
void ChatWindow::process_update(td::td_api::updateMessageUnreadReactions &update) {
  auto it = messages_.find(update.message_id_);
  if (it != messages_.end()) {
    it->second->message->unread_reactions_ = std::move(update.unread_reactions_);
    change_element(it->second.get());
  }
}

//@description A message with a live location was viewed. When the update is received, the client is supposed to update the live location
//@chat_id Identifier of the chat with the live location message @message_id Identifier of the message with live location
void ChatWindow::process_update(td::td_api::updateMessageLiveLocationViewed &update) {
}

}  // namespace tdcurses
