#pragma once

#include "td/actor/Timeout.h"
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

class Backend {
 public:
  virtual ~Backend() = default;
  virtual bool stop() = 0;
  virtual void on_resize() = 0;
  virtual td::int32 width() = 0;
  virtual td::int32 height() = 0;
  virtual void tick() = 0;
  virtual void refresh(bool force, std::shared_ptr<Window> base_window) = 0;
  virtual td::int32 poll_fd() = 0;
  virtual void create_backend_window(std::shared_ptr<Window> window) {
  }
  virtual void delete_backend_window(Window *window) {
  }
};

class Screen {
 public:
  enum class BackendType { Auto, Notcurses, Tickit };
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_close() = 0;
    virtual void on_resize(td::int32 height, td::int32 width) {
    }
  };

  Screen(std::unique_ptr<Callback> callback, BackendType backend_type, std::string control_key);
  ~Screen();
  void init();
  void init_notcurses();
  void stop();
  void handle_input(const InputEvent &info);
  td::Timestamp loop();
  void on_resize(int height, int width);
  void refresh(bool force = false);
  void render(Window &root, WindowOutputter &rb, bool force);

  td::int32 width();
  td::int32 height();

  bool has_popup_window(Window *ptr) const;
  void add_popup_window(std::shared_ptr<Window> window, td::int32 priority);
  void del_popup_window(Window *window);
  void change_layout(std::shared_ptr<WindowLayout> window_layout);

  td::int32 poll_fd();

  void set_backend(std::unique_ptr<Backend> backend) {
    backend_ = std::move(backend);
  }

 private:
  void activate_window_in();

  std::unique_ptr<Backend> backend_;

  std::shared_ptr<Window> base_window_;
  std::unique_ptr<Callback> callback_;
  bool finished_{false};

  BackendType backend_type_;
  std::string control_key_;
  bool in_control_mode_{false};
};

}  // namespace windows
