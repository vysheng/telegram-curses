#pragma once

#include "td/utils/Time.h"
#include "td/utils/int_types.h"
#include "td/utils/Status.h"
#include <memory>
#include <utility>
#include <vector>
#include <list>
#include <map>
#include "Input.hpp"
#include "Output.hpp"

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

  class Impl {
   public:
    virtual ~Impl() = default;
    virtual bool stop() = 0;
    virtual void on_resize() = 0;
    virtual td::int32 width() = 0;
    virtual td::int32 height() = 0;
    virtual void tick() = 0;
    virtual void refresh(bool force, std::shared_ptr<Window> base_window) = 0;
  };

  Screen(std::unique_ptr<Callback> callback);
  ~Screen();
  void init();
  void init_tickit();
  void stop();
  void handle_input(const InputEvent &info);
  void loop();
  void on_resize(int width, int height);
  void refresh(bool force = false);
  void render(Window &root, WindowOutputter &rb, bool force);

  td::int32 width();
  td::int32 height();

  bool has_popup_window(Window *ptr) const;
  void add_popup_window(std::shared_ptr<Window> window, td::int32 priority);
  void del_popup_window(Window *window);
  void change_layout(std::shared_ptr<WindowLayout> window_layout);

  td::Timestamp need_refresh_at();

 private:
  void activate_window_in();

  std::unique_ptr<Impl> impl_;

  std::shared_ptr<Window> base_window_;
  std::unique_ptr<Callback> callback_;
  bool finished_{false};
};

}  // namespace windows
