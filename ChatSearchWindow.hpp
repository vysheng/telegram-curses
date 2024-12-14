#pragma once

#include "td/tl/TlObject.h"
#include "TdcursesWindowBase.hpp"
#include "ChatManager.hpp"
#include "MenuWindow.hpp"
#include "windows/OneLineInputWindow.hpp"
#include "td/utils/Promise.h"
#include "windows/Output.hpp"
#include <memory>
#include <vector>

namespace tdcurses {

class ChatSearchWindow
    : public windows::Window
    , public MenuWindow {
 public:
  enum class Mode { Local, Global, Both };
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_answer(ChatSearchWindow &, std::shared_ptr<Chat> chat) = 0;
    virtual void on_abort(ChatSearchWindow &) = 0;
  };
  ChatSearchWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, Mode mode, std::unique_ptr<Callback> callback)
      : MenuWindow(root, std::move(root_actor)), mode_(mode), callback_(std::move(callback)) {
    build_subwindows();
  }

  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> value) override {
    return std::static_pointer_cast<ChatSearchWindow>(std::move(value));
  }

  static size_t max_results() {
    return 5;
  }

  void install_callback(std::unique_ptr<Callback> callback) {
    callback_ = std::move(callback);
  }
  void build_subwindows();

  void handle_input(const windows::InputEvent &info) override;
  void render(windows::WindowOutputter &rb, bool force) override;

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
  Mode mode_;
  std::unique_ptr<Callback> callback_;
};

template <typename T, typename F>
std::enable_if_t<std::is_base_of<MenuWindow, T>::value, std::shared_ptr<ChatSearchWindow>> spawn_chat_search_window(
    T &cur_window, ChatSearchWindow::Mode mode, F &&cb) {
  class Cb : public ChatSearchWindow::Callback {
   public:
    Cb(F &&cb, T *win) : cb_(std::move(cb)), self_(win), self_id_(win->window_unique_id()) {
    }
    void on_abort(ChatSearchWindow &w) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
        cb_(td::Status::Error("aborted"));
      }
    }
    void on_answer(ChatSearchWindow &w, std::shared_ptr<Chat> answer) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
        cb_(std::move(answer));
      }
    }

   private:
    F cb_;
    T *self_;
    td::int64 self_id_;
  };
  auto callback = std::make_unique<Cb>(std::move(cb), &cur_window);
  return cur_window.template spawn_submenu<ChatSearchWindow>(mode, std::move(callback));
}

template <typename T, typename F>
std::enable_if_t<!std::is_base_of<MenuWindow, T>::value && std::is_base_of<TdcursesWindowBase, T>::value,
                 std::shared_ptr<ChatSearchWindow>>
spawn_chat_search_window(T &cur_window, ChatSearchWindow::Mode mode, F &&cb) {
  class Cb : public ChatSearchWindow::Callback {
   public:
    Cb(F &&cb, T *win) : cb_(std::move(cb)), self_(win), self_id_(win->window_unique_id()) {
    }
    void on_abort(ChatSearchWindow &w) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
        cb_(td::Status::Error("aborted"));
      }
    }
    void on_answer(ChatSearchWindow &w, std::shared_ptr<Chat> answer) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
        cb_(std::move(answer));
      }
    }

   private:
    F cb_;
    T *self_;
    td::int64 self_id_;
  };
  auto callback = std::make_unique<Cb>(std::move(cb), &cur_window);
  return create_menu_window<ChatSearchWindow>(cur_window.root(), cur_window.root_actor_id(), mode, std::move(callback));
}

template <typename F>
std::shared_ptr<ChatSearchWindow> spawn_chat_search_window(Tdcurses &curses, td::ActorId<Tdcurses> curses_id,
                                                           ChatSearchWindow::Mode mode, F &&cb) {
  class Cb : public ChatSearchWindow::Callback {
   public:
    Cb(F &&cb) : cb_(std::move(cb)) {
    }
    void on_abort(ChatSearchWindow &w) override {
      w.rollback();
      cb_(td::Status::Error("aborted"));
    }
    void on_answer(ChatSearchWindow &w, std::shared_ptr<Chat> answer) override {
      w.rollback();
      cb_(std::move(answer));
    }

   private:
    F cb_;
  };
  auto callback = std::make_unique<Cb>(std::move(cb));
  return create_menu_window<ChatSearchWindow>(&curses, curses_id, mode, std::move(callback));
}

}  // namespace tdcurses
