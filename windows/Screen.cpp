#include "Screen.hpp"
#include "Window.hpp"
#include "WindowLayout.hpp"
#include "td/actor/Timeout.h"
#include "td/utils/Slice-decl.h"
#include "unicode.h"
#include "BackendTickit.h"
#include "BackendNotcurses.h"

#include <memory>

#include "td/utils/Time.h"
#include "td/utils/Timer.h"

namespace windows {

class BaseWindow : public Window {
 public:
  BaseWindow() {
  }

  auto resize_all_popup_windows() {
    for (auto it = popup_windows_.rbegin(); it != popup_windows_.rend(); it++) {
      auto &window = it->second;
      if (window->min_width() > width() || window->min_height() > height()) {
        UNREACHABLE();
      }

      auto max_w = std::max(width() / 2, window->min_width());
      auto max_h = std::max(height() / 2, window->min_height());
      auto w = std::min(max_w, window->best_width());
      auto h = std::min(max_h, window->best_height());

      auto w_off = (width() - w) / 2;
      auto h_off = (height() - h) / 2;
      window->resize(h, w);
      window->move_yx(h_off, w_off);
    }
  }

  void on_resize(td::int32 old_height, td::int32 old_width, td::int32 new_height, td::int32 new_width) override {
    set_need_refresh();
    if (layout_) {
      layout_->resize(new_height, new_width);
    }
    resize_all_popup_windows();
  }

  void render(WindowOutputter &rb, bool force) override {
    resize_all_popup_windows();
  }

  bool has_popup_window(Window *window) const {
    for (auto it = popup_windows_.begin(); it != popup_windows_.end(); it++) {
      if (it->second.get() == window) {
        return true;
      }
    }
    return false;
  }

  void add_popup_window(std::shared_ptr<Window> window, td::int32 priority) {
    add_subwindow(window, 0, 0);

    auto it = popup_windows_.begin();
    while (it != popup_windows_.end() && it->first <= priority) {
      it++;
    }
    if (false && it != popup_windows_.end()) {
      window->backend_window()->move_below(it->second->backend_window());
    } else {
      window->backend_window()->move_to_top();
    }
    it = popup_windows_.emplace(it, priority, window);
    if (it == popup_windows_.begin()) {
      activate_subwindow(window);
    }
    set_need_refresh_force_rec();
  }

  void del_popup_window(Window *window) {
    bool found = false;
    for (auto it = popup_windows_.begin(); it != popup_windows_.end(); it++) {
      if (it->second.get() == window) {
        popup_windows_.erase(it);
        found = true;
        break;
      }
    }
    CHECK(found);
    if (active_subwindow().get() == window) {
      if (popup_windows_.size() > 0) {
        activate_subwindow(popup_windows_.front().second);
      } else if (layout_) {
        activate_subwindow(layout_);
      }
    }
    del_subwindow(window);
    set_need_refresh_force_rec();
  }

  void change_layout(std::shared_ptr<WindowLayout> window_layout) {
    set_need_refresh_force();
    if (layout_) {
      del_subwindow(layout_.get());
    }
    layout_ = std::move(window_layout);
    layout_->resize(height(), width());
    layout_->backend_window()->move_above(backend_window());
    add_subwindow(layout_, 0, 0);
  }

  void handle_input(const InputEvent &info) override {
    if (info == "C-r") {
      set_need_refresh_force_rec();
      return;
    }

    auto w = active_subwindow();
    if (w) {
      w->handle_input(info);
    }
  }

 private:
  std::shared_ptr<WindowLayout> layout_;
  std::list<std::pair<td::int32, std::shared_ptr<Window>>> popup_windows_;
};

Screen::Screen(std::unique_ptr<Callback> callback, BackendType backend_type)
    : callback_(std::move(callback)), backend_type_(backend_type) {
}

void Screen::init() {
  base_window_ = std::make_shared<BaseWindow>();
  if (backend_type_ == BackendType::Tickit) {
    init_tickit_backend(this);
  } else {
    init_notcurses_backend(this);
  }

  on_resize(height(), width());
  backend_->create_backend_window(base_window_);
}

void Screen::stop() {
  if (!finished_ && backend_ && backend_->stop()) {
    callback_->on_close();
    finished_ = true;
    backend_ = nullptr;
  }
}

void Screen::on_resize(int height, int width) {
  if (!backend_ || finished_) {
    return;
  }

  backend_->on_resize();
  base_window_->resize(height, width);
  refresh(true);
  refresh(true);
}

td::int32 Screen::width() {
  if (!backend_ || finished_) {
    return 0;
  }
  return backend_->width();
}

td::int32 Screen::height() {
  if (!backend_ || finished_) {
    return 0;
  }
  return backend_->height();
}

bool Screen::has_popup_window(Window *window) const {
  if (!backend_ || !base_window_ || finished_) {
    return false;
  }
  return static_cast<BaseWindow &>(*base_window_).has_popup_window(window);
}

void Screen::add_popup_window(std::shared_ptr<Window> window, td::int32 priority) {
  if (!backend_ || !base_window_ || finished_) {
    return;
  }
  backend_->create_backend_window(window);
  static_cast<BaseWindow &>(*base_window_).add_popup_window(window, priority);
}

void Screen::del_popup_window(Window *window) {
  if (!backend_ || !base_window_ || finished_) {
    return;
  }
  static_cast<BaseWindow &>(*base_window_).del_popup_window(window);
}

void Screen::change_layout(std::shared_ptr<WindowLayout> window_layout) {
  if (!backend_ || !base_window_ || finished_) {
    return;
  }
  backend_->create_backend_window(window_layout);
  static_cast<BaseWindow &>(*base_window_).change_layout(window_layout);
}

td::Timestamp Screen::loop() {
  if (!backend_ || finished_) {
    return td::Timestamp::never();
  }

  backend_->tick();
  refresh(false);
  if (!base_window_->need_refresh()) {
    return td::Timestamp::never();
  } else {
    return base_window_->need_refresh_at();
  }
}

void Screen::handle_input(const InputEvent &info) {
  if (!backend_ || finished_) {
    return;
  }

  if (info == "C-r") {
    refresh(true);
    return;
  }

  static_cast<BaseWindow &>(*base_window_).handle_input(info);

  refresh();
}

void Screen::refresh(bool force) {
  if (!backend_ || finished_) {
    return;
  }
  /*if (force) {
    backend_->on_resize();
  }*/

  backend_->refresh(force, base_window_);
}

td::int32 Screen::poll_fd() {
  if (backend_) {
    return backend_->poll_fd();
  } else {
    return -1;
  }
}

Screen::~Screen() {
  stop();
}

}  // namespace windows
