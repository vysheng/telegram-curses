#pragma once
#include "td/actor/impl/ActorId-decl.h"
#include "td/actor/impl/Scheduler-decl.h"
#include "PadWindow.hpp"

#include "td/utils/logging.h"
#include "windows/Markup.hpp"

#include <memory>
#include <mutex>
#include <list>
#include <string>
#include <vector>

namespace windows {

class LogWindow : public windows::PadWindow {
 public:
  LogWindow() {
    scroll_last_line();
  }
  void add_log_el(std::string el, std::vector<MarkupElement> markup) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      unprocessed_log_.emplace_back(el, std::move(markup));
    }
    set_need_refresh();
  }

  void render(WindowOutputter &rb, bool force) override {
    std::list<std::pair<std::string, std::vector<MarkupElement>>> r;
    {
      std::lock_guard<std::mutex> lock(mutex);
      r = std::move(unprocessed_log_);
    }
    for (auto &e : r) {
      class E : public windows::PadWindowElement {
       public:
        E(std::string text, std::vector<MarkupElement> markup, td::int64 id)
            : text_(std::move(text)), markup_(std::move(markup)), id_(id) {
        }

        td::int32 render(PadWindow &root, WindowOutputter &rb, windows::SavedRenderedImagesDirectory &dir,
                         bool is_selected) override {
          return render_plain_text(rb, text_, markup_, width(), 1, is_selected, &dir);
        }

        bool is_less(const PadWindowElement &el) const override {
          return id_ < static_cast<const E &>(el).id_;
        }

       private:
        std::string text_;
        std::vector<MarkupElement> markup_;
        td::int64 id_;
      };

      auto l = std::make_shared<E>(std::move(e.first), e.second, ++last_id_);
      add_element(l);
      elements_.push_back(std::move(l));
    }
    while (elements_.size() > max_size_) {
      auto l = elements_.front();
      elements_.pop_front();
      delete_element(l.get());
    }
    PadWindow::render(rb, force);
  }

 private:
  std::mutex mutex;
  std::list<std::pair<std::string, std::vector<MarkupElement>>> unprocessed_log_;
  std::list<std::shared_ptr<windows::PadWindowElement>> elements_;
  td::int64 last_id_{0};
  size_t max_size_{1000};
};

template <typename ActorType>
class WindowLogInterface : public td::LogInterface {
 public:
  WindowLogInterface(std::shared_ptr<LogWindow> window, td::ActorId<ActorType> root, td::LogInterface *next)
      : window_(std::move(window)), root_(std::move(root)), next_(next) {
  }

  void do_append(int log_level, td::CSlice slice) override {
    if (next_) {
      next_->do_append(log_level, slice);
    }
    std::vector<MarkupElement> markup;
    markup.push_back(std::make_shared<MarkupElementFgColor>(MarkupElementPos{0, 0},
                                                            MarkupElementPos{slice.size() + 5, 0}, Color::Grey));
    window_->add_log_el(PSTRING() << " *** " << slice.str(), std::move(markup));
    td::send_closure(root_, &td::Actor::loop);
  }

  void after_rotation() override {
    if (next_) {
      next_->after_rotation();
    }
  }

  td::vector<td::string> get_file_paths() override {
    if (next_) {
      return next_->get_file_paths();
    } else {
      return {};
    }
  }

 private:
  std::shared_ptr<LogWindow> window_;
  td::ActorId<ActorType> root_;
  td::LogInterface *next_{nullptr};
};

}  // namespace windows
