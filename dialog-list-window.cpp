#include "dialog-list-window.h"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "telegram-curses-output.h"
#include "td/utils/Random.h"
#include "td/utils/Status.h"
#include <memory>

namespace tdcurses {

td::int32 DialogListWindow::Element::render(windows::PadWindow &root, TickitRenderBuffer *rb, bool is_selected) {
  Outputter out;
  std::string prefix;
  if (unread_count() > 0) {
    if (default_disable_notification()) {
      out << Color::Grey << (unread_count() > 9 ? 9 : unread_count()) << Color::Revert << " ";
    } else {
      out << Color::Red << (unread_count() > 9 ? 9 : unread_count()) << Color::Revert << " ";
    }
  } else if (is_marked_unread()) {
    out << Color::Red << "\xe2\x80\xa2" << Color::Revert << " ";
  } else {
    out << "  ";
  }
  out << title();
  return render_plain_text(rb, out.as_cslice(), out.markup(), width(), 1, is_selected);
}

void DialogListWindow::request_bottom_elements() {
  if (running_req_ || is_completed_) {
    return;
  }
  LOG(WARNING) << "request new chats";
  running_req_ = true;

  send_request(td::make_tl_object<td::td_api::loadChats>(nullptr, 10),
               [&](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { received_bottom_elements(std::move(R)); });
}

void DialogListWindow::received_bottom_elements(td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
  LOG(WARNING) << "received answer";
  //running_req_ = false;
  if (R.is_error() && R.error().code() == 404) {
    LOG(WARNING) << "answer is full";
    is_completed_ = true;
  }
}

void DialogListWindow::process_update(td::td_api::updateNewChat &update) {
  auto chat = get_chat(update.chat_->id_);
  if (!chat) {
    auto ptr = std::make_shared<Element>(std::move(update.chat_));
    add_chat(ptr);
    if (ptr->cur_order() > 0) {
      add_element(ptr);
    }
    return;
  }

  auto el = std::static_pointer_cast<Element>(std::move(chat));
  change_element(el, [&]() {
    el->full_update(std::move(update.chat_));
    el->update_order();
  });
}

void DialogListWindow::process_update(td::td_api::updateChatLastMessage &update) {
  auto chat = get_chat(update.chat_id_);
  if (!chat) {
    return;
  }

  auto el = std::static_pointer_cast<Element>(std::move(chat));
  change_element(el, [&]() {
    ChatManager::process_update(update);
    el->update_order();
  });
}

void DialogListWindow::process_update(td::td_api::updateChatPosition &update) {
  auto chat = get_chat(update.chat_id_);
  if (!chat) {
    return;
  }

  auto el = std::static_pointer_cast<Element>(std::move(chat));
  change_element(el, [&]() {
    ChatManager::process_update(update);
    el->update_order();
  });
}

void DialogListWindow::handle_input(TickitKeyEventInfo *info) {
  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "Enter")) {
      auto a = get_active_element();
      if (a) {
        auto el = std::static_pointer_cast<Element>(std::move(a));
        root()->open_chat(el->chat_id());
      }
      return;
    }
  }
  return PadWindow::handle_input(info);
}

}  // namespace tdcurses
