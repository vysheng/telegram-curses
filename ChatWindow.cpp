#include "ChatWindow.hpp"
#include "ChatSearchWindow.hpp"
#include "ErrorWindow.hpp"
#include "ReactionSelectionWindow.hpp"
#include "ReactionSelectionWindowNew.hpp"
#include "td/telegram/files/ResourceState.h"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Random.h"
#include "td/utils/Slice.h"
#include "td/utils/Status.h"
#include "td/utils/overloaded.h"
#include "TdObjectsOutput.h"
#include "FileManager.hpp"
#include "MessageInfoWindow.hpp"
#include "MessageProcess.hpp"
#include "MenuWindowEdit.hpp"
#include "CommandLineWindow.hpp"
#include "GlobalParameters.hpp"
#include "LoadingWindow.hpp"
#include "YesNoWindow.hpp"
#include "Action.hpp"
#include "windows/EditorWindow.hpp"
#include "windows/Markup.hpp"
#include "windows/Output.hpp"
#include "windows/TextEdit.hpp"
#include "windows/unicode.h"
#include <limits>
#include <memory>
#include <tuple>
#include <memory>
#include <vector>
#include "td/utils/utf8.h"

namespace tdcurses {

ChatWindow::ChatWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id)
    : TdcursesWindowBase(root, std::move(root_actor)), main_chat_id_(chat_id) {
  set_pad_to(PadTo::Bottom);
  scroll_last_line();
  send_open();

  auto chat = chat_manager().get_chat(chat_id);
  if (chat) {
    set_title(chat->title());
  }
}

void ChatWindow::send_open() {
  td::tl_object_ptr<td::td_api::openChat> req;
  mode_.visit(td::overloaded(
      [&](const ModeDefault &) { req = td::make_tl_object<td::td_api::openChat>(main_chat_id()); },
      [&](const ModeSearch &) { req = td::make_tl_object<td::td_api::openChat>(main_chat_id()); },
      [&](const ModeComments &m) { req = td::make_tl_object<td::td_api::openChat>(m.message_id.chat_id); }));
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
  create_menu_window<MessageInfoWindow>(root(), root_actor_id(), el->message->chat_id_, el->message->id_);
}

void ChatWindow::handle_input(const windows::InputEvent &info) {
  set_need_refresh();
  if (info == "T-Escape") {
    if (multi_message_selection_mode_) {
      multi_message_selection_mode_ = false;
      selected_messages_.clear();
      return;
    }
    set_mode(Mode{ModeDefault{}});
    return;
  } else if (info == "T-Enter") {
    show_message_actions();
    return;
  } else if (info == "C-q" || info == "C-Q" || info == "Q") {
    if (multi_message_selection_mode_) {
      multi_message_selection_mode_ = false;
      selected_messages_.clear();
      return;
    }
    set_mode(Mode{ModeDefault{}});
    return;
  } else if (info == "i") {
    auto chat = chat_manager().get_chat(main_chat_id_);
    if (chat && chat->permissions() && chat->permissions()->can_send_basic_messages_) {
      mode_.visit(td::overloaded([&](const ModeDefault &) { root()->open_compose_window(main_chat_id_, 0, 0, ""); },
                                 [&](const ModeSearch &) { root()->open_compose_window(main_chat_id_, 0, 0, ""); },
                                 [&](const ModeComments &m) {
                                   root()->open_compose_window(m.message_id.chat_id, m.thread_id,
                                                               m.message_id.message_id, "");
                                 }));
    }
    return;
  } else if (info == "/" || info == ":") {
    root()->command_line_window()->handle_input(info);
    return;
  } else if (info == "F") {
    root()->spawn_file_selection_window([self = this](td::Result<std::string> R) {
      if (R.is_error()) {
        return;
      }
      auto fname = R.move_as_ok();

      //sendMessage chat_id:int53 message_thread_id:int53 reply_to:InputMessageReplyTo options:messageSendOptions reply_markup:ReplyMarkup input_message_content:InputMessageContent = Message;
      //inputMessageForwarded from_chat_id:int53 message_id:int53 in_game_share:Bool copy_options:messageCopyOptions = InputMessageContent;
      auto req = td::make_tl_object<td::td_api::sendMessage>(
          self->main_chat_id(), 0, nullptr /*reply_to*/, nullptr /*options*/, nullptr /*reply_markup*/,
          td::make_tl_object<td::td_api::inputMessageDocument>(
              td::make_tl_object<td::td_api::inputFileLocal>(fname), nullptr /* thumbnail */,
              false /* disable type detection */, nullptr /* caption */));
      self->send_request(std::move(req), [](td::Result<td::tl_object_ptr<td::td_api::message>> R) {});
    });
    return;
  }
  windows::PadWindow::handle_input(info);
  update_visible();
}

