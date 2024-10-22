#include "GroupMembersWindow.hpp"
#include "ChatInfoWindow.hpp"
#include "ChatManager.hpp"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "td/utils/overloaded.h"
#include "windows/PadWindow.hpp"
#include "Outputter.hpp"
#include "windows/TextEdit.hpp"
#include <memory>

namespace tdcurses {

namespace {
class Element : public windows::PadWindowElement {
 public:
  Element(std::shared_ptr<Chat> chat, size_t idx) : chat_(std::move(chat)), idx_(idx) {
  }
  Element(std::shared_ptr<User> chat, size_t idx) : user_(std::move(chat)), idx_(idx) {
  }
  td::int32 render(windows::PadWindow &root, TickitRenderBuffer *rb, bool is_selected) override {
    return render_plain_text(rb, title(), width(), 1, is_selected);
  }

  std::string title() const {
    if (chat_) {
      return chat_->title();
    }
    if (user_->last_name().size() > 0) {
      return user_->first_name() + " " + user_->last_name();
    } else {
      return user_->first_name();
    }
  }

  bool is_less(const PadWindowElement &other) const override {
    return idx_ < static_cast<const Element &>(other).idx_;
  }

  void handle_input(windows::PadWindow &root, TickitKeyEventInfo *info) override {
    if (info->type == TICKIT_KEYEV_KEY) {
      if (!strcmp(info->str, "Enter")) {
        if (chat_) {
          static_cast<GroupMembersWindow &>(root).spawn_submenu<ChatInfoWindow>(chat_);
        } else {
          static_cast<GroupMembersWindow &>(root).spawn_submenu<ChatInfoWindow>(user_);
        }
      }
    }
  }

 private:
  std::shared_ptr<Chat> chat_;
  std::shared_ptr<User> user_;
  size_t idx_;
};
}  // namespace

void GroupMembersWindow::request_bottom_elements() {
  if (running_req_ || is_completed_) {
    return;
  }
  running_req_ = true;
  switch (chat_->chat_type()) {
    case ChatType::SecretChat:
    case ChatType::User:
    case ChatType::Unknown:
      running_req_ = false;
      is_completed_ = true;
      return;
    case ChatType::Basicgroup: {
      auto req = td::make_tl_object<td::td_api::getBasicGroupFullInfo>(chat_->chat_base_id());
      send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::basicGroupFullInfo>> R) {
        DROP_IF_DELETED(R);
        if (R.is_ok()) {
          self->got_members(std::move(R));
        }
      });
    } break;
    case ChatType::Supergroup:
    case ChatType::Channel: {
      auto req = td::make_tl_object<td::td_api::getSupergroupMembers>(chat_->chat_base_id(), nullptr, offset_, 10);
      send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::chatMembers>> R) {
        DROP_IF_DELETED(R);
        if (R.is_ok()) {
          self->got_members(std::move(R));
        }
      });
    } break;
  }
}

void GroupMembersWindow::got_members(td::Result<td::tl_object_ptr<td::td_api::basicGroupFullInfo>> R) {
  running_req_ = false;
  if (R.is_error()) {
    is_completed_ = true;
    return;
  }
  is_completed_ = true;
  auto info = R.move_as_ok();
  for (auto &member : info->members_) {
    td::td_api::downcast_call(*member->member_id_, td::overloaded(
                                                       [&](td::td_api::messageSenderUser &user) {
                                                         auto u = chat_manager().get_user(user.user_id_);
                                                         if (u) {
                                                           add_element(std::make_shared<Element>(u, last_idx_++));
                                                         }
                                                       },
                                                       [&](td::td_api::messageSenderChat &chat) {
                                                         auto u = chat_manager().get_chat(chat.chat_id_);
                                                         if (u) {
                                                           add_element(std::make_shared<Element>(u, last_idx_++));
                                                         }
                                                       }));
  }
  set_need_refresh();
}

void GroupMembersWindow::got_members(td::Result<td::tl_object_ptr<td::td_api::chatMembers>> R) {
  running_req_ = false;
  if (R.is_error()) {
    is_completed_ = true;
    return;
  }
  auto res = R.move_as_ok();
  if (res->members_.size() == 0) {
    is_completed_ = true;
    return;
  }
  offset_ += (td::int32)res->members_.size();
  for (auto &member : res->members_) {
    td::td_api::downcast_call(*member->member_id_, td::overloaded(
                                                       [&](td::td_api::messageSenderUser &user) {
                                                         auto u = chat_manager().get_user(user.user_id_);
                                                         if (u) {
                                                           add_element(std::make_shared<Element>(u, last_idx_++));
                                                         }
                                                       },
                                                       [&](td::td_api::messageSenderChat &chat) {
                                                         auto u = chat_manager().get_chat(chat.chat_id_);
                                                         if (u) {
                                                           add_element(std::make_shared<Element>(u, last_idx_++));
                                                         }
                                                       }));
  }
  set_need_refresh();
}

}  // namespace tdcurses
