#include "windows/window-layout.h"
#include "windows/window.h"
#include "telegram-curses-window-root.h"
#include <memory>

namespace tdcurses {

class Layout
    : public windows::WindowLayout
    , public TdcursesWindowBase {
 public:
  Layout(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : TdcursesWindowBase(root, std::move(root_actor)) {
    log_window_ = std::make_shared<windows::EmptyWindow>();
    dialog_list_window_ = std::make_shared<windows::EmptyWindow>();
    chat_window_ = std::make_shared<windows::EmptyWindow>();

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
    if (active_window().get() == dialog_list_window_.get()) {
      activate_window(log_window_);
    } else if (active_window().get() == chat_window_.get()) {
      if (compose_window_) {
        activate_window(compose_window_);
      } else {
        activate_window(log_window_);
      }
    } else if (active_window().get() == compose_window_.get()) {
      activate_window(log_window_);
    }
  }
  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    auto h = log_window_enabled_ ? (td::int32)(height() * dialog_list_window_height_) : height();
    auto w = (td::int32)(width() * dialog_list_window_width_);
    dialog_list_window_->resize(w, h);
    if (!compose_window_) {
      chat_window_->resize(width() - 1 - w, h);
    } else {
      auto h2 = (td::int32)(height() * chat_window_height_);
      chat_window_->resize(width() - 1 - w, h2);
      compose_window_->resize(width() - 1 - w, h - 1 - h2);
    }
    if (log_window_enabled_) {
      log_window_->resize(width(), height() - 1 - h);
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
      windows.emplace_back(std::make_unique<WindowInfo>(0, dialog_list_window_->height() + 1, log_window_));
    }
    set_subwindow_list(std::move(windows));
  }

  void render_borders(TickitRenderBuffer *rb) override {
    tickit_renderbuffer_hline_at(rb, dialog_list_window_->height(), 0, width() - 1, TickitLineStyle::TICKIT_LINE_SINGLE,
                                 TickitLineCaps::TICKIT_LINECAP_BOTH);
    tickit_renderbuffer_vline_at(rb, 0, dialog_list_window_->height(), dialog_list_window_->width(),
                                 TickitLineStyle::TICKIT_LINE_SINGLE, TickitLineCaps::TICKIT_LINECAP_START);
    if (compose_window_) {
      tickit_renderbuffer_hline_at(rb, chat_window_->height(), dialog_list_window_->width(), width() - 1,
                                   TickitLineStyle::TICKIT_LINE_SINGLE, TickitLineCaps::TICKIT_LINECAP_END);
    }
  }

  void replace_log_window(std::shared_ptr<windows::Window> window) {
    replace_window_in(log_window_, std::move(window));
  }
  void replace_dialog_list_window(std::shared_ptr<windows::Window> window) {
    replace_window_in(dialog_list_window_, std::move(window));
  }
  void replace_chat_window(std::shared_ptr<windows::Window> window) {
    replace_window_in(chat_window_, std::move(window));
  }
  void replace_compose_window(std::shared_ptr<windows::Window> window) {
    replace_window_in(compose_window_, std::move(window), false);
    on_resize(width(), height(), width(), height());
  }

  void handle_input(TickitKeyEventInfo *info) override {
    if (info->type == TICKIT_KEYEV_KEY) {
      if (!strcmp(info->str, "F9")) {
        root()->show_config_window();
        return;
      }
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

 private:
  bool log_window_enabled_{true};

  std::shared_ptr<windows::Window> log_window_;
  std::shared_ptr<windows::Window> dialog_list_window_;
  std::shared_ptr<windows::Window> chat_window_;
  std::shared_ptr<windows::Window> compose_window_;

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
