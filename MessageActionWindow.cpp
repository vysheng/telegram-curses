#include "MessageActionWindow.hpp"
#include "ChatWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/Markup.hpp"
#include "ChatManager.hpp"
#include "TdObjectsOutput.h"
#include "DebugInfoWindow.hpp"
#include "GlobalParameters.hpp"
#include <vector>
#include <unistd.h>

namespace tdcurses {

std::function<void()> MessageActionWindowBuilder::wrap_callback(std::function<void()> callback) {
  return [chat_window = chat_window_, id = chat_window_->window_unique_id(), cb = std::move(callback)]() {
    if (chat_window->window_unique_id() == id) {
      cb();
    }
  };
}

void MessageActionWindowBuilder::add_action_custom(std::string name, std::vector<windows::MarkupElement> markup,
                                                   std::function<void()> callback) {
  elements_.emplace_back(std::move(name), std::move(markup), wrap_callback(std::move(callback)));
}

void MessageActionWindowBuilder::add_action_user_info(std::string prefix, td::int64 user_id,
                                                      td::Slice custom_user_name) {
  auto user = chat_manager().get_user(user_id);

  Outputter out;
  out << "info " << prefix << " " << Color::Red;
  if (custom_user_name.size() || !user) {
    out << custom_user_name;
  } else {
    out << user;
  }
  out << Color::Revert;
  add_action_custom(out.as_str(), out.markup(),
                    [curses = tdcurses_, user_id]() { curses->show_user_info_window(user_id); });
}

void MessageActionWindowBuilder::add_action_user_chat_open(std::string prefix, td::int64 user_id,
                                                           td::Slice custom_user_name) {
  auto user = chat_manager().get_user(user_id);

  Outputter out;
  out << "open " << prefix << " " << Color::Red;
  if (custom_user_name.size() || !user) {
    out << custom_user_name;
  } else {
    out << user;
  }
  out << Color::Revert;
  auto callback = [curses = tdcurses_, user_id]() {
    auto req = td::make_tl_object<td::td_api::createPrivateChat>(user_id, true);
    curses->send_request(std::move(req), [curses](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
      if (R.is_ok()) {
        auto chat = R.move_as_ok();
        curses->open_chat(chat->id_);
      }
    });
  };
  add_action_custom(out.as_str(), out.markup(), std::move(callback));
}

void MessageActionWindowBuilder::add_action_chat_info(std::string prefix, td::int64 chat_id,
                                                      td::Slice custom_chat_name) {
  auto chat = chat_manager().get_chat(chat_id);

  Outputter out;
  out << "info " << prefix << " " << Color::Red;
  if (custom_chat_name.size() || !chat) {
    out << custom_chat_name;
  } else {
    out << chat;
  }
  out << Color::Revert;
  add_action_custom(out.as_str(), out.markup(),
                    [curses = tdcurses_, chat_id]() { curses->show_chat_info_window(chat_id); });
}

void MessageActionWindowBuilder::add_action_chat_open(std::string prefix, td::int64 chat_id,
                                                      td::Slice custom_chat_name) {
  auto chat = chat_manager().get_chat(chat_id);

  Outputter out;
  out << "open " << prefix << " " << Color::Red;
  if (custom_chat_name.size() || !chat) {
    out << custom_chat_name;
  } else {
    out << chat;
  }
  out << Color::Revert;
  auto callback = [curses = tdcurses_, chat_id]() { curses->show_chat_info_window(chat_id); };
  add_action_custom(out.as_str(), out.markup(), std::move(callback));
}

void MessageActionWindowBuilder::add_action_chat_by_username(std::string prefix, std::string username) {
  Outputter out;
  out << "view " << prefix << " " << Color::Red << username << Color::Revert;
  add_action_custom(out.as_str(), out.markup(), [curses = tdcurses_, username]() {
    auto req = td::make_tl_object<td::td_api::searchPublicChat>(username);
    curses->send_request(std::move(req), [curses](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
      if (R.is_ok()) {
        auto chat = R.move_as_ok();
        curses->show_chat_info_window(chat->id_);
      }
    });
  });
}

void MessageActionWindowBuilder::add_action_chat_open_by_username(std::string prefix, std::string username) {
  Outputter out;
  out << "open " << prefix << " " << Color::Red << username << Color::Revert;
  add_action_custom(out.as_str(), out.markup(), [curses = tdcurses_, username]() {
    auto req = td::make_tl_object<td::td_api::searchPublicChat>(username);
    curses->send_request(std::move(req), [curses](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
      if (R.is_ok()) {
        auto chat = R.move_as_ok();
        curses->open_chat(chat->id_);
      }
    });
  });
}

void MessageActionWindowBuilder::add_action_link(std::string link) {
  Outputter out;
  out << "open '" << Outputter::Underline(true) << link << Outputter::Underline(false) << "'";
  add_action_custom(out.as_str(), out.markup(), [link]() {
    std::string cmd = global_parameters().link_open_command();

    auto p = vfork();
    if (!p) {
      execlp(cmd.c_str(), cmd.c_str(), link.c_str(), (char *)NULL);
    }
  });
}

void MessageActionWindowBuilder::add_action_search_pattern(std::string pattern) {
  Outputter out;
  out << "search '" << Color::Navy << pattern << Color::Revert << "'";
  add_action_custom(out.as_str(), out.markup(),
                    [pattern, self = chat_window_]() { self->set_search_pattern(pattern); });
}

void MessageActionWindowBuilder::add_action_message_goto(std::string prefix, td::int64 chat_id, td::int64 message_id,
                                                         const td::td_api::message *message) {
  Outputter out;
  out << "go to " << prefix << Color::Navy << " message" << Color::Revert;
  if (message) {
    out << " " << *message->content_;
  }
  auto message_full_id = chat_window_->build_message_id(chat_id, message_id);
  add_action_custom(out.as_str(), out.markup(),
                    [chat = chat_window_, message_full_id]() { chat->seek(message_full_id); });
}

void MessageActionWindowBuilder::add_action_poll(td::int64 chat_id, td::int64 message_id,
                                                 const td::td_api::formattedText &text, bool is_selected,
                                                 td::int32 idx) {
  Outputter out;
  if (is_selected) {
    out << "retract vote from ";
  } else {
    out << "vote for ";
  }
  out << text;
  add_action_custom(out.as_str(), out.markup(), [idx, is_selected, chat_id, message_id, self = chat_window_]() {
    td::tl_object_ptr<td::td_api::setPollAnswer> req;
    if (is_selected) {
      req = td::make_tl_object<td::td_api::setPollAnswer>(chat_id, message_id, std::vector<td::int32>());
    } else {
      req = td::make_tl_object<td::td_api::setPollAnswer>(chat_id, message_id, std::vector<td::int32>{idx});
    }
    self->send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {});
  });
}