void ChatWindow::update_visible() {
  auto els = get_visible_elements();
  if (els.size() > 0) {
    std::vector<td::int64> ids;
    td::int64 last_chat_id = 0;
    for (auto &el : els) {
      auto &e = static_cast<Element &>(*el);
      if (!last_chat_id) {
        last_chat_id = e.message->chat_id_;
      } else if (last_chat_id != e.message->chat_id_) {
        CHECK(ids.size() > 0);
        auto req = td::make_tl_object<td::td_api::viewMessages>(last_chat_id, std::move(ids), nullptr, false);
        send_request(std::move(req), {});
        last_chat_id = e.message->chat_id_;
        ids.clear();
      }
      ids.push_back(e.message->id_);
    }
    if (ids.size() > 0) {
      auto req = td::make_tl_object<td::td_api::viewMessages>(last_chat_id, std::move(ids), nullptr, false);
      send_request(std::move(req), {});
    }
  }
}

td::int32 ChatWindow::Element::render(windows::PadWindow &root, windows::WindowOutputter &rb,
                                      windows::SavedRenderedImagesDirectory &dir, bool is_selected) {
  auto &chat_window = static_cast<ChatWindow &>(root);
  auto d = chat_window.multi_message_selection_mode() ? 1 : 0;
  Outputter out;
  out.set_chat(static_cast<ChatWindow *>(&root));
  out << message;
  bool s = chat_window.multi_message_selection_mode_is_selected(ChatWindow::build_message_id(*message));
  auto r = windows::TextEdit::render(rb, width() - d, out.as_str(), 0, out.markup(), is_selected, false, &dir, d,
                                     s ? "*" : " ");
  return r;
}

void ChatWindow::request_bottom_elements_ex(td::int32 message_id) {
  if (running_req_bottom_ || messages_.size() == 0 || is_completed_bottom_ || !is_main_mode()) {
    return;
  }

  running_req_bottom_ = true;

  auto max_message_id = get_newest_message_id();

  mode_.visit(td::overloaded(
      [&](const ModeDefault &) {
        auto req = td::make_tl_object<td::td_api::getChatHistory>(max_message_id.chat_id, max_message_id.message_id,
                                                                  -10, 10, false);
        send_request(std::move(req), [self = this, pivot_message_id = max_message_id.message_id](
                                         td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
          self->received_bottom_elements(std::move(R), pivot_message_id);
        });
      },
      [&](const ModeSearch &m) {
        //searchChatMessages chat_id:int53 query:string sender_id:MessageSender from_message_id:int53 offset:int32 limit:int32 filter:SearchMessagesFilter message_thread_id:int53 saved_messages_topic_id:int53 = FoundChatMessages;
        auto req = td::make_tl_object<td::td_api::searchChatMessages>(
            max_message_id.chat_id, m.search_pattern, /* sender_id */ nullptr,
            /* from_message_id */ max_message_id.message_id, /* offset */ -10, /* limit */ 11,
            /*filter */ nullptr, /* message_thread_id */ 0, /* message_topic_id */ 0);
        send_request(std::move(req), [self = this, pivot_message_id = max_message_id.message_id](
                                         td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R) {
          self->received_bottom_search_elements(std::move(R), pivot_message_id);
        });
      },
      [&](const ModeComments &m) {
        //getMessageThreadHistory chat_id:int53 message_id:int53 from_message_id:int53 offset:int32 limit:int32 = Messages;
        auto req = td::make_tl_object<td::td_api::getMessageThreadHistory>(
            m.message_id.chat_id, m.message_id.message_id, max_message_id.message_id, /* offset */ -10,
            /* limit */ -11);
        send_request(std::move(req), [self = this, pivot_message_id = max_message_id.message_id](
                                         td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
          self->received_bottom_elements(std::move(R), pivot_message_id);
        });
      }));
}

void ChatWindow::received_bottom_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R,
                                          td::int64 pivot_message_id) {
  if (R.is_error() && R.error().code() == ErrorCodeWindowDeleted) {
    return;
  }
  CHECK(running_req_bottom_);
  running_req_bottom_ = false;
  R.ensure();
  auto res = R.move_as_ok();
  bool found_new = false;
  for (auto &m : res->messages_) {
    if (m->id_ > pivot_message_id) {
      found_new = true;
    }
  }
  if (!found_new) {
    is_completed_bottom_ = true;
  }
  add_messages(std::move(res->messages_));
}

