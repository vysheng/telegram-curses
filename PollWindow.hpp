#pragma once

#include "common-windows/MenuWindow.hpp"
#include "common-windows/MenuWindowCommon.hpp"
#include "common-windows/ErrorWindow.hpp"
#include "auto/td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include <set>

namespace tdcurses {

class PollWindow : public MenuWindowCommon {
 public:
  PollWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id, td::int64 message_id)
      : MenuWindowCommon(root, std::move(root_actor)), chat_id_(chat_id), message_id_(message_id) {
    request_message();
  }

  void request_message() {
    reqs_waiting_++;
    auto req = td::make_tl_object<td::td_api::getMessage>(chat_id_, message_id_);
    send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::message>> R) mutable {
      DROP_IF_DELETED(R);
      self->got_message(std::move(R));
    });
  }

  void request_message_properties() {
    reqs_waiting_++;
    auto req = td::make_tl_object<td::td_api::getMessageProperties>(chat_id_, message_id_);
    send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::messageProperties>> R) mutable {
      DROP_IF_DELETED(R);
      self->got_message_properties(std::move(R));
    });
  }

  void req_completed() {
    reqs_waiting_--;
    if (!reqs_waiting_) {
      process_message();
    }
  }

  void update_poll(td::tl_object_ptr<td::td_api::poll> poll);

  void got_message(td::Result<td::tl_object_ptr<td::td_api::message>> R) {
    if (R.is_error()) {
      spawn_replace_error_window(*this, PSTRING() << "failed to get message: " << R.move_as_error());
      return;
    }
    auto msg = R.move_as_ok();
    if (msg->content_->get_id() != td::td_api::messagePoll::ID) {
      spawn_replace_error_window(*this, "message is not poll");
      return;
    }
    auto p = td::move_tl_object_as<td::td_api::messagePoll>(std::move(msg->content_));
    update_poll(std::move(p->poll_));
    req_completed();
  }

  void got_message_properties(td::Result<td::tl_object_ptr<td::td_api::messageProperties>> R) {
    if (R.is_error()) {
      add_element("ERROR", R.move_as_error().to_string());
      return;
    }

    message_properties_ = R.move_as_ok();
    req_completed();
  }

  void process_message();
  void select(td::int32 idx);

 private:
  td::int32 reqs_waiting_{0};
  td::int64 chat_id_;
  td::int64 message_id_;
  std::shared_ptr<ElInfo> last_el_;
  std::set<td::int32> options_to_be_set_;

  td::tl_object_ptr<td::td_api::poll> poll_;
  td::tl_object_ptr<td::td_api::messageProperties> message_properties_;
  bool already_voted_{false};
  bool allow_multichoise_{false};
  bool is_quiz_{false};
};

}  // namespace tdcurses
