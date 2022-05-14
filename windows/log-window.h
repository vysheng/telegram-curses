#pragma once
#include "td/actor/impl/ActorId-decl.h"
#include "td/actor/impl/Scheduler-decl.h"
#include "padwindow.h"

#include "td/utils/logging.h"

#include <memory>
#include <mutex>
#include <list>
#include <set>
#include <string>

namespace windows {

class LogWindow : public windows::PadWindow {
 public:
  LogWindow() {
    scroll_last_line();
  }
  void add_log_el(std::string el) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      unprocessed_log_.push_back(el);
    }
    set_need_refresh();
  }

  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override {
    std::list<std::string> r;
    {
      std::lock_guard<std::mutex> lock(mutex);
      r = std::move(unprocessed_log_);
    }
    for (auto &e : r) {
      class E : public windows::PadWindowElement {
       public:
        E(std::string text, td::int64 id) : text_(std::move(text)), id_(id) {
        }

        td::int32 render(TickitRenderBuffer *rb, bool is_selected) override {
          return render_plain_text(rb, text_, width(), 1, is_selected);
        }

        bool is_less(const PadWindowElement &el) const override {
          return id_ < static_cast<const E &>(el).id_;
        }

       private:
        std::string text_;
        td::int64 id_;
      };

      add_element(std::make_shared<E>(std::move(e), ++last_id_));
    }
    PadWindow::render(rb, cursor_x, cursor_y, cursor_shape, force);
  }

 private:
  std::mutex mutex;
  std::list<std::string> unprocessed_log_;
  td::int64 last_id_{0};
};

template <typename ActorType>
class WindowLogInterface : public td::LogInterface {
 public:
  WindowLogInterface(std::shared_ptr<LogWindow> window, td::ActorId<ActorType> root)
      : window_(std::move(window)), root_(std::move(root)) {
  }
  void do_append(int log_level, td::CSlice slice) override {
    window_->add_log_el(slice.str());
    td::send_closure(root_, &td::Actor::loop);
  }

 private:
  std::shared_ptr<LogWindow> window_;
  td::ActorId<ActorType> root_;
};

}  // namespace windows