void ChatWindow::received_bottom_search_elements(td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R,
                                                 td::int64 pivot_message_id) {
  if (R.is_error() && R.error().code() == ErrorCodeWindowDeleted) {
    return;
  }
  CHECK(running_req_bottom_);
  running_req_bottom_ = false;
  R.ensure();
  auto res = R.move_as_ok();
  bool found_new = false;
  for (auto &m : res->messages_) {
    if (m->id_ > pivot_message_id) {
      found_new = true;
    }
  }
  if (!found_new) {
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

  if (is_main_mode() && msg && msg->content_->get_id() == td::td_api::messageChatUpgradeFrom::ID) {
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
  mode_.visit(td::overloaded(
      [&](const ModeDefault &) {
        auto req = td::make_tl_object<td::td_api::getChatHistory>(min_message_id.chat_id, min_message_id.message_id, 0,
                                                                  10, false);
        send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
          received_top_elements(std::move(R));
        });
      },
      [&](const ModeSearch &m) {
        //searchChatMessages chat_id:int53 query:string sender_id:MessageSender from_message_id:int53 offset:int32 limit:int32 filter:SearchMessagesFilter message_thread_id:int53 saved_messages_topic_id:int53 = FoundChatMessages;
        auto req = td::make_tl_object<td::td_api::searchChatMessages>(
            min_message_id.chat_id, m.search_pattern, /* sender_id */ nullptr,
            /* from_message_id */ min_message_id.message_id, /* offset */ 0, /* limit */ 10,
            /*filter */ nullptr, /* message_thread_id */ 0, /* message_topic_id */ 0);
        send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R) {
          received_top_search_elements(std::move(R));
        });
      },
      [&](const ModeComments &m) {
        //getMessageThreadHistory chat_id:int53 message_id:int53 from_message_id:int53 offset:int32 limit:int32 = Messages;
        auto req = td::make_tl_object<td::td_api::getMessageThreadHistory>(
            m.message_id.chat_id, m.message_id.message_id, min_message_id.message_id, /* offset */ 0, /* limit */ 10);
        send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
          received_top_elements(std::move(R));
        });
      }));
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
  bool need_drop = false;
  mode_.visit(td::overloaded([&](const ModeDefault &) { need_drop = (update.message_->chat_id_ != main_chat_id_); },
                             [&](const ModeSearch &) { need_drop = true; },
                             [&](const ModeComments &m) {
                               need_drop = (update.message_->chat_id_ != m.message_id.chat_id) ||
                                           (update.message_->message_thread_id_ != m.thread_id);
                             }));
  if (need_drop) {
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
  set_need_refresh();
}

