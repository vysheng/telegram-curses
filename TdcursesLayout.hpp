#include "windows/Output.hpp"
#include "windows/WindowLayout.hpp"
#include "windows/Window.hpp"
#include "windows/EmptyWindow.hpp"
#include "windows/BorderedWindow.hpp"
#include "TdcursesWindowBase.hpp"
#include "ConfigWindow.hpp"
#include "HelpWindow.hpp"
#include "Debug.hpp"
#include "settings-menu/MainSettingsWindow.hpp"
#include "ChatInfoWindow.hpp"
#include "managers/GlobalParameters.hpp"
#include <memory>

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

    add_subwindow(log_window_, 0, 0);
    add_subwindow(dialog_list_window_, 0, 0);
    add_subwindow(chat_window_, 0, 0);
    add_subwindow(command_line_, 0, 0);
    add_subwindow(status_line_, 0, 0);

    activate_subwindow(dialog_list_window_);
  }
  void activate_left_window() override {
    if (active_subwindow().get() != log_window_.get()) {
      activate_subwindow(dialog_list_window_);
    }
  }
  void activate_right_window() override {
    if (active_subwindow().get() == dialog_list_window_.get()) {
      activate_subwindow(chat_window_);
    }
  }
  void activate_upper_window() override {
    if (active_subwindow().get() == log_window_.get()) {
      activate_subwindow(dialog_list_window_);
    } else if (active_subwindow().get() == compose_window_.get()) {
      activate_subwindow(chat_window_);
    }
  }
  void activate_lower_window() override {
    if (active_subwindow().get() == chat_window_.get() && compose_window_) {
      activate_subwindow(compose_window_);
    } else if (log_window_enabled_) {
      activate_subwindow(log_window_);
    }
  }
  void on_resize(td::int32 old_height, td::int32 old_width, td::int32 new_height, td::int32 new_width) override {
    auto h = log_window_enabled_ ? (td::int32)(height() * dialog_list_window_height_) : height();
    auto w = (td::int32)(width() * dialog_list_window_width_);
    h -= 2;
    dialog_list_window_->resize(h, w);
    dialog_list_window_->move_yx(0, 0);
    if (!compose_window_) {
      chat_window_->resize(h, width() - 1 - w);
      chat_window_->move_yx(0, w + 1);
    } else {
      auto h2 = (td::int32)(height() * chat_window_height_);
      chat_window_->resize(h2, width() - 1 - w);
      chat_window_->move_yx(0, w + 1);
      compose_window_->resize(h - 1 - h2, width() - 1 - w);
      compose_window_->move_yx(h2 + 1, w + 1);
    }
    status_line_->resize(1, width());
    status_line_->move_yx(h, 0);
    command_line_->resize(1, width());
    command_line_->move_yx(h + 1, 0);
    if (log_window_enabled_) {
      log_window_->resize(height() - h - 2, width());
      log_window_->move_yx(h + 2, 0);
    } else {
      log_window_->resize(3, width());
      log_window_->move_yx(new_height, 0);
    }
  }

  void render_borders(windows::WindowOutputter &rb) override {
    const auto &borders = get_border_type_info(windows::BorderType::Simple, false);
    rb.fill_vert_yx(0, dialog_list_window_->width(), borders[0], dialog_list_window_->height());
    if (compose_window_) {
      rb.fill_yx(chat_window_->height(), dialog_list_window_->width(), borders[10],
                 width() - dialog_list_window_->width());
      rb.putstr_yx(chat_window_->height(), dialog_list_window_->width(), borders[4]);
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
    if (compose_window_ && active_subwindow().get() == compose_window_.get()) {
      activate_subwindow(chat_window_);
    }
    replace_window_in(compose_window_, std::move(window), false);
    on_resize(height(), width(), height(), width());
  }

  void handle_input(const windows::InputEvent &info) override {
    if (info == "T-F9") {
      root()->show_config_window();
      return;
    } else if (info == "T-F8") {
      auto user = chat_manager().get_user(global_parameters().my_user_id());
      if (user) {
        create_menu_window<ChatInfoWindow>(root(), root_actor_id(), user);
      }
      return;
    } else if (info == "T-F7") {
      create_menu_window<MainSettingsWindow>(root(), root_actor_id());
      return;
    } else if (info == "T-F12") {
      auto s = debug_counters().to_str();
      root()->spawn_popup_view_window(s, 1);
      return;
    } else if (info == "T-F1") {
      Outputter out;
      generate_help_info(out);
      root()->spawn_popup_view_window(out.as_str(), out.markup(), 1);
      return;
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
      on_resize(height(), width(), height(), width());
      set_need_refresh();
    }
  }

  void disable_log_window() {
    if (log_window_enabled_) {
      log_window_enabled_ = false;
      on_resize(height(), width(), height(), width());
      set_need_refresh();
    }
  }

  void initialize_sizes(bool log_window_enabled, td::int32 log_window_height, td::int32 dialog_list_window_width,
                        td::int32 compose_window_height) {
    log_window_enabled_ = log_window_enabled;
    dialog_list_window_height_ = 1.0 - 0.01 * log_window_height;
    dialog_list_window_width_ = 0.01 * dialog_list_window_width;
    chat_window_height_ = 1.0 - 0.01 * compose_window_height;

    on_resize(height(), width(), height(), width());
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
    if (old && old.get() == window.get()) {
      return;
    }
    if (!window && need_empty) {
      window = std::make_shared<windows::EmptyWindow>();
    }
    if (window) {
      if (old) {
        add_subwindow(window, old->y_offset(), old->x_offset());
        window->resize(old->height(), old->width());
      } else {
        add_subwindow(window, 0, 0);
        window->resize(1, 1);
      }
    }
    auto cur_active = active_subwindow();
    if (active_subwindow().get() == old.get()) {
      if (window) {
        activate_subwindow(window);
      } else {
        activate_subwindow(dialog_list_window_);
      }
    }
    del_subwindow(old.get());
    old = std::move(window);
    set_need_refresh();
  }
};

}  // namespace tdcurses
