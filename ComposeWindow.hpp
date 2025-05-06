#pragma once

#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "td/utils/Time.h"
#include "windows/EditorWindow.hpp"
#include "TdcursesWindowBase.hpp"
#include "windows/Output.hpp"
#include "windows/Window.hpp"
#include "GlobalParameters.hpp"
#include <memory>

namespace tdcurses {

class ComposeWindow
    : public windows::Window
    , public TdcursesWindowBase {
 public:
  struct EditMode {};
  struct ComposeMode {};
  enum class Mode { Compose, Edit };
  ComposeWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, Mode mode, td::int64 chat_id, td::int64 thread_id,
                std::string text, td::int64 reply_message_id, td::int64 edit_message_id, ChatWindow *chat_window)
      : TdcursesWindowBase(root, std::move(root_actor))
      , chat_id_(chat_id)
      , thread_id_(thread_id)
      , reply_message_id_(reply_message_id)
      , edit_message_id_(edit_message_id)
      , chat_window_(chat_window) {
    enabled_markdown_ = global_parameters().use_markdown();
    editor_window_ = std::make_shared<windows::EditorWindow>(std::move(text), nullptr);
    add_subwindow(editor_window_, 1, 0);
    install_callback();
  }
  ComposeWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, ComposeMode, td::int64 chat_id, td::int64 thread_id,
                std::string text, ChatWindow *chat_window)
      : ComposeWindow(root, std::move(root_actor), Mode::Compose, chat_id, thread_id, std::move(text), 0, 0,
                      chat_window) {
  }
  ComposeWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, ComposeMode, td::int64 chat_id, td::int64 thread_id,
                std::string text, td::int64 reply_message_id, ChatWindow *chat_window)
      : ComposeWindow(root, std::move(root_actor), Mode::Compose, chat_id, thread_id, std::move(text), reply_message_id,
                      0, chat_window) {
  }
  ComposeWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, EditMode, td::int64 chat_id, td::int64 message_id,
                std::string text, ChatWindow *chat_window)
      : ComposeWindow(root, std::move(root_actor), Mode::Edit, chat_id, 0, std::move(text), 0, message_id,
                      chat_window) {
  }

  void install_callback();
  void send_message(std::string message);
  void message_sent(td::Result<td::tl_object_ptr<td::td_api::message>> result);
  void set_draft(std::string message);

  void render(windows::WindowOutputter &rb, bool force) override;
  void on_resize(td::int32 old_height, td::int32 old_width, td::int32 new_height, td::int32 new_width) override {
    editor_window_->resize(reply_message_id_ ? new_height - 2 : new_height - 1, new_width);
    editor_window_->move_yx(reply_message_id_ ? 2 : 1, 0);
  }
  bool need_refresh() override {
    return Window::need_refresh() || (editor_window_ && editor_window_->need_refresh());
  }
  td::Timestamp need_refresh_at() override {
    auto t = Window::need_refresh_at();
    if (editor_window_) {
      t.relax(editor_window_->need_refresh_at());
    }
    return t;
  }
  void handle_input(const windows::InputEvent &info) override;
  void set_reply_message_id(td::int64 reply_message_id) {
    reply_message_id_ = reply_message_id;
    editor_window_->resize(reply_message_id_ ? height() - 2 : height() - 1, width());
    editor_window_->move_yx(reply_message_id_ ? 2 : 1, 0);
    set_need_refresh();
  }

  td::int32 min_height() override {
    return 4;
  }

 private:
  td::int64 chat_id_;
  td::int64 thread_id_;
  td::int64 reply_message_id_{0};
  td::int64 edit_message_id_{0};
  ChatWindow *chat_window_{nullptr};
  std::shared_ptr<windows::EditorWindow> editor_window_;
  bool enabled_markdown_;
  bool no_sound_{false};
};

}  // namespace tdcurses
