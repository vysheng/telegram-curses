#pragma once

#include "common-windows/MenuWindowCommon.hpp"
#include "td/utils/Promise.h"
#include "windows/EditorWindow.hpp"
#include "windows/Markup.hpp"
#include "windows/Output.hpp"
#include "windows/Window.hpp"
#include "windows/ViewWindow.hpp"
#include <memory>
#include <type_traits>
#include <vector>

namespace tdcurses {

class SelectionWindow : public MenuWindowCommon {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_abort(SelectionWindow &) = 0;
    virtual void on_answer(SelectionWindow &, size_t answer) = 0;
  };
  SelectionWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::string text, std::vector<std::string> options,
                  std::unique_ptr<Callback> callback)
      : MenuWindowCommon(root, std::move(root_actor)), callback_(std::move(callback)) {
    set_title(text);
    size_t idx = 0;
    for (auto &el : options) {
      add_element(el, "", {}, [idx](MenuWindow &w) -> bool {
        static_cast<SelectionWindow &>(w).on_answer(idx);
        return false;
      });
      idx++;
    }
  }
  void on_answer(size_t idx) {
    if (!sent_answer_) {
      sent_answer_ = true;
      callback_->on_answer(*this, idx);
    }
  }

  td::int32 min_width() override {
    return 12;
  }
  td::int32 min_height() override {
    return 3;
  }
  td::int32 best_width() override {
    return 40;
  }

  ~SelectionWindow() {
    if (!sent_answer_) {
      sent_answer_ = true;
      callback_->on_abort(*this);
    }
  }

 private:
  std::unique_ptr<Callback> callback_;
  bool sent_answer_ = false;
};

template <typename T, typename F>
std::enable_if_t<std::is_base_of<MenuWindow, T>::value, std::shared_ptr<SelectionWindow>> spawn_selection_window(
    T &cur_window, std::string text, std::vector<std::string> options, F &&cb) {
  class Cb : public SelectionWindow::Callback {
   public:
    Cb(F &&cb, T *win) : cb_(std::move(cb)), self_(win), self_id_(win->window_unique_id()) {
    }
    void on_abort(SelectionWindow &w) override {
      if (w.root()->window_exists(self_id_)) {
        cb_(td::Status::Error("abort"));
      }
    }
    void on_answer(SelectionWindow &w, size_t idx) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
        cb_(idx);
      }
    }

   private:
    F cb_;
    T *self_;
    td::int64 self_id_;
  };
  auto callback = std::make_unique<Cb>(std::move(cb), &cur_window);
  return cur_window.template spawn_submenu<SelectionWindow>(std::move(text), std::move(options), std::move(callback));
}

}  // namespace tdcurses
