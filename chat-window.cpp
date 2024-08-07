#include "chat-window.h"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Random.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/Status.h"
#include "td/utils/overloaded.h"
#include "telegram-curses-output.h"
#include "windows/markup.h"
#include "windows/text.h"
#include "windows/selection-window.h"
#include "windows/window.h"
#include <limits>
#include <memory>
#include <tuple>
#include <memory>
#include <vector>
#include <unistd.h>
#include "td/utils/utf8.h"

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

void ChatWindow::show_message_actions() {
  std::vector<windows::SelectionWindow::ElementInfo> elements;
  auto cur_element = get_active_element();
  if (!cur_element) {
    return;
  }
  auto el = static_cast<Element *>(cur_element.get());

  auto add_user = [&](const std::string &prefix, td::int64 user_id) {
    auto user = ChatManager::instance->get_user(user_id);
    if (user) {
      {
        Outputter out;
        out << "info " << prefix << " " << Outputter::FgColor{Color::Red} << user << Outputter::FgColor{Color::Revert};
        elements.emplace_back(out.as_str(), out.markup(),
                              [curses = root(), user_id]() { curses->show_user_info_window(user_id); });
      }
      {
        Outputter out;
        out << "open " << prefix << " " << Outputter::FgColor{Color::Red} << user << Outputter::FgColor{Color::Revert};
        elements.emplace_back(out.as_str(), out.markup(), [self = this, curses = root(), user_id]() {
          auto req = td::make_tl_object<td::td_api::createPrivateChat>(user_id, true);
          self->send_request(std::move(req), [curses](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
            if (R.is_ok()) {
              auto chat = R.move_as_ok();
              curses->open_chat(chat->id_);
            }
          });
        });
      }
    } else {
      elements.emplace_back(PSTRING() << "info " << prefix << " <UNAVAILABLE>", std::vector<windows::MarkupElement>{},
                            []() {});
    }
  };

  auto add_chat = [&](const std::string &prefix, td::int64 chat_id) {
    auto chat = ChatManager::instance->get_chat(chat_id);
    if (chat) {
      {
        Outputter out;
        out << "info " << prefix << " " << Outputter::FgColor{Color::Red} << chat << Outputter::FgColor{Color::Revert};
        elements.emplace_back(out.as_str(), out.markup(),
                              [curses = root(), chat_id]() { curses->show_chat_info_window(chat_id); });
      }
      {
        Outputter out;
        out << "open " << prefix << " " << Outputter::FgColor{Color::Red} << chat << Outputter::FgColor{Color::Revert};
        elements.emplace_back(out.as_str(), out.markup(), [curses = root(), chat_id]() { curses->open_chat(chat_id); });
      }
    } else {
      elements.emplace_back(PSTRING() << "info " << prefix << " <UNAVAILABLE>", std::vector<windows::MarkupElement>{},
                            []() {});
    }
  };

  auto add_user_custom = [&](const std::string &prefix, td::Slice name, td::int64 user_id) {
    {
      Outputter out;
      out << "info " << prefix << " " << Outputter::FgColor{Color::Red} << name << Outputter::FgColor{Color::Revert};
      elements.emplace_back(out.as_str(), out.markup(),
                            [curses = root(), user_id]() { curses->show_user_info_window(user_id); });
    }
    {
      Outputter out;
      out << "open " << prefix << " " << Outputter::FgColor{Color::Red} << name << Outputter::FgColor{Color::Revert};
      elements.emplace_back(out.as_str(), out.markup(), [self = this, curses = root(), user_id]() {
        auto req = td::make_tl_object<td::td_api::createPrivateChat>(user_id, true);
        self->send_request(std::move(req), [curses](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
          if (R.is_ok()) {
            auto chat = R.move_as_ok();
            curses->open_chat(chat->id_);
          }
        });
      });
    }
  };

  auto add_chat_username = [&](const std::string &prefix, std::string username) {
    {
      Outputter out;
      out << "view " << prefix << " " << Outputter::FgColor{Color::Red} << username
          << Outputter::FgColor{Color::Revert};
      elements.emplace_back(out.as_str(), out.markup(), [self = this, curses = root(), username]() {
        auto req = td::make_tl_object<td::td_api::searchPublicChat>(username);
        self->send_request(std::move(req), [curses](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
          if (R.is_ok()) {
            auto chat = R.move_as_ok();
            curses->show_chat_info_window(chat->id_);
          }
        });
      });
    }
    {
      Outputter out;
      out << "open " << prefix << " " << Outputter::FgColor{Color::Red} << username
          << Outputter::FgColor{Color::Revert};
      elements.emplace_back(out.as_str(), out.markup(), [self = this, curses = root(), username]() {
        auto req = td::make_tl_object<td::td_api::searchPublicChat>(username);
        self->send_request(std::move(req), [curses](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
          if (R.is_ok()) {
            auto chat = R.move_as_ok();
            curses->open_chat(chat->id_);
          }
        });
      });
    }
  };

  auto add_link = [&](std::string link) {
    {
      Outputter out;
      out << "open '" << Outputter::Underline(true) << link << Outputter::Underline(false) << "'";
      elements.emplace_back(out.as_str(), out.markup(), [link]() {
        std::string cmd = "xdg-open";
        const char *args[3];
        args[0] = cmd.c_str();
        args[1] = link.c_str();
        args[2] = nullptr;

        auto p = vfork();
        if (!p) {
          execvp(cmd.c_str(), const_cast<char **>(args));
        }
      });
    }
  };

  auto add_search_pattern = [&](std::string pattern) {
    {
      Outputter out;
      out << "search '" << Outputter::FgColor{Color::Navy} << pattern << Outputter::FgColor{Color::Revert} << "'";
      elements.emplace_back(out.as_str(), out.markup(),
                            [pattern, self = this]() { self->set_search_pattern(pattern); });
    }
  };

  auto add_message = [&](const std::string &prefix, td::int64 message_id, const td::td_api::message *message) {
    Outputter out;
    out << "go to " << prefix << Outputter::FgColor{Color::Navy} << " message" << Outputter::FgColor{Color::Revert};
    if (message) {
      out << " " << *message->content_;
    }
    elements.emplace_back(out.as_str(), out.markup(), [curses = root(), chat_id = chat_id_, message_id]() {
      curses->seek_chat(chat_id, message_id);
    });
  };

  add_message("current", el->message->id_, el->message.get());

  if (el->message->is_outgoing_) {
    add_chat("destination", chat_id_);
  } else {
    if (el->message->sender_id_) {
      td::td_api::downcast_call(
          *el->message->sender_id_,
          td::overloaded([&](td::td_api::messageSenderUser &sender) { add_user("source", sender.user_id_); },
                         [&](td::td_api::messageSenderChat &sender) { add_chat("source", sender.chat_id_); }));
    }
  }

  if (el->message->reply_to_) {
    td::td_api::downcast_call(*el->message->reply_to_,
                              td::overloaded(
                                  [&](td::td_api::messageReplyToMessage &info) {
                                    if (info.chat_id_ == chat_id_) {
                                      auto it = messages_.find(info.message_id_);
                                      add_message("replied", info.message_id_,
                                                  (it == messages_.end()) ? nullptr : it->second->message.get());
                                    }
                                  },
                                  [&](td::td_api::messageReplyToStory &info) {}));
  }

  if (el->message->forward_info_ && el->message->forward_info_->origin_) {
    td::td_api::downcast_call(
        *el->message->forward_info_->origin_,
        td::overloaded(
            [&](td::td_api::messageOriginUser &sender) { add_user("forward source", sender.sender_user_id_); },
            [&](td::td_api::messageOriginChat &sender) { add_chat("forward source", sender.sender_chat_id_); },
            [&](td::td_api::messageOriginChannel &sender) { add_chat("forward source", sender.chat_id_); },
            [&](td::td_api::messageOriginHiddenUser &sender) {}));
  }

  if (el->message->via_bot_user_id_) {
    add_user("via bot ", el->message->via_bot_user_id_);
  }

  auto process_formatted_text = [&](td::td_api::formattedText &content) {
    auto &text = content.text_;
    auto get_text_slice = [&](size_t from, size_t to) -> td::Slice {
      if (from >= to) {
        return td::Slice();
      }
      const unsigned char *text_ptr = (const unsigned char *)text.data();
      auto text_start = text_ptr;
      size_t from_pos = text.size();
      size_t to_pos = text.size();
      size_t p = 0;
      while (*text_ptr) {
        if (from == p) {
          from_pos = text_ptr - text_start;
        }
        if (to == p) {
          to_pos = text_ptr - text_start;
          break;
        }
        td::uint32 code;
        text_ptr = td::next_utf8_unsafe(text_ptr, &code);
        p += (code <= 0xffff) ? 1 : 2;  // UTF16 codepoints =((
      }

      return td::Slice(text).remove_prefix(from_pos).truncate(to_pos - from_pos);
    };

    for (auto &ent : content.entities_) {
      td::td_api::downcast_call(
          *ent->type_,
          td::overloaded(
              [&](const td::td_api::textEntityTypeMention &e) {
                auto us = get_text_slice(ent->offset_, ent->offset_ + ent->length_);
                add_chat_username("mentioned chat", us.str());
              },
              [&](const td::td_api::textEntityTypeHashtag &e) {
                auto us = get_text_slice(ent->offset_, ent->offset_ + ent->length_);
                add_search_pattern(us.str());
              },
              [&](const td::td_api::textEntityTypeCashtag &) {
                auto us = get_text_slice(ent->offset_, ent->offset_ + ent->length_);
                add_search_pattern(us.str());
              },
              [&](const td::td_api::textEntityTypeBotCommand &) {},
              [&](const td::td_api::textEntityTypeUrl &) {
                auto us = get_text_slice(ent->offset_, ent->offset_ + ent->length_);
                add_link(us.str());
              },
              [&](const td::td_api::textEntityTypeEmailAddress &) {},
              [&](const td::td_api::textEntityTypePhoneNumber &) {},
              [&](const td::td_api::textEntityTypeBankCardNumber &) {}, [&](const td::td_api::textEntityTypeBold &) {},
              [&](const td::td_api::textEntityTypeItalic &) {},
              [&](const td::td_api::textEntityTypeUnderline &) {
                auto us = get_text_slice(ent->offset_, ent->offset_ + ent->length_);
                add_search_pattern(us.str());
              },
              [&](const td::td_api::textEntityTypeStrikethrough &) {},
              [&](const td::td_api::textEntityTypeSpoiler &) {}, [&](const td::td_api::textEntityTypeCode &) {},
              [&](const td::td_api::textEntityTypePre &) {}, [&](const td::td_api::textEntityTypePreCode &) {},
              [&](const td::td_api::textEntityTypeBlockQuote &e) {},
              [&](const td::td_api::textEntityTypeExpandableBlockQuote &) {},
              [&](const td::td_api::textEntityTypeTextUrl &e) { add_link(e.url_); },
              [&](const td::td_api::textEntityTypeMentionName &e) {
                auto us = get_text_slice(ent->offset_, ent->offset_ + ent->length_);
                add_user_custom("mentioned chat", us, e.user_id_);
              },
              [&](const td::td_api::textEntityTypeCustomEmoji &) {},
              [&](const td::td_api::textEntityTypeMediaTimestamp &) {}));
    }
  };

  if (el->message->content_) {
    if (el->message->content_->get_id() == td::td_api::messageText::ID) {
      auto &content = static_cast<td::td_api::messageText &>(*el->message->content_);
      process_formatted_text(*content.text_);
    } else if (el->message->content_->get_id() == td::td_api::messagePhoto::ID) {
      auto &content = static_cast<td::td_api::messagePhoto &>(*el->message->content_);
      if (content.caption_) {
        process_formatted_text(*content.caption_);
      }
    }
  }

  auto selection_window = std::make_shared<windows::SelectionWindow>(std::move(elements), nullptr);
  auto boxed_window =
      std::make_shared<windows::BorderedWindow>(selection_window, windows::BorderedWindow::BorderType::Double);

  class Callback : public windows::SelectionWindow::Callback {
   public:
    Callback(Tdcurses *curses, std::shared_ptr<windows::Window> window) : curses_(curses), window_(std::move(window)) {
    }

    void on_end() override {
      curses_->del_popup_window(window_.get());
    }

    void on_abort() override {
      curses_->del_popup_window(window_.get());
    }

   private:
    Tdcurses *curses_;
    std::shared_ptr<windows::Window> window_;
  };

  auto callback = std::make_unique<Callback>(root(), boxed_window);
  selection_window->set_callback(std::move(callback));
  root()->add_popup_window(boxed_window, 2);
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
      root()->open_compose_window();
      return;
    } else if (!strcmp(info->str, "I")) {
      show_message_actions();
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
      auto req = td::make_tl_object<td::td_api::viewMessages>(chat_id_, std::vector<td::int64>{e.message->id_}, nullptr,
                                                              false);
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
  if (running_req_bottom_ || messages_.size() == 0 || is_completed_bottom_) {
    return;
  }
  running_req_bottom_ = true;
  auto m = messages_.rbegin()->first;
  if (search_pattern_.size() == 0) {
    auto req = td::make_tl_object<td::td_api::getChatHistory>(chat_id_, m, -10, 11, false);
    send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
      received_bottom_elements(std::move(R));
    });
  } else {
    //searchChatMessages chat_id:int53 query:string sender_id:MessageSender from_message_id:int53 offset:int32 limit:int32 filter:SearchMessagesFilter message_thread_id:int53 saved_messages_topic_id:int53 = FoundChatMessages;
    auto req = td::make_tl_object<td::td_api::searchChatMessages>(
        chat_id_, search_pattern_, /* sender_id */ nullptr,
        /* from_message_id */ message_id ?: m, /* offset */ -10, /* limit */ 11,
        /*filter */ nullptr, /* message_thread_id */ 0, /* message_topic_id */ 0);
    send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R) {
      received_bottom_search_elements(std::move(R));
    });
  }
}
void ChatWindow::received_bottom_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
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
  td::int64 m;
  if (messages_.size() != 0) {
    m = messages_.begin()->first;
  } else {
    m = std::numeric_limits<td::int64>::max();
    is_completed_bottom_ = true;
  }
  if (search_pattern_.size() == 0) {
    auto req = td::make_tl_object<td::td_api::getChatHistory>(chat_id_, message_id ?: m, 0, 10, false);
    send_request(std::move(req),
                 [&](td::Result<td::tl_object_ptr<td::td_api::messages>> R) { received_top_elements(std::move(R)); });
  } else {
    //searchChatMessages chat_id:int53 query:string sender_id:MessageSender from_message_id:int53 offset:int32 limit:int32 filter:SearchMessagesFilter message_thread_id:int53 saved_messages_topic_id:int53 = FoundChatMessages;
    auto req = td::make_tl_object<td::td_api::searchChatMessages>(
        chat_id_, search_pattern_, /* sender_id */ nullptr,
        /* from_message_id */ message_id ?: m, /* offset */ 0, /* limit */ 10,
        /*filter */ nullptr, /* message_thread_id */ 0, /* message_topic_id */ 0);
    send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R) {
      received_top_search_elements(std::move(R));
    });
  }
}
void ChatWindow::received_top_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
  if (R.is_error()) {
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
  if (R.is_error()) {
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
    auto id = m->id_;
    auto it = messages_.find(id);
    if (it != messages_.end()) {
    } else {
      auto el = std::make_shared<Element>(std::move(m));
      messages_.emplace(id, el);
      add_file_message_pair(el->message->id_, get_file_id(*el->message));
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
    add_file_message_pair(el->message->id_, get_file_id(*el->message));
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
    del_file_message_pair(it->second->message->id_, get_file_id(*it->second->message));
    delete_element(it->second.get());
    messages_.erase(it);
    auto m = std::move(update.message_);
    auto id = m->id_;
    auto el = std::make_shared<Element>(std::move(m));
    messages_.emplace(id, el);
    add_file_message_pair(el->message->id_, get_file_id(*el->message));
    add_element(std::move(el));
  }
}

//@description A message failed to send. Be aware that some messages being sent can be irrecoverably deleted, in which case updateDeleteMessages will be received instead of this update
//@message Contains information about the message that failed to send @old_message_id The previous temporary message identifier @error_code An error code @error_message Error message
//updateMessageSendFailed message:message old_message_id:int53 error_code:int32 error_message:string = Update;
void ChatWindow::process_update(td::td_api::updateMessageSendFailed &update) {
  auto it = messages_.find(update.old_message_id_);
  if (it != messages_.end()) {
    del_file_message_pair(it->second->message->id_, get_file_id(*it->second->message));
    delete_element(it->second.get());
    messages_.erase(it);
    auto m = std::move(update.message_);
    auto id = m->id_;
    auto el = std::make_shared<Element>(std::move(m));
    messages_.emplace(id, el);
    add_file_message_pair(el->message->id_, get_file_id(*el->message));
    add_element(std::move(el));
  }
}

//@description The message content has changed @chat_id Chat identifier @message_id Message identifier @new_content New message content
//updateMessageContent chat_id:int53 message_id:int53 new_content:MessageContent = Update;
void ChatWindow::process_update(td::td_api::updateMessageContent &update) {
  auto it = messages_.find(update.message_id_);
  if (it != messages_.end()) {
    auto old_f = get_file_id(*it->second->message);
    it->second->message->content_ = std::move(update.new_content_);
    auto new_f = get_file_id(*it->second->message);
    if (old_f != new_f) {
      del_file_message_pair(update.message_id_, old_f);
      add_file_message_pair(update.message_id_, new_f);
    }
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

//@description A fact-check added to a message was changed
//@chat_id Chat identifier
//@message_id Message identifier
//@fact_check The new fact-check
//updateMessageFactCheck chat_id:int53 message_id:int53 fact_check:factCheck = Update;
void ChatWindow::process_update(td::td_api::updateMessageFactCheck &update) {
  auto it = messages_.find(update.message_id_);
  if (it != messages_.end()) {
    it->second->message->fact_check_ = std::move(update.fact_check_);
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

void ChatWindow::process_update(td::td_api::updateDeleteMessages &update) {
  if (!update.is_permanent_) {
    return;
  }
  for (auto x : update.message_ids_) {
    auto it = messages_.find(x);
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

void ChatWindow::update_file(td::int64 message_id, td::td_api::file &file) {
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
                         [id = message->id_, window](td::Result<td::tl_object_ptr<td::td_api::file>> R) {
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
  change_unique_id();
  file_id_2_messages_.clear();
  search_pattern_ = std::move(pattern);
  running_req_top_ = false;
  running_req_bottom_ = false;
  is_completed_top_ = false;
  is_completed_bottom_ = false;

  std::shared_ptr<Element> cur;
  if (search_pattern_ == "") {
    cur = std::static_pointer_cast<Element>(get_active_element());
  }

  PadWindow::clear();
  messages_.clear();

  if (cur) {
    messages_.emplace(cur->message->id_, cur);
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
  if (chat_id_ != chat_id) {
    return;
  }
  auto it = messages_.find(message_id);
  if (it != messages_.end()) {
    scroll_to_element(it->second.get(), true);
    set_search_pattern("");
  }
}

}  // namespace tdcurses
