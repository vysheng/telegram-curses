#include "Screen.hpp"
#include "Window.hpp"
#include "WindowLayout.hpp"
#include "td/utils/Slice-decl.h"
#include "unicode.h"

#include <memory>
#include <tickit.h>
#include <notcurses/notcurses.h>

#include "td/utils/Time.h"
#include "td/utils/Timer.h"

namespace windows {

class BaseWindow : public Window {
 public:
  BaseWindow() {
  }

  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    set_need_refresh();
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

Screen::Screen(std::unique_ptr<Callback> callback) : callback_(std::move(callback)) {
}

struct ImplTickit : public Screen::Impl {
  Tickit *tickit_root_{nullptr};
  TickitTerm *tickit_term_{nullptr};
  TickitWindow *tickit_root_window_{nullptr};

  bool stop() override {
    if (tickit_root_) {
      tickit_window_destroy(tickit_root_window_);
      tickit_unref(tickit_root_);
      tickit_root_ = nullptr;
      tickit_term_ = nullptr;
      tickit_root_window_ = nullptr;
      return true;
    } else {
      return false;
    }
  }

  void on_resize() override {
    tickit_term_refresh_size(tickit_term_);
  }

  td::int32 height() override {
    int lines, cols;
    tickit_term_get_size(tickit_term_, &lines, &cols);
    return lines;
  }

  td::int32 width() override {
    int lines, cols;
    tickit_term_get_size(tickit_term_, &lines, &cols);
    return cols;
  }

  void tick() override {
    tickit_tick(tickit_root_, TICKIT_RUN_NOHANG);
  }

  void refresh(bool force, std::shared_ptr<Window> base_window) override {
    TickitRenderBuffer *tickit_rb = tickit_renderbuffer_new(height(), width());
    CHECK(tickit_rb);

    td::int32 cursor_y = 0, cursor_x = 0;
    WindowOutputter::CursorShape cursor_shape = WindowOutputter::CursorShape::None;
    {
      auto rb = tickit_window_outputter(tickit_rb, height(), width());
      static_cast<BaseWindow &>(*base_window).render(*rb, force);
      cursor_y = rb->global_cursor_y();
      cursor_x = rb->global_cursor_x();
      cursor_shape = rb->cursor_shape();
    }

    if (cursor_shape != WindowOutputter::CursorShape::None) {
      auto shape = [](WindowOutputter::CursorShape cursor_shape) -> TickitCursorShape {
        switch (cursor_shape) {
          case WindowOutputter::CursorShape::Block:
            return TickitCursorShape::TICKIT_CURSORSHAPE_BLOCK;
          case WindowOutputter::CursorShape::Underscore:
            return TickitCursorShape::TICKIT_CURSORSHAPE_UNDER;
          case WindowOutputter::CursorShape::LeftBar:
            return TickitCursorShape::TICKIT_CURSORSHAPE_LEFT_BAR;
          default:
            return TickitCursorShape::TICKIT_CURSORSHAPE_LEFT_BAR;
        }
      }(cursor_shape);
      tickit_window_set_cursor_shape(tickit_root_window_, shape);
      tickit_window_set_cursor_visible(tickit_root_window_, true);
    } else {
      tickit_window_set_cursor_visible(tickit_root_window_, false);
    }
    tickit_window_set_cursor_position(tickit_root_window_, cursor_y, cursor_x);
    tickit_window_flush(tickit_root_window_);

    tickit_renderbuffer_flush_to_term(tickit_rb, tickit_term_);
    tickit_renderbuffer_destroy(tickit_rb);

    tickit_term_flush(tickit_term_);
  }

  td::int32 poll_fd() override {
    return 0;
  }
};

struct ImplNotcurses : public Screen::Impl {
  /*Tickit *tickit_root_{nullptr};
  TickitTerm *tickit_term_{nullptr};
  TickitWindow *tickit_root_window_{nullptr};*/
  struct notcurses *nc_{nullptr};
  struct ncplane *baseplane_{nullptr};
  struct ncplane *renderplane_{nullptr};
  Screen *screen_{nullptr};
  bool cursor_enabled_{true};

  bool stop() override {
    if (nc_) {
      notcurses_stop(nc_);
      nc_ = nullptr;
      return true;
    } else {
      return false;
    }
  }

  void on_resize() override {
    //tickit_term_refresh_size(tickit_term_);
  }

  td::int32 height() override {
    unsigned lines, cols;
    notcurses_stddim_yx(nc_, &lines, &cols);
    return lines;
  }

  td::int32 width() override {
    unsigned lines, cols;
    notcurses_stddim_yx(nc_, &lines, &cols);
    return cols;
  }

  void tick() override {
    while (true) {
      struct timespec ts = {.tv_sec = 0, .tv_nsec = 0};
      struct ncinput ni;
      auto res = notcurses_get(nc_, &ts, &ni);
      if (!res) {
        break;
      }

      if (ni.id == NCKEY_RESIZE) {
        screen_->on_resize(width(), height());
        continue;
      }

      if (ni.evtype != NCTYPE_PRESS && ni.evtype != NCTYPE_REPEAT) {
        continue;
      }

      char buf[64];
      size_t pos = 0;
      size_t p = 0;
      while (ni.eff_text[p]) {
        pos += utf8_code_to_str(ni.eff_text[p++], buf + pos);
      }

      td::MutableCSlice eff_text(buf, buf + pos);

      LOG(ERROR) << "has_ctrl=" << ncinput_ctrl_p(&ni) << " has_alt=" << ncinput_alt_p(&ni)
                 << " has_shift=" << ncinput_shift_p(&ni) << " letter=" << eff_text.data() << " code=" << ni.id;

      auto r =
          parse_notcurses_input_event(ni.id, eff_text, ncinput_alt_p(&ni), ncinput_ctrl_p(&ni), ncinput_shift_p(&ni));
      if (!r) {
        continue;
      }
      screen_->handle_input(*r);
    }
    screen_->refresh(true);
  }

