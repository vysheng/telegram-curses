#include "ChatSearchWindow.hpp"
#include "td/telegram/StickerType.h"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Promise.h"
#include "td/utils/Random.h"
#include "td/utils/Status.h"
#include "windows/EditorWindow.hpp"
#include "windows/Markup.hpp"
#include "windows/Output.hpp"
#include "windows/TextEdit.hpp"
#include <memory>
#include <vector>

namespace tdcurses {

void ChatSearchWindow::build_subwindows() {
  editor_window_ = std::make_shared<windows::OneLineInputWindow>(">", false, nullptr);
  editor_window_->resize(width(), 1);
}

void ChatSearchWindow::handle_input(const windows::InputEvent &info) {
  if (info == "T-Up") {
    if (cur_selected_ > 0) {
      cur_selected_--;
    }
  } else if (info == "T-Down") {
    if (cur_selected_ < found_chats()) {
      cur_selected_++;
    }
  } else if (info == "T-Enter" || info == "M-T-Enter") {
    if (cur_selected_ != 0) {
      callback_->on_answer(*this, get_found_chat(cur_selected_ - 1));
    }
  } else if (info == "T-Tab") {
    cur_selected_++;
    if (cur_selected_ > found_chats()) {
      cur_selected_ = 0;
    }
  } else if (info == "T-Escape") {
    callback_->on_abort(*this);
  } else if (cur_selected_ == 0) {
    editor_window_->handle_input(info);
  }

  try_run_request();
  set_need_refresh();
}

void ChatSearchWindow::try_run_request() {
  if (running_request_) {
    return;
  }
  auto text = editor_window_->export_data();
  if (text == last_request_text_) {
    return;
  }
  last_request_text_ = std::move(text);

  auto request_chats = [&](bool is_local) {
    running_request_++;
    auto P = td::PromiseCreator::lambda([self = this, is_local](td::Result<td::tl_object_ptr<td::td_api::chats>> R) {
      if (R.is_ok() || R.error().code() != ErrorCodeWindowDeleted) {
        if (is_local) {
          R.ensure();
        }
        if (R.is_ok()) {
          self->got_chats(R.move_as_ok(), is_local);
        } else {
          self->failed_to_get_chats(R.move_as_error(), is_local);
        }
      }
    });
    if (is_local) {
      auto req = td::make_tl_object<td::td_api::searchChats>(last_request_text_, height() - 1);
      send_request(std::move(req), std::move(P));
    } else {
      auto req = td::make_tl_object<td::td_api::searchPublicChats>(last_request_text_);
      send_request(std::move(req), std::move(P));
    }
  };

  switch (mode_) {
    case Mode::Local:
      request_chats(true);
      break;
    case Mode::Global:
      request_chats(false);
      break;
    case Mode::Both:
      request_chats(true);
      request_chats(false);
      break;
  }
}

void ChatSearchWindow::got_chats(td::tl_object_ptr<td::td_api::chats> res, bool is_local) {
  CHECK(running_request_);
  running_request_--;

  auto &chats = (is_local ? found_chats_local_ : found_chats_global_);
  chats.clear();
  for (auto c : res->chat_ids_) {
    auto chat = chat_manager().get_chat(c);
    if (chat) {
      chats.push_back(chat);
    }
  }
  if (cur_selected_ > found_chats()) {
    cur_selected_ = found_chats();
  }
  set_need_refresh();
  try_run_request();
}

void ChatSearchWindow::failed_to_get_chats(td::Status error, bool is_local) {
  CHECK(running_request_);
  running_request_ = false;

  auto &chats = (is_local ? found_chats_local_ : found_chats_global_);
  chats.clear();
  if (cur_selected_ > found_chats()) {
    cur_selected_ = found_chats();
  }
  set_need_refresh();
  try_run_request();
}

void ChatSearchWindow::render(windows::WindowOutputter &rb, bool force) {
  editor_window_->render(rb, force);

  if (cur_selected_ != 0) {
    rb.cursor_move_yx(0, 0, windows::WindowOutputter::CursorShape::None);
  }

  td::uint32 i = 0;
  for (i = 0; i < (td::uint32)height() - 1 && i < found_chats(); i++) {
    rb.translate(i + 1, 0);
    bool is_selected = i + 1 == cur_selected_;
    windows::TextEdit::render(rb, width(), get_found_chat(i)->title(), 0, std::vector<windows::MarkupElement>(),
                              is_selected, false);
    rb.untranslate(i + 1, 0);
  }

  if (i + 1 < (td::uint32)height()) {
    rb.erase_rect(i + 1, 0, height() - i - 1, width());
  }
}

}  // namespace tdcurses
