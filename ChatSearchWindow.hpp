#pragma once

#include "td/tl/TlObject.h"
#include "TdcursesWindowBase.hpp"
#include "ChatManager.hpp"
#include "windows/EditorWindow.hpp"
#include "td/utils/Promise.h"
#include <memory>
#include <vector>

namespace tdcurses {

class ChatSearchWindow
    : public windows::Window
    , public TdcursesWindowBase {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_answer(std::shared_ptr<Chat> chat) = 0;
    virtual void on_abort() = 0;
  };
  ChatSearchWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, bool local)
      : TdcursesWindowBase(root, std::move(root_actor)), local_(local) {
    build_subwindows();
  }

  void install_callback(std::unique_ptr<Callback> callback) {
    callback_ = std::move(callback);
  }
  void build_subwindows();

  void handle_input(TickitKeyEventInfo *info) override;
  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override;

  void try_run_request();
  void got_chats(td::tl_object_ptr<td::td_api::chats> res);
  void failed_to_get_chats(td::Status error);

  td::int32 best_width() override {
    return 30;
  }
  td::int32 best_height() override {
    return 6;
  }

  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    editor_window_->resize(new_width, 1);
  }

 private:
  std::shared_ptr<windows::OneLineInputWindow> editor_window_;
  std::vector<std::shared_ptr<Chat>> found_chats_;
  size_t cur_selected_{0};
  std::string last_request_text_;
  bool running_request_{false};
  std::unique_ptr<Callback> callback_;
  bool local_{false};
};

}  // namespace tdcurses