void ChatWindow::process_update_sent_message(td::tl_object_ptr<td::td_api::message> message) {
  if (!is_completed_bottom_) {
    return;
  }
  bool need_drop = false;
  mode_.visit(td::overloaded([&](const ModeDefault &) { need_drop = (message->chat_id_ != main_chat_id_); },
                             [&](const ModeSearch &) { need_drop = true; },
                             [&](const ModeComments &m) {
                               need_drop = (message->chat_id_ != m.message_id.chat_id) ||
                                           (message->message_thread_id_ != m.thread_id);
                             }));
  if (need_drop) {
    return;
  }
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
void ChatWindow::process_file_update(const td::td_api::updateFile &update) {
  auto it = file_id_2_messages_.find(update.file_->id_);
  if (it != file_id_2_messages_.end()) {
    for (auto x : it->second.messages) {
      update_file(x, *update.file_);
    }
  }
}

void ChatWindow::process_update(td::td_api::updateChatAction &update) {
}

td::int32 ChatWindow::get_file_id(const td::td_api::message &message) {
  auto f = message_get_file(message);
  return f ? f->id_ : 0;
}

void ChatWindow::update_message_file(td::td_api::message &message, const td::td_api::file &file_in) {
  auto file = clone_td_file(file_in);
  auto f = message_get_file_object_ref(message);
  if (f && (*f)->id_ == file->id_) {
    *f = std::move(file);
  }
}

void ChatWindow::update_file(MessageId message_id, const td::td_api::file &file) {
  auto it = messages_.find(message_id);
  if (it != messages_.end()) {
    update_message_file(*it->second->message, file);
    change_element(it->second.get());
  }
}

void ChatWindow::del_file_message_pair(MessageId msg_id, td::int32 file_id) {
  if (!file_id) {
    return;
  }
  auto it = file_id_2_messages_.find(file_id);
  CHECK(it != file_id_2_messages_.end());
  it->second.messages.erase(msg_id);
  if (it->second.messages.size() == 0) {
    file_manager().unsubscribe_from_file_updates(file_id, it->second.subscription_id);
    file_id_2_messages_.erase(it);
  }
}

void ChatWindow::add_file_message_pair(MessageId msg_id, td::int32 file_id) {
  if (!file_id) {
    return;
  }
  auto it = file_id_2_messages_.find(file_id);
  if (it == file_id_2_messages_.end()) {
    auto subscription_id = file_manager().subscribe_to_file_updates(
        file_id, [ptr = this](const td::td_api::updateFile &update) { ptr->process_file_update(update); });
    it = file_id_2_messages_.emplace(file_id, subscription_id).first;
  }
  it->second.messages.emplace(msg_id);
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

void ChatWindow::Element::handle_input(PadWindow &root, const windows::InputEvent &info) {
  auto &chat_window = static_cast<ChatWindow &>(root);
  if (info == "T-Enter") {
    run(&chat_window);
    return;
  } else if (info == " ") {
    if (!chat_window.multi_message_selection_mode_) {
      chat_window.multi_message_selection_mode_ = true;
      chat_window.on_resize(chat_window.height(), chat_window.width(), chat_window.height(), chat_window.width());
      chat_window.selected_messages_.clear();
    }
    auto msg_id = message_id();
    if (chat_window.selected_messages_.count(msg_id)) {
      chat_window.selected_messages_.erase(msg_id);
      if (chat_window.selected_messages_.size() == 0) {
        chat_window.multi_message_selection_mode_ = false;
        chat_window.on_resize(chat_window.height(), chat_window.width(), chat_window.height(), chat_window.width());
      }
    } else {
      chat_window.selected_messages_.insert(msg_id);
    }
    return;
  } else if (info == "v") {
    auto file = message_get_file(*message);
    if (file && file->local_->is_downloading_completed_) {
      global_parameters().open_document(file->local_->path_);
    } else if (file) {
      chat_window.send_request(td::make_tl_object<td::td_api::downloadFile>(file->id_, 20, 0, 0, false),
                               [](td::Result<td::tl_object_ptr<td::td_api::file>> R) {});
    }
  } else if (info == "r") {
    chat_window.root()->open_compose_window(chat_window.main_chat_id(), 0, message_id().message_id, "");
  } else if (info == "q") {
    if (message->content_->get_id() == td::td_api::messageText::ID) {
      auto ftext = clone_tl_object(*static_cast<const td::td_api::messageText &>(*message->content_).text_);

      auto R = chat_window.run_request_sync(td::make_tl_object<td::td_api::getMarkdownText>(std::move(ftext)));

      if (R.is_error()) {
        return;
      }

      auto text = R.move_as_ok()->text_;
      spawn_text_edit_window(chat_window, "select text to quote", text,
                             [&chat_window, message_id = message_id()](td::Result<std::string> res) {
                               if (res.is_error()) {
                                 return;
                               }
                               chat_window.root()->open_compose_window(message_id.chat_id, 0, message_id.message_id,
                                                                       res.move_as_ok());
                             });
    }
  } else if (info == "e") {
    if (message->is_outgoing_ && message->content_->get_id() == td::td_api::messageText::ID) {
      chat_window.root()->open_edit_window(message_id().chat_id, message_id().message_id);
    }
  } else if (info == "I") {
    create_menu_window<MessageInfoWindow>(chat_window.root(), chat_window.root_actor_id(), message->chat_id_,
                                          message->id_);
  } else if (info == "L") {
    create_menu_window<ReactionSelectionWindowNew>(chat_window.root(), chat_window.root_actor_id(), message->chat_id_,
                                                   message->id_);
  } else if (info == "y" || info == "Y") {
    if (!chat_window.multi_message_selection_mode()) {
      auto text = message_get_formatted_text(*message);
      if (text) {
        global_parameters().copy_to_buffer(text->text_, info.get_utf8_str()[0] == 'y');
      }
    } else {
      Outputter out;
      out.set_chat(&chat_window);
      for (auto &m : chat_window.selected_messages_) {
        auto msg = chat_window.get_message(m);
        if (msg) {
          out << *msg->message << "\n";
        }
      }
      global_parameters().copy_to_buffer(out.as_str(), info.get_utf8_str()[0] == 'y');
      chat_window.clear_multi_message_selection_mode();
    }
  } else if (info == "f") {
    std::vector<MessageId> msg_ids;
    if (chat_window.multi_message_selection_mode()) {
      for (auto &m : chat_window.selected_messages_) {
        msg_ids.push_back(m);
      }
    } else {
      msg_ids.push_back(message_id());
    }
    spawn_chat_search_window(
        chat_window, ChatSearchWindow::Mode::Local,
        [msg_ids = msg_ids, self = &chat_window, self_id = chat_window.window_unique_id(),
         curses = chat_window.root()](td::Result<std::shared_ptr<Chat>> R) {
          if (R.is_error() || !curses->window_exists(self_id)) {
            return;
          }

          auto dst = R.move_as_ok();

          std::vector<td::int64> mids;
          for (auto &m : msg_ids) {
            mids.emplace_back(m.message_id);
          }

          auto from_chat_id = msg_ids[0].chat_id;
          Outputter out;
          out << "forward " << mids.size() << " messages to chat " << dst;

          //forwardMessages chat_id:int53 message_thread_id:int53 from_chat_id:int53 message_ids:vector<int53> options:messageSendOptions send_copy:Bool remove_caption:Bool = Messages;
          auto req = td::make_tl_object<td::td_api::forwardMessages>(dst->chat_id(), 0, from_chat_id, std::move(mids),
                                                                     nullptr, false, false);
          spawn_yes_no_window_and_loading_windows(
              *self, out.as_str(), out.markup(), true, "forwarding...", {}, std::move(req),
              [dst, curses = self->root(), self](td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
                DROP_IF_DELETED(R);
                if (R.is_error()) {
                  spawn_error_window(*self, PSTRING() << "deleting messages failed: " << R.move_as_error(), {});
                  return;
                }
                curses->open_chat(dst->chat_id());
              });
        });
    chat_window.clear_multi_message_selection_mode();
    return;
  } else if (info == "d" || info == "D") {
    std::vector<MessageId> msg_ids;
    if (chat_window.multi_message_selection_mode()) {
      for (auto &m : chat_window.selected_messages_) {
        msg_ids.push_back(m);
      }
    } else {
      msg_ids.push_back(message_id());
    }
    std::vector<td::int64> mids;
    for (auto &m : msg_ids) {
      mids.emplace_back(m.message_id);
    }
    auto from_chat_id = msg_ids[0].chat_id;
    bool for_all = (info.get_utf8_str()[0] == 'd');
    Outputter out;
    out << "Delete " << mids.size() << " messages " << (for_all ? "for all members" : "only for yourself") << "?";
    //deleteMessages chat_id:int53 message_ids:vector<int53> revoke:Bool = Ok;
    auto req = td::make_tl_object<td::td_api::deleteMessages>(from_chat_id, std::move(mids), for_all);
    spawn_yes_no_window_and_loading_windows(
        chat_window, out.as_str(), out.markup(), true, "deleting...", {}, std::move(req),
        [self = &chat_window](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
          DROP_IF_DELETED(R);
          if (R.is_error()) {
            spawn_error_window(*self, PSTRING() << "deleting messages failed: " << R.move_as_error(), {});
            return;
          }
        });
    chat_window.clear_multi_message_selection_mode();
    return;
  }
}

void ChatWindow::clear_file_subscriptions() {
  for (auto &x : file_id_2_messages_) {
    file_manager().unsubscribe_from_file_updates(x.first, x.second.subscription_id);
  }
  file_id_2_messages_.clear();
}

void ChatWindow::set_mode(ChatWindow::Mode mode) {
  if (mode_ == mode) {
    return;
  }
  std::shared_ptr<Element> cur;
  if (is_main_mode(mode) && is_search_mode(mode_)) {
    cur = std::static_pointer_cast<Element>(get_active_element());
  }

  PadWindow::clear();
  change_unique_id();

  clear_file_subscriptions();
  mode_ = std::move(mode);
  running_req_top_ = false;
  running_req_bottom_ = false;
  is_completed_top_ = false;
  is_completed_bottom_ = !is_main_mode();

  messages_.clear();

  if (cur) {
    messages_.emplace(cur->message_id(), cur);
    add_file_message_pair(cur->message_id(), get_file_id(*cur->message));
    add_element(cur);
    unglue();
  }

  set_need_refresh();
  request_top_elements_ex(0);
  if (cur) {
    request_bottom_elements_ex(0);
  }

  send_open();

  root()->update_status_line();
}

void ChatWindow::seek(td::int64 chat_id, td::int64 message_id) {
  return seek(build_message_id(chat_id, message_id));
}

void ChatWindow::seek(MessageId message_id) {
  auto it = messages_.find(message_id);
  if (it != messages_.end()) {
    scroll_to_element(it->second.get(), ScrollMode::Minimal);
    set_mode(Mode{ModeDefault{}});
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
