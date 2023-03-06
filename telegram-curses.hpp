#pragma once

#include "telegram-curses.h"
#include "windows/screen.h"
#include "windows/log-window.h"
#include "windows/window.h"
#include "status-line-window.h"
#include <vector>

namespace tdcurses {

class Layout;
class DialogListWindow;
class ChatWindow;
class ComposeWindow;
class ConfigWindow;

struct TdcursesParameters {
  bool log_window_enabled{false};
  td::int32 log_window_height{10};
  td::int32 dialog_list_window_width{10};
  td::int32 compose_window_height{10};
};

class Tdcurses : public TdcursesInterface {
 public:
  struct Option {
    Option(std::string name, std::vector<std::string> values, std::function<void(std::string)> on_change,
           td::uint32 cur_selected = 0)
        : name(std::move(name)), values(std::move(values)), on_change(on_change), cur_selected(cur_selected) {
    }
    Option(std::string name, std::vector<std::string> n_values, std::function<void(std::string)> on_change,
           std::string f, td::uint32 cur_selected = 0)
        : name(std::move(name)), values(std::move(n_values)), on_change(on_change), cur_selected(cur_selected) {
      for (td::uint32 i = 0; i < values.size(); i++) {
        if (values[i] == f) {
          cur_selected = i;
          return;
        }
      }
    }
    std::string name;
    std::vector<std::string> values;
    std::function<void(std::string)> on_change;
    td::uint32 cur_selected{0};

    void update() {
      if (!values.size()) {
        return;
      }
      cur_selected++;
      if (cur_selected >= values.size()) {
        cur_selected = 0;
      }
      on_change(values[cur_selected]);
    }
  };

  void initialize_options(TdcursesParameters &params);

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
  std::shared_ptr<windows::Window> config_window_;
  std::shared_ptr<StatusLineWindow> status_line_window_;
  std::shared_ptr<CommandLineWindow> command_line_window_;

  std::vector<Option> options_;

 public:
  Tdcurses() {
  }

  void start_curses(TdcursesParameters &params);

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

  void hide_config_window();
  void show_config_window();

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
  auto status_line_window() const {
    return status_line_window_;
  }
  auto command_line_window() const {
    return command_line_window_;
  }
  auto chat_window() const {
    return chat_window_;
  }

  void start_up() override {
  }
  void on_close() {
  }

  virtual void update_status_line() = 0;
};

}  // namespace tdcurses
