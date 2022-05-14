#include "window-layout.h"
#include <memory>
#include <vector>

namespace windows {

class WindowLayoutSplit : public WindowLayout {
 public:
  WindowLayoutSplit() {
    for (size_t i = 0; i < 4; i++) {
      windows_.push_back(std::make_shared<EmptyWindow>());
    }
    update_windows_list();
    activate_window(windows_[0]);
  }
  void set_window(size_t idx, std::shared_ptr<Window> window) {
    if (!window) {
      window = std::make_shared<EmptyWindow>();
    }
    CHECK(idx >= 0 && idx <= 3);
    auto cur_active = active_window();
    CHECK(cur_active);
    bool change_active = windows_[idx].get() == cur_active.get();
    window->resize(windows_[idx]->width(), windows_[idx]->height());
    windows_[idx] = window;

    if (change_active) {
      activate_window(window);
    }

    update_windows_list();
  }

  void del_window(size_t idx) {
    set_window(idx, nullptr);
  }

  void update_windows_list() {
    std::list<std::unique_ptr<WindowInfo>> windows;
    for (size_t i = 0; i < 4; i++) {
      td::int32 x_offset = (i & 1) ? windows_[0]->width() + 1 : 0;
      td::int32 y_offset = (i > 1) ? windows_[0]->height() + 1 : 0;
      windows.emplace_back(std::make_unique<WindowInfo>(x_offset, y_offset, windows_[i]));
    }
    set_subwindow_list(std::move(windows));
  }

  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    auto w = new_width <= 2 ? 0 : (new_width / 2 - 1);
    auto h = new_height <= 2 ? 0 : (new_height / 2 - 1);
    for (size_t i = 0; i < 4; i++) {
      windows_[i]->resize((i & 1) ? new_width - 1 - w : w, (i > 1) ? new_height - 1 - h : h);
    }
    update_windows_list();
  }

  void render_borders(TickitRenderBuffer *rb) override {
    tickit_renderbuffer_hline_at(rb, windows_[0]->height(), 0, width() - 1, TickitLineStyle::TICKIT_LINE_SINGLE,
                                 TickitLineCaps::TICKIT_LINECAP_BOTH);
    tickit_renderbuffer_vline_at(rb, 0, height() - 1, windows_[0]->width(), TickitLineStyle::TICKIT_LINE_SINGLE,
                                 TickitLineCaps::TICKIT_LINECAP_BOTH);
  }

 private:
  std::vector<std::shared_ptr<Window>> windows_;
};

}  // namespace windows
