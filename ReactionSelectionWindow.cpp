#include "ReactionSelectionWindow.hpp"

namespace tdcurses {

void ReactionSelectionWindow::request_message() {
  auto req = td::make_tl_object<td::td_api::getMessage>(message_id_.chat_id, message_id_.message_id);
  send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::message>> R) mutable {
    DROP_IF_DELETED(R);
    self->got_message(std::move(R));
  });
}

}  // namespace tdcurses
