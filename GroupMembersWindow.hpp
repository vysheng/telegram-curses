#pragma once

#include "common-windows/MenuWindowPad.hpp"
#include "managers/ChatManager.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include <memory>

namespace tdcurses {

class GroupMembersWindow : public MenuWindowPad {
 public:
  GroupMembersWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::shared_ptr<Chat> chat)
      : MenuWindowPad(root, std::move(root_actor)), chat_(std::move(chat)) {
    CHECK(chat_);
    set_pad_to(PadTo::Top);
    scroll_first_line();
    set_need_refresh();

    set_title(PSTRING() << "groups members of " << chat_->title());
  }

  void request_bottom_elements() override;
  void got_members(td::Result<td::tl_object_ptr<td::td_api::basicGroupFullInfo>> R);
  void got_members(td::Result<td::tl_object_ptr<td::td_api::chatMembers>> R);

 private:
  std::shared_ptr<Chat> chat_;
  td::int64 last_chat_id_{0};
  bool running_req_{false};
  bool is_completed_{false};
  size_t last_idx_{0};
  td::int32 offset_{0};
};

}  // namespace tdcurses
