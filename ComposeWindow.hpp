#pragma once

#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/EditorWindow.hpp"
#include "TdcursesWindowBase.hpp"
#include "windows/Window.hpp"
#include <memory>

namespace tdcurses {

class ComposeWindow
    : public windows::Window
    , public TdcursesWindowBase {
 public:
  ComposeWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id, std::string text,
                ChatWindow *chat_window)
      : TdcursesWindowBase(root, std::move(root_actor)), chat_id_(chat_id), chat_window_(chat_window) {
    editor_window_ = std::make_shared<windows::EditorWindow>(std::move(text), nullptr);
    install_callback();
  }
  ComposeWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id, std::string text,
                td::int64 reply_message_id, ChatWindow *chat_window)
      : TdcursesWindowBase(root, std::move(root_actor))
      , chat_id_(chat_id)
      , reply_message_id_(reply_message_id)
      , chat_window_(chat_window) {
    editor_window_ = std::make_shared<windows::EditorWindow>(std::move(text), nullptr);
    install_callback();
  }

  void install_callback();
  void send_message(std::string message);
  void message_sent(td::Result<td::tl_object_ptr<td::td_api::message>> result);
  void set_draft(std::string message);

  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override;
  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    editor_window_->resize(width(), reply_message_id_ ? height() - 1 : height());
  }
  bool need_refresh() override {
    return Window::need_refresh() || (editor_window_ && editor_window_->need_refresh());
  }
  void handle_input(TickitKeyEventInfo *info) override {
    editor_window_->handle_input(info);
  }
  void set_reply_message_id(td::int64 reply_message_id) {
    reply_message_id_ = reply_message_id;
    editor_window_->resize(width(), reply_message_id_ ? height() - 1 : height());
    set_need_refresh();
  }

 private:
  td::int64 chat_id_;
  td::int64 reply_message_id_{0};
  ChatWindow *chat_window_{nullptr};
  std::shared_ptr<windows::EditorWindow> editor_window_;
};

}  // namespace tdcurses