std::shared_ptr<windows::Window> MessageActionWindowBuilder::build() {
  auto selection_window = std::make_shared<windows::SelectionWindow>(std::move(elements_), nullptr);
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

  auto callback = std::make_unique<Callback>(tdcurses_, boxed_window);
  selection_window->set_callback(std::move(callback));
  return boxed_window;
}

void MessageActionWindowBuilder::add_action_forward(td::int64 chat_id, td::int64 message_id) {
  add_action_custom("forward", {}, [chat_id, message_id, curses = tdcurses_]() {
    curses->spawn_chat_selection_window([chat_id, message_id, curses](td::Result<std::shared_ptr<Chat>> R) {
      if (R.is_error()) {
        return;
      }
      auto dst = R.move_as_ok();

      //sendMessage chat_id:int53 message_thread_id:int53 reply_to:InputMessageReplyTo options:messageSendOptions reply_markup:ReplyMarkup input_message_content:InputMessageContent = Message;
      //inputMessageForwarded from_chat_id:int53 message_id:int53 in_game_share:Bool copy_options:messageCopyOptions = InputMessageContent;
      auto req = td::make_tl_object<td::td_api::sendMessage>(
          dst->chat_id(), 0, nullptr /*replay_to*/, nullptr /*options*/, nullptr /*reply_markup*/,
          td::make_tl_object<td::td_api::inputMessageForwarded>(chat_id, message_id, false, nullptr));
      curses->send_request(std::move(req), [](td::Result<td::tl_object_ptr<td::td_api::message>> R) {});
    });
  });
}

void MessageActionWindowBuilder::add_action_copy(std::string text) {
  add_action_custom("copy to clipboard", {}, [text = std::move(text)]() {
    std::string cmd = global_parameters().copy_command();

    auto p = vfork();
    if (!p) {
      execlp(cmd.c_str(), cmd.c_str(), "--", text.c_str(), NULL);
    }
  });
}

void MessageActionWindowBuilder::add_action_copy_primary(std::string text) {
  add_action_custom("copy to primary buffer", {}, [text = std::move(text)]() {
    std::string cmd = global_parameters().copy_command();

    auto p = vfork();
    if (!p) {
      execlp(cmd.c_str(), cmd.c_str(), "--primary", "--", text.c_str(), NULL);
    }
  });
}

void MessageActionWindowBuilder::add_action_open_file(std::string file_path) {
  add_action_custom("open document", {}, [file_path = std::move(file_path)]() {
    std::string cmd = global_parameters().file_open_command();

    auto p = vfork();
    if (!p) {
      execlp(cmd.c_str(), cmd.c_str(), "--", file_path.c_str(), NULL);
    }
  });
}

void MessageActionWindowBuilder::add_action_reply(td::int64 chat_id, td::int64 message_id) {
  add_action_custom("reply", {},
                    [chat_id, message_id, curses = tdcurses_]() { curses->open_compose_window(chat_id, message_id); });
}

/*
 *
 * DEBUG
 *
 */

void MessageActionWindowBuilder::add_action_message_debug_info(std::string prefix, td::int64 chat_id,
                                                               td::int64 message_id,
                                                               const td::td_api::message *message) {
  Outputter out;
  out << "debug info of " << prefix << Outputter::FgColor{Color::Navy} << " message"
      << Outputter::FgColor{Color::Revert};
  if (message) {
    out << " " << *message->content_;
  }
  auto message_full_id = chat_window_->build_message_id(chat_id, message_id);
  add_action_custom(out.as_str(), out.markup(), [curses = tdcurses_, chat = chat_window_, message_full_id]() {
    const auto &message = chat->get_message_as_message(message_full_id);
    if (!message) {
      return;
    }
    auto info_window = std::make_shared<DebugInfoWindow>();
    auto bordered_window =
        std::make_shared<windows::BorderedWindow>(info_window, windows::BorderedWindow::BorderType::Double);
    class Callback : public windows::ViewWindow::Callback {
     public:
      Callback(Tdcurses *root, std::shared_ptr<windows::Window> window) : root_(root), window_(std::move(window)) {
      }
      void on_answer(windows::ViewWindow *window) override {
        root_->del_popup_window(window_.get());
      }
      void on_abort(windows::ViewWindow *window) override {
        root_->del_popup_window(window_.get());
      }

     private:
      Tdcurses *root_;
      std::shared_ptr<windows::Window> window_;
    };
    info_window->set_callback(std::make_unique<Callback>(curses, bordered_window));
    info_window->create_text(*message);
    curses->add_popup_window(bordered_window, 1);
  });
}

}  // namespace tdcurses