  void refresh(bool force, std::shared_ptr<Window> base_window) override {
    td::int32 cursor_y = 0, cursor_x = 0;
    WindowOutputter::CursorShape cursor_shape = WindowOutputter::CursorShape::None;
    {
      auto rb = notcurses_window_outputter(nc_, baseplane_, renderplane_, height(), width());
      static_cast<BaseWindow &>(*base_window).render(*rb, force);
      cursor_y = rb->global_cursor_y();
      cursor_x = rb->global_cursor_x();
      cursor_shape = rb->cursor_shape();
    }

    notcurses_render(nc_);

    if (cursor_shape != WindowOutputter::CursorShape::None && cursor_y >= 0 && cursor_x >= 0) {
      notcurses_cursor_enable(nc_, cursor_y, cursor_x);
      cursor_enabled_ = true;
    } else {
      if (cursor_enabled_) {
        notcurses_cursor_disable(nc_);
        cursor_enabled_ = false;
      }
    }
  }

  td::int32 poll_fd() override {
    return notcurses_inputready_fd(nc_);
  }
};

void Screen::init_tickit() {
  set_tickit_wrap();

  impl_ = std::make_unique<ImplTickit>();
  auto impl = (ImplTickit *)(impl_.get());

  base_window_ = std::make_shared<BaseWindow>();

  impl->tickit_root_ = tickit_new_stdtty();
  CHECK(impl->tickit_root_);
  impl->tickit_term_ = tickit_get_term(impl->tickit_root_);
  CHECK(impl->tickit_term_);
  td::int32 value;
  CHECK(tickit_term_getctl_int(impl->tickit_term_, TickitTermCtl::TICKIT_TERMCTL_MOUSE, &value));
  CHECK(value == TickitTermMouseMode::TICKIT_TERM_MOUSEMODE_OFF);

  int lines, cols;
  tickit_term_get_size(impl->tickit_term_, &lines, &cols);

  auto handle_resize = [](TickitTerm *tt, TickitEventFlags flags, void *_info, void *data) {
    TickitResizeEventInfo *info = (TickitResizeEventInfo *)_info;
    static_cast<Screen *>(data)->on_resize(info->cols, info->lines);
    return 1;
  };
  tickit_term_bind_event(impl->tickit_term_, TICKIT_TERM_ON_RESIZE, (TickitBindFlags)0, handle_resize, this);

  auto handle_input = [](TickitTerm *tt, TickitEventFlags flags, void *_info, void *data) {
    TickitKeyEventInfo *info = (TickitKeyEventInfo *)_info;
    if (info->type == TICKIT_KEYEV_KEY || info->type == TICKIT_KEYEV_TEXT) {
      auto r = parse_tickit_input_event(td::CSlice(info->str), info->type == TICKIT_KEYEV_KEY);
      static_cast<Screen *>(data)->handle_input(*r);
    }
    return 1;
  };
  tickit_term_bind_event(impl->tickit_term_, TICKIT_TERM_ON_KEY, (TickitBindFlags)0, handle_input, this);

  impl->tickit_root_window_ = tickit_window_new_root(impl->tickit_term_);
  tickit_window_take_focus(impl->tickit_root_window_);

  tickit_term_observe_sigwinch(impl->tickit_term_, true);

  create_empty_window_outputter_libtickit();

  on_resize(cols, lines);
}

void Screen::init_notcurses() {
  impl_ = std::make_unique<ImplNotcurses>();
  auto impl = (ImplNotcurses *)(impl_.get());

  base_window_ = std::make_shared<BaseWindow>();

  struct notcurses_options curses_opts{.termtype = NULL,
                                       .loglevel = NCLOGLEVEL_WARNING,
                                       .margin_t = 0,
                                       .margin_r = 0,
                                       .margin_b = 0,
                                       .margin_l = 0,
                                       .flags = 0};

  impl->screen_ = this;
  impl->nc_ = notcurses_init(&curses_opts, NULL);
  impl->baseplane_ = notcurses_stdplane(impl->nc_);

  struct ncplane_options plane_opts{.y = 0,
                                    .x = 0,
                                    .rows = (unsigned int)impl->height(),
                                    .cols = (unsigned int)impl->width(),
                                    .userptr = nullptr,
                                    .name = nullptr,
                                    .resizecb = nullptr,
                                    .flags = NCPLANE_OPTION_FIXED,
                                    .margin_b = 0,
                                    .margin_r = 0};

  impl->renderplane_ = ncplane_create(impl->baseplane_, &plane_opts);
  create_empty_window_outputter_notcurses(impl->nc_, impl->baseplane_, impl->renderplane_);
  on_resize(impl->width(), impl->height());
}

void Screen::init() {
  //init_tickit();
  init_notcurses();
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
  if (force) {
    impl_->on_resize();
  }

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
