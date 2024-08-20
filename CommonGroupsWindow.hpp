#pragma once

#include "MenuWindow.hpp"
#include "ChatManager.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include <memory>

namespace tdcurses {

class CommonGroupsWindow : public MenuWindow {
 public:
  CommonGroupsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::shared_ptr<User> user)
      : MenuWindow(root, std::move(root_actor)), user_(std::move(user)) {
    CHECK(user_);
    set_pad_to(PadTo::Top);
    scroll_first_line();
    set_need_refresh();

    set_title(PSTRING() << "common groups with " << user_->first_name() << " " << user_->last_name());
  }

  void request_bottom_elements() override;
  void got_chats(td::Result<td::tl_object_ptr<td::td_api::chats>> R);

  static MenuWindowSpawnFunction spawn_function(std::shared_ptr<User> user) {
    return [user](Tdcurses *root, td::ActorId<Tdcurses> root_id) -> std::shared_ptr<MenuWindow> {
      return std::make_shared<CommonGroupsWindow>(root, root_id, user);
    };
  }

 private:
  std::shared_ptr<User> user_;
  td::int64 last_chat_id_{0};
  bool running_req_{false};
  bool is_completed_{false};
  size_t last_idx_{0};
};

}  // namespace tdcurses
