#include "CommonGroupsWindow.hpp"
#include "ChatInfoWindow.hpp"
#include "ChatManager.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/PadWindow.hpp"
#include "Outputter.hpp"
#include "windows/TextEdit.hpp"
#include <memory>

namespace tdcurses {

class Element : public windows::PadWindowElement {
 public:
  Element(std::shared_ptr<Chat> chat, size_t idx) : chat_(std::move(chat)), idx_(idx) {
  }
  td::int32 render(windows::PadWindow &root, TickitRenderBuffer *rb, bool is_selected) override {
    return render_plain_text(rb, chat_->title(), width(), 1, is_selected);
  }

  bool is_less(const PadWindowElement &other) const override {
    return idx_ < static_cast<const Element &>(other).idx_;
  }

  void handle_input(windows::PadWindow &root, TickitKeyEventInfo *info) override {
    if (info->type == TICKIT_KEYEV_KEY) {
      if (!strcmp(info->str, "Enter")) {
        static_cast<CommonGroupsWindow &>(root).spawn_submenu<ChatInfoWindow>(chat_);
      }
    }
  }

 private:
  std::shared_ptr<Chat> chat_;
  size_t idx_;
};

void CommonGroupsWindow::request_bottom_elements() {
  if (running_req_ || is_completed_) {
    return;
  }
  running_req_ = true;
  auto req = td::make_tl_object<td::td_api::getGroupsInCommon>(user_->user_id(), last_chat_id_, 100);
  send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::chats>> R) {
    DROP_IF_DELETED(R);
    if (R.is_ok()) {
      self->got_chats(std::move(R));
    }
  });
}

void CommonGroupsWindow::got_chats(td::Result<td::tl_object_ptr<td::td_api::chats>> R) {
  running_req_ = false;
  if (R.is_error()) {
    is_completed_ = true;
    return;
  }
  is_completed_ = true;
  auto chats = R.move_as_ok();
  if (!chats->chat_ids_.size()) {
    is_completed_ = true;
    return;
  }
  for (auto &chat_id : chats->chat_ids_) {
    auto chat = chat_manager().get_chat(chat_id);
    if (chat) {
      add_element(std::make_shared<Element>(chat, last_idx_++));
    }
  }
  last_chat_id_ = chats->chat_ids_.back();
  set_need_refresh();
}

}  // namespace tdcurses
