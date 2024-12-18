#include "Screen.hpp"
#include "Window.hpp"
#include "WindowLayout.hpp"
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

  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    set_need_refresh();
    if (layout_) {
      layout_->resize(new_width, new_height);
    }
  }

  void render(WindowOutputter &rb, bool force) override {
    if (layout_) {
      layout_->resize(width(), height());
      if (force || (layout_->need_refresh() && layout_->need_refresh_at().is_in_past())) {
        render_subwindow(rb, layout_.get(), force, popup_windows_.size() == 0, true);
      }
    } else if (force) {
      rb.erase_rect(0, 0, height(), width());
    }

    size_t x = popup_windows_.size();
    for (auto it = popup_windows_.rbegin(); it != popup_windows_.rend(); it++) {
      bool is_last = --x == 0;
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
      window->resize(w, h);
      window->set_parent_offset(h_off, w_off);

      if (force || (window->need_refresh() && window->need_refresh_at().is_in_past())) {
        render_subwindow(rb, window.get(), force, is_last, true);
      }
    }
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
    layout_->set_parent(this);

    auto it = popup_windows_.begin();
    while (it != popup_windows_.end() && it->first <= priority) {
      it++;
    }
    it = popup_windows_.emplace(it, priority, window);
    if (it == popup_windows_.begin()) {
      if (active_window_) {
        active_window_->set_active(false);
      }
      active_window_ = window;
      active_window_->set_active(true);
    }
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
    if (active_window_.get() == window) {
      active_window_->set_active(false);
      active_window_ = nullptr;
      if (popup_windows_.size() > 0) {
        active_window_ = popup_windows_.begin()->second;
        active_window_->set_active(true);
      }
    }
  }

  void change_layout(std::shared_ptr<WindowLayout> window_layout) {
    layout_ = std::move(window_layout);
    layout_->resize(width(), height());
    layout_->set_parent(this);
  }

  void handle_input(const InputEvent &info) override {
    if (info == "C-r") {
      set_need_refresh();
      return;
    }

    if (active_window_) {
      auto saved_window = active_window_;
      active_window_->handle_input(info);
    } else if (layout_) {
      auto saved_layout = layout_;
      layout_->handle_input(info);
    }
  }

  td::Timestamp need_refresh_at() override {
    td::Timestamp r = Window::need_refresh_at();
    for (auto &x : popup_windows_) {
      if (x.second->need_refresh()) {
        r.relax(x.second->need_refresh_at());
      }
    }
    if (layout_ && layout_->need_refresh()) {
      r.relax(layout_->need_refresh_at());
    }
    return r;
  }

  bool need_refresh() override {
    if (Window::need_refresh()) {
      return true;
    }
    for (auto &x : popup_windows_) {
      if (x.second->need_refresh()) {
        return true;
      }
    }
    if (layout_ && layout_->need_refresh()) {
      return true;
    }
    return false;
  }

 private:
  std::shared_ptr<WindowLayout> layout_;
  std::list<std::pair<td::int32, std::shared_ptr<Window>>> popup_windows_;

  std::shared_ptr<Window> active_window_;
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

  on_resize(width(), height());
}

void Screen::stop() {
  if (!finished_ && impl_ && impl_->stop()) {
    callback_->on_close();
    finished_ = true;
    impl_ = nullptr;
  }
}

void Screen::on_resize(int width, int height) {
  if (!impl_ || finished_) {
    return;
  }

  impl_->on_resize();
  base_window_->resize(width, height);
  refresh(true);
  refresh(true);
}

td::int32 Screen::width() {
  if (!impl_ || finished_) {
    return 0;
  }
  return impl_->width();
}

td::int32 Screen::height() {
  if (!impl_ || finished_) {
    return 0;
  }
  return impl_->height();
}

bool Screen::has_popup_window(Window *window) const {
  if (!impl_ || !base_window_ || finished_) {
    return false;
  }
  return static_cast<BaseWindow &>(*base_window_).has_popup_window(window);
}

void Screen::add_popup_window(std::shared_ptr<Window> window, td::int32 priority) {
  if (!impl_ || !base_window_ || finished_) {
    return;
  }
  static_cast<BaseWindow &>(*base_window_).add_popup_window(window, priority);
  refresh(true);
}

void Screen::del_popup_window(Window *window) {
  if (!impl_ || !base_window_ || finished_) {
    return;
  }
  static_cast<BaseWindow &>(*base_window_).del_popup_window(window);
  refresh(true);
}

void Screen::change_layout(std::shared_ptr<WindowLayout> window_layout) {
  if (!impl_ || !base_window_ || finished_) {
    return;
  }
  static_cast<BaseWindow &>(*base_window_).change_layout(window_layout);
  refresh(true);
}

void Screen::loop() {
  if (!impl_ || finished_) {
    return;
  }

  impl_->tick();
  refresh(true);
}

void Screen::handle_input(const InputEvent &info) {
  if (!impl_ || finished_) {
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
  if (!impl_ || finished_) {
    return;
  }
  /*if (force) {
    impl_->on_resize();
  }*/

  impl_->refresh(force, base_window_);
}

td::Timestamp Screen::need_refresh_at() {
  td::Timestamp r = static_cast<BaseWindow &>(*base_window_).need_refresh_at();
  return r;
}

td::int32 Screen::poll_fd() {
  if (impl_) {
    return impl_->poll_fd();
  } else {
    return -1;
  }
}

Screen::~Screen() {
  stop();
}

}  // namespace windows
