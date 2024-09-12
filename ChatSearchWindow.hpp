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
  enum class Mode { Local, Global, Both };
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_answer(std::shared_ptr<Chat> chat) = 0;
    virtual void on_abort() = 0;
  };
  ChatSearchWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, Mode mode)
      : TdcursesWindowBase(root, std::move(root_actor)), mode_(mode) {
    build_subwindows();
  }

  static size_t max_results() {
    return 5;
  }

  void install_callback(std::unique_ptr<Callback> callback) {
    callback_ = std::move(callback);
  }
  void build_subwindows();

  void handle_input(TickitKeyEventInfo *info) override;
  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override;

  void try_run_request();
  void got_chats(td::tl_object_ptr<td::td_api::chats> res, bool is_local);
  void failed_to_get_chats(td::Status error, bool is_local);

  td::int32 best_width() override {
    return 30;
  }
  td::int32 best_height() override {
    return (td::int32)((mode_ == Mode::Both) ? 1 + 2 * max_results() : 1 + max_results());
  }

  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    editor_window_->resize(new_width, 1);
  }

  auto found_chats() const {
    return std::min(max_results(), found_chats_local_.size()) + std::min(max_results(), found_chats_global_.size());
  }

  std::shared_ptr<Chat> get_found_chat(size_t idx) {
    if (idx < std::min(found_chats_local_.size(), max_results())) {
      return found_chats_local_[idx];
    } else {
      idx -= std::min(found_chats_local_.size(), max_results());
      return found_chats_global_[idx];
    }
  }

 private:
  std::shared_ptr<windows::OneLineInputWindow> editor_window_;
  std::vector<std::shared_ptr<Chat>> found_chats_local_;
  std::vector<std::shared_ptr<Chat>> found_chats_global_;
  size_t cur_selected_{0};
  std::string last_request_text_;
  td::int32 running_request_{0};
  std::unique_ptr<Callback> callback_;
  Mode mode_;
};

}  // namespace tdcurses
