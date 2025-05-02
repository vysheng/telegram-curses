#include "ReactionSelectionWindowNew.hpp"
#include "td/telegram/td_api.h"

namespace tdcurses {

void ReactionSelectionWindowNew::request_message() {
  auto req = td::make_tl_object<td::td_api::getMessage>(message_id_.chat_id, message_id_.message_id);
  send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::message>> R) mutable {
    DROP_IF_DELETED(R);
    self->got_message(std::move(R));
  });
}

void ReactionSelectionWindowNew::request_popular_reactions() {
  auto req = td::make_tl_object<td::td_api::getMessageAvailableReactions>(message_id_.chat_id, message_id_.message_id,
                                                                          width() / 5);
  send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::availableReactions>> R) mutable {
    DROP_IF_DELETED(R);
    self->got_message_available_reactions(std::move(R));
  });
}

}  // namespace tdcurses
