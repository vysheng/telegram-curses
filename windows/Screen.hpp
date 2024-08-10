#pragma once

#include "td/utils/Time.h"
#include "td/utils/int_types.h"
#include "td/utils/Status.h"
#include <memory>
#include <utility>
#include <vector>
#include <list>
#include <map>

#include <tickit.h>

namespace windows {

class Window;
class WindowLayout;

class Screen {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_close() = 0;
    virtual void on_resize(td::int32 width, td::int32 height) {
    }
  };

  Screen(std::unique_ptr<Callback> callback);
  ~Screen();
  void init();
  void stop();
  void handle_input(TickitKeyEventInfo *info);
  void loop();
  void resize(int width, int height);
  void refresh(bool force = false);

  td::int32 width();
  td::int32 height();

  bool has_popup_window(Window *ptr) const;
  void add_popup_window(std::shared_ptr<Window> window, td::int32 priority);
  void del_popup_window(Window *window);
  void change_layout(std::shared_ptr<WindowLayout> window_layout);

  td::Timestamp need_refresh_at();

 private:
  void activate_window_in();

  Tickit *tickit_root_{nullptr};
  TickitTerm *tickit_term_{nullptr};
  TickitWindow *tickit_root_window_{nullptr};

  std::shared_ptr<WindowLayout> layout_;
  std::list<std::pair<td::int32, std::shared_ptr<Window>>> popup_windows_;

  td::int32 cursor_x_{0}, cursor_y_{0};
  TickitCursorShape cursor_shape_ = TickitCursorShape::TICKIT_CURSORSHAPE_BLOCK;

  std::shared_ptr<Window> active_window_;
  std::unique_ptr<Callback> callback_;
  bool finished_{false};
};

}  // namespace windows
