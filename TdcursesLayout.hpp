#include "windows/WindowLayout.hpp"
#include "windows/Window.hpp"
#include "TdcursesWindowBase.hpp"
#include "ConfigWindow.hpp"
#include "Debug.hpp"
#include "SettingsWindow.hpp"
#include "ChatInfoWindow.hpp"
#include "GlobalParameters.hpp"
#include <memory>
#include <tickit.h>

namespace tdcurses {

class TdcursesLayout
    : public windows::WindowLayout
    , public TdcursesWindowBase {
 public:
  TdcursesLayout(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : TdcursesWindowBase(root, std::move(root_actor)) {
    log_window_ = std::make_shared<windows::EmptyWindow>();
    dialog_list_window_ = std::make_shared<windows::EmptyWindow>();
    chat_window_ = std::make_shared<windows::EmptyWindow>();
    command_line_ = std::make_shared<windows::EmptyWindow>();
    status_line_ = std::make_shared<windows::EmptyWindow>();

    update_windows_list();
    activate_window(dialog_list_window_);
  }
  void activate_left_window() override {
    if (active_window().get() != log_window_.get()) {
      activate_window(dialog_list_window_);
    }
  }
  void activate_right_window() override {
    if (active_window().get() == dialog_list_window_.get()) {
      activate_window(chat_window_);
    }
  }
  void activate_upper_window() override {
    if (active_window().get() == log_window_.get()) {
      activate_window(dialog_list_window_);
    } else if (active_window().get() == compose_window_.get()) {
      activate_window(chat_window_);
    }
  }
  void activate_lower_window() override {
    if (active_window().get() == chat_window_.get() && compose_window_) {
      activate_window(compose_window_);
    } else if (log_window_enabled_) {
      activate_window(log_window_);
    }
  }
  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    auto h = log_window_enabled_ ? (td::int32)(height() * dialog_list_window_height_) : height();
    auto w = (td::int32)(width() * dialog_list_window_width_);
    h -= 2;
    dialog_list_window_->resize(w, h);
    if (!compose_window_) {
      chat_window_->resize(width() - 1 - w, h);
    } else {
      auto h2 = (td::int32)(height() * chat_window_height_);
      chat_window_->resize(width() - 1 - w, h2);
      compose_window_->resize(width() - 1 - w, h - 1 - h2);
    }
    status_line_->resize(width(), 1);
    command_line_->resize(width(), 1);
    if (log_window_enabled_) {
      log_window_->resize(width(), height() - 1 - h - 2);
    }
    update_windows_list();
  }

  void update_windows_list() {
    std::list<std::unique_ptr<WindowInfo>> windows;

    windows.emplace_back(std::make_unique<WindowInfo>(0, 0, dialog_list_window_));
    windows.emplace_back(std::make_unique<WindowInfo>(dialog_list_window_->width() + 1, 0, chat_window_));
    if (compose_window_) {
      windows.emplace_back(
          std::make_unique<WindowInfo>(dialog_list_window_->width() + 1, chat_window_->height() + 1, compose_window_));
    }
    if (log_window_enabled_) {
      windows.emplace_back(std::make_unique<WindowInfo>(0, dialog_list_window_->height() + 3, log_window_));
    }
    windows.emplace_back(std::make_unique<WindowInfo>(0, dialog_list_window_->height(), status_line_));
    windows.emplace_back(std::make_unique<WindowInfo>(0, dialog_list_window_->height() + 1, command_line_));
    set_subwindow_list(std::move(windows));
  }

  void render_borders(TickitRenderBuffer *rb) override {
    /*if (log_window_enabled_) {
      tickit_renderbuffer_hline_at(rb, dialog_list_window_->height() + 2, 0, width() - 1,
                                   TickitLineStyle::TICKIT_LINE_SINGLE, TickitLineCaps::TICKIT_LINECAP_BOTH);
    }*/
    tickit_renderbuffer_vline_at(rb, 0, dialog_list_window_->height() - 1, dialog_list_window_->width(),
                                 TickitLineStyle::TICKIT_LINE_SINGLE, TickitLineCaps::TICKIT_LINECAP_BOTH);
    if (compose_window_) {
      tickit_renderbuffer_hline_at(rb, chat_window_->height(), dialog_list_window_->width(), width() - 1,
                                   TickitLineStyle::TICKIT_LINE_SINGLE, TickitLineCaps::TICKIT_LINECAP_END);
    }
  }

  void replace_log_window(std::shared_ptr<windows::Window> window) {
    replace_window_in(log_window_, std::move(window));
  }
  void replace_status_line_window(std::shared_ptr<windows::Window> window) {
    replace_window_in(status_line_, std::move(window));
  }
  void replace_command_line_window(std::shared_ptr<windows::Window> window) {
    replace_window_in(command_line_, std::move(window));
  }
  void replace_dialog_list_window(std::shared_ptr<windows::Window> window) {
    replace_window_in(dialog_list_window_, std::move(window));
  }
  void replace_chat_window(std::shared_ptr<windows::Window> window) {
    replace_window_in(chat_window_, std::move(window));
  }
  void replace_compose_window(std::shared_ptr<windows::Window> window) {
    if (compose_window_ && active_window().get() == compose_window_.get()) {
      activate_window(chat_window_);
    }
    replace_window_in(compose_window_, std::move(window), false);
    on_resize(width(), height(), width(), height());
  }

  void handle_input(TickitKeyEventInfo *info) override {
    if (info->type == TICKIT_KEYEV_KEY) {
      if (!strcmp(info->str, "F9")) {
        root()->show_config_window();
        return;
      }
      if (!strcmp(info->str, "F8")) {
        auto user = chat_manager().get_user(global_parameters().my_user_id());
        if (user) {
          create_menu_window<ChatInfoWindow>(root(), root_actor_id(), user);
        }
        return;
      }
      if (!strcmp(info->str, "F7")) {
        create_menu_window<MainSettingsWindow>(root(), root_actor_id());
        return;
      }
      if (!strcmp(info->str, "F1")) {
        auto s = debug_counters().to_str();
        root()->spawn_popup_view_window(s, 1);
        return;
      }
    }

    if (command_line_->force_active()) {
      command_line_->handle_input(info);
      return;
    }

    return windows::WindowLayout::handle_input(info);
  }

  void enable_log_window() {
    if (!log_window_enabled_) {
      log_window_enabled_ = true;
      on_resize(width(), height(), width(), height());
      set_need_refresh();
    }
  }

  void disable_log_window() {
    if (log_window_enabled_) {
      log_window_enabled_ = false;
      on_resize(width(), height(), width(), height());
      set_need_refresh();
    }
  }

  void initialize_sizes(TdcursesParameters &params) {
    log_window_enabled_ = params.log_window_enabled;
    dialog_list_window_height_ = 1.0 - 0.01 * params.log_window_height;
    dialog_list_window_width_ = 0.01 * params.dialog_list_window_width;
    chat_window_height_ = 1.0 - 0.01 * params.compose_window_height;

    on_resize(width(), height(), width(), height());
    set_need_refresh();
  }

 private:
  bool log_window_enabled_{true};

  std::shared_ptr<windows::Window> log_window_;
  std::shared_ptr<windows::Window> dialog_list_window_;
  std::shared_ptr<windows::Window> chat_window_;
  std::shared_ptr<windows::Window> compose_window_;
  std::shared_ptr<windows::Window> status_line_;
  std::shared_ptr<windows::Window> command_line_;

  double dialog_list_window_height_{0.9};
  double dialog_list_window_width_{0.1};
  double chat_window_height_{0.8};

  void replace_window_in(std::shared_ptr<windows::Window> &old, std::shared_ptr<windows::Window> window,
                         bool need_empty = true) {
    auto cur_active = active_window();
    if (!window && need_empty) {
      window = std::make_shared<windows::EmptyWindow>();
    }
    if (window) {
      if (old) {
        window->resize(old->width(), old->height());
      }
    }
    if (cur_active.get() == old.get()) {
      if (window) {
        activate_window(window);
      } else {
        activate_window(dialog_list_window_);
      }
    }
    old = std::move(window);
    update_windows_list();
    set_need_refresh();
  }
};

}  // namespace tdcurses
