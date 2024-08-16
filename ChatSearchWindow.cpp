#include "ChatSearchWindow.hpp"
#include "td/telegram/StickerType.h"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Promise.h"
#include "td/utils/Random.h"
#include "td/utils/Status.h"
#include "windows/EditorWindow.hpp"
#include "windows/Markup.hpp"
#include "windows/TextEdit.hpp"
#include <memory>
#include <vector>

namespace tdcurses {

void ChatSearchWindow::build_subwindows() {
  editor_window_ = std::make_shared<windows::OneLineInputWindow>(">", false, nullptr);
  editor_window_->resize(width(), 1);
}

void ChatSearchWindow::handle_input(TickitKeyEventInfo *info) {
  if (info->type == TICKIT_KEYEV_KEY) {
    if (!strcmp(info->str, "Up")) {
      if (cur_selected_ > 0) {
        cur_selected_--;
      }
    } else if (!strcmp(info->str, "Down")) {
      if (cur_selected_ < found_chats_.size()) {
        cur_selected_++;
      }
    } else if (!strcmp(info->str, "Enter")) {
      if (cur_selected_ != 0) {
        callback_->on_answer(found_chats_[cur_selected_ - 1]);
      }
    } else if (!strcmp(info->str, "Tab")) {
      cur_selected_++;
      if (cur_selected_ > found_chats_.size()) {
        cur_selected_ = 0;
      }
    } else if (!strcmp(info->str, "M-Enter")) {
      if (cur_selected_ != 0) {
        callback_->on_answer(found_chats_[cur_selected_ - 1]);
      }
    } else if (!strcmp(info->str, "Escape")) {
      callback_->on_abort();
    } else {
      if (cur_selected_ == 0) {
        editor_window_->handle_input(info);
      }
    }
  } else {
    if (cur_selected_ == 0) {
      editor_window_->handle_input(info);
    }
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
  running_request_ = true;
  last_request_text_ = std::move(text);

  auto P = td::PromiseCreator::lambda([self = this](td::Result<td::tl_object_ptr<td::td_api::chats>> R) {
    if (R.is_ok() || R.error().code() != ErrorCodeWindowDeleted) {
      if (self->local_) {
        R.ensure();
      }
      if (R.is_ok()) {
        self->got_chats(R.move_as_ok());
      } else {
        self->failed_to_get_chats(R.move_as_error());
      }
    }
  });

  if (local_) {
    auto req = td::make_tl_object<td::td_api::searchChats>(last_request_text_, height() - 1);
    send_request(std::move(req), std::move(P));
  } else {
    auto req = td::make_tl_object<td::td_api::searchPublicChats>(last_request_text_);
    send_request(std::move(req), std::move(P));
  }
}

void ChatSearchWindow::got_chats(td::tl_object_ptr<td::td_api::chats> res) {
  CHECK(running_request_);
  running_request_ = false;

  found_chats_.clear();
  for (auto c : res->chat_ids_) {
    auto chat = chat_manager().get_chat(c);
    if (chat) {
      found_chats_.push_back(chat);
    }
  }
  if (cur_selected_ > found_chats_.size()) {
    cur_selected_ = found_chats_.size();
  }
  set_need_refresh();
  try_run_request();
}

void ChatSearchWindow::failed_to_get_chats(td::Status error) {
  CHECK(running_request_);
  running_request_ = false;

  found_chats_.clear();
  if (cur_selected_ > found_chats_.size()) {
    cur_selected_ = found_chats_.size();
  }
  set_need_refresh();
  try_run_request();
}

void ChatSearchWindow::render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y,
                              TickitCursorShape &cursor_shape, bool force) {
  td::int32 t_x, t_y;
  TickitCursorShape t_cur;

  editor_window_->render(rb, t_x, t_y, t_cur, force);

  if (cur_selected_ == 0) {
    cursor_x = t_x;
    cursor_y = t_y;
    cursor_shape = t_cur;
  }

  td::uint32 i = 0;
  for (i = 0; i < (td::uint32)height() - 1 && i < found_chats_.size(); i++) {
    bool is_selected = i + 1 == cur_selected_;
    auto rect = TickitRect{.top = (int)i + 1, .left = 0, .lines = 1, .cols = width()};
    tickit_renderbuffer_save(rb);
    tickit_renderbuffer_clip(rb, &rect);
    tickit_renderbuffer_translate(rb, i + 1, 0);
    windows::TextEdit::render(rb, t_x, t_y, t_cur, width(), found_chats_[i]->title(), 0,
                              std::vector<windows::MarkupElement>(), is_selected, false);
    tickit_renderbuffer_restore(rb);
  }

  if (i + 1 < (td::uint32)height()) {
    auto rect = TickitRect{.top = (td::int32)i + 1, .left = 0, .lines = height() - (td::int32)i - 1, .cols = width()};
    tickit_renderbuffer_eraserect(rb, &rect);
  }
}

}  // namespace tdcurses
