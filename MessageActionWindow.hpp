#pragma once

#include "td/utils/Slice.h"
#include "td/utils/common.h"
#include "auto/td/telegram/td_api.h"
#include "windows/Markup.hpp"
#include "windows/SelectionWindow.hpp"
#include "Tdcurses.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tdcurses {

class ChatWindow;

class MessageActionWindowBuilder {
 public:
  MessageActionWindowBuilder(Tdcurses *tdcurses, td::ActorId<Tdcurses> tdcurses_actor_id, ChatWindow *chat_window)
      : tdcurses_(tdcurses), tdcurses_actor_id_(std::move(tdcurses_actor_id)), chat_window_(chat_window) {
    CHECK(chat_window_);
  }
  std::function<void()> wrap_callback(std::function<void()> callback);
  void add_action_custom(std::string name, std::vector<windows::MarkupElement> markup, std::function<void()> callback);
  void add_action_custom(std::string name, std::function<void()> callback) {
    return add_action_custom(std::move(name), {}, std::move(callback));
  }
  void add_action_user_info(std::string prefix, td::int64 user_id, td::Slice custom_user_name = td::Slice());
  void add_action_user_chat_open(std::string prefix, td::int64 user_id, td::Slice custom_user_name = td::Slice());
  void add_action_chat_info(std::string prefix, td::int64 chat_id, td::Slice custom_chat_name = td::Slice());
  void add_action_chat_open(std::string prefix, td::int64 chat_id, td::Slice custom_chat_name = td::Slice());
  void add_action_chat_by_username(std::string prefix, std::string username);
  void add_action_chat_open_by_username(std::string prefix, std::string username);
  void add_action_link(std::string link);
  void add_action_search_pattern(std::string pattern);
  void add_action_message_goto(std::string prefix, td::int64 chat_id, td::int64 message_id,
                               const td::td_api::message *message);
  void add_action_message_debug_info(std::string prefix, td::int64 chat_id, td::int64 message_id,
                                     const td::td_api::message *message);
  void add_action_poll(td::int64 chat_id, td::int64 message_id, const td::td_api::formattedText &text, bool is_selected,
                       td::int32 idx);

  void add_action_forward(td::int64 chat_id, td::int64 message_id);
  void add_action_copy(std::string text);
  void add_action_copy_primary(std::string text);
  void add_action_open_file(std::string file_path);
  void add_action_open_file(const td::td_api::file &file) {
    if (file.local_->is_downloading_completed_) {
      add_action_open_file(file.local_->path_);
    }
  }
  void add_action_reply(td::int64 chat_id, td::int64 message_id);
  void add_action_reactions(td::int64 chat_id, td::int64 message_id);

  std::shared_ptr<windows::Window> build();

 private:
  Tdcurses *tdcurses_;
  td::ActorId<Tdcurses> tdcurses_actor_id_;
  ChatWindow *chat_window_;
  std::vector<windows::SelectionWindow::ElementInfo> elements_;
};

};  // namespace tdcurses
