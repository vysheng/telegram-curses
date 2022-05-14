#pragma once

#include "telegram-curses.h"
#include "windows/screen.h"
#include "windows/log-window.h"
#include "windows/window.h"

namespace tdcurses {

class Layout;
class DialogListWindow;
class ChatWindow;
class ComposeWindow;

class Tdcurses : public TdcursesInterface {
 private:
  td::ActorId<Tdcurses> self_;
  std::unique_ptr<windows::Screen> screen_;
  std::shared_ptr<Layout> layout_;
  std::shared_ptr<windows::LogWindow> log_window_;
  std::unique_ptr<td::LogInterface> log_interface_;
  std::shared_ptr<windows::Window> qr_code_window_;
  std::shared_ptr<DialogListWindow> dialog_list_window_;
  std::shared_ptr<ChatWindow> chat_window_;
  std::shared_ptr<ComposeWindow> compose_window_;

 public:
  Tdcurses() {
  }

  void start_curses();

  bool window_exists(td::int64 id);

  void add_qr_code_window(std::shared_ptr<windows::Window> window) {
    if (qr_code_window_) {
      del_popup_window(qr_code_window_.get());
    }
    qr_code_window_ = window;
    add_popup_window(window, 0);
  }

  void del_qr_code_window() {
    if (qr_code_window_) {
      del_popup_window(qr_code_window_.get());
      qr_code_window_ = nullptr;
    }
  }

  void add_popup_window(std::shared_ptr<windows::Window> window, td::int32 prio) {
    screen_->add_popup_window(std::move(window), prio);
  }
  void del_popup_window(windows::Window *window) {
    screen_->del_popup_window(window);
  }
  void open_chat(td::int64 chat_id);
  void open_compose_window();
  void close_compose_window();

  void loop() override;
  void refresh();
  void tear_down() override;

  auto width() const {
    return screen_->width();
  }
  auto height() const {
    return screen_->height();
  }
  auto dialog_list_window() const {
    return dialog_list_window_;
  }
  auto chat_window() const {
    return chat_window_;
  }

  void start_up() override {
  }
  void on_close() {
  }
};

}  // namespace tdcurses
