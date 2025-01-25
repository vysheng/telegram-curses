#pragma once

#include "MenuWindowCommon.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include <memory>
#include <set>

namespace tdcurses {

class MessageInfoWindow : public MenuWindowCommon {
 public:
  MessageInfoWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id, td::int64 message_id)
      : MenuWindowCommon(root, std::move(root_actor)), chat_id_(chat_id), message_id_(message_id) {
    request_message();
  }

  ~MessageInfoWindow();

  void request_message() {
    auto req = td::make_tl_object<td::td_api::getMessage>(chat_id_, message_id_);
    send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::message>> R) mutable {
      DROP_IF_DELETED(R);
      self->got_message(std::move(R));
    });
  }

  void got_message(td::Result<td::tl_object_ptr<td::td_api::message>> R) {
    if (R.is_error()) {
      add_element("ERROR", R.move_as_error().to_string());
      return;
    }
    message_ = R.move_as_ok();
    auto req = td::make_tl_object<td::td_api::getMessageProperties>(chat_id_, message_id_);
    send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::messageProperties>> R) mutable {
      DROP_IF_DELETED(R);
      self->got_message_properties(std::move(R));
    });
  }

  void got_message_properties(td::Result<td::tl_object_ptr<td::td_api::messageProperties>> R) {
    if (R.is_error()) {
      add_element("ERROR", R.move_as_error().to_string());
      return;
    }

    message_properties_ = R.move_as_ok();

    if (!message_properties_->can_get_message_thread_) {
      process_message();
      return;
    }
    auto req = td::make_tl_object<td::td_api::getMessageThread>(chat_id_, message_id_);
    send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::messageThreadInfo>> R) mutable {
      DROP_IF_DELETED(R);
      self->got_message_thread_info(std::move(R));
    });
  }

  void got_message_thread_info(td::Result<td::tl_object_ptr<td::td_api::messageThreadInfo>> R) {
    if (R.is_ok()) {
      message_thread_info_ = R.move_as_ok();
    }

    process_message();
  }

  void process_message();

 private:
  void add_action_user_chat_info(td::int64 user_id, td::Slice custom_user_name = td::Slice());
  void add_action_user_chat_open(td::int64 user_id, td::Slice custom_user_name = td::Slice());
  void add_action_chat_info(td::int64 chat_id, td::Slice custom_chat_name = td::Slice());
  void add_action_chat_open(td::int64 chat_id, td::Slice custom_chat_name = td::Slice());
  void add_action_chat_info_by_username(std::string username);
  void add_action_chat_open_by_username(std::string username);
  void add_action_link(std::string link);
  void add_action_search_pattern(std::string pattern);
  void add_action_message_goto(td::int64 chat_id, td::int64 message_id, const td::td_api::message *message);
  void add_action_message_debug_info(td::int64 chat_id, td::int64 message_id, const td::td_api::message *message);
  void add_action_poll(td::int64 chat_id, td::int64 message_id, const td::td_api::formattedText &text, bool is_selected,
                       td::int32 idx);
  void add_action_forward(td::int64 chat_id, td::int64 message_id);
  void add_action_copy(std::string text);
  void add_action_copy_primary(std::string text);
  void add_action_download_file(const td::td_api::file &file);
  void add_action_open_file(std::string file_path);
  void add_action_open_file(const td::td_api::file &file) {
    if (file.local_->is_downloading_completed_) {
      add_action_open_file(file.local_->path_);
    } else {
      add_action_download_file(file);
    }
  }
  void add_action_reply(td::int64 chat_id, td::int64 message_id);
  void add_action_reactions(td::int64 chat_id, td::int64 message_id);
  void add_action_delete_message(td::int64 chat_id, td::int64 message_id);
  void add_action_view_thread(td::int64 chat_id, td::int64 message_id);

  void handle_file_update(const td::td_api::updateFile &);

  td::int64 chat_id_;
  td::int64 message_id_;
  td::tl_object_ptr<td::td_api::message> message_;
  td::tl_object_ptr<td::td_api::messageProperties> message_properties_;
  td::tl_object_ptr<td::td_api::messageThreadInfo> message_thread_info_;
  std::map<td::int64, std::pair<td::int64, std::shared_ptr<ElInfo>>> subscription_ids_;
  std::shared_ptr<ElInfo> open_file_el_;
};

}  // namespace tdcurses
