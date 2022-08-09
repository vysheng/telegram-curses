#pragma once

#include "windows/window.h"
#include "windows/padwindow.h"
#include "telegram-curses-window-root.h"
#include <functional>
#include <memory>
#include <vector>

namespace tdcurses {

class ConfigWindow
    : public windows::PadWindow
    , public TdcursesWindowBase {
 public:
  class Element : public windows::PadWindowElement {
   public:
    Element(ConfigWindow &win, size_t idx) : win_(win), idx_(idx) {
    }
    bool is_less(const PadWindowElement &other) const override {
      return idx_ < static_cast<const Element &>(other).idx_;
    }

    td::int32 render(TickitRenderBuffer *rb, bool is_selected) override;

    Tdcurses::Option &get_opt() const;

   private:
    ConfigWindow &win_;
    size_t idx_;
  };
  ConfigWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::vector<Tdcurses::Option> &options)
      : TdcursesWindowBase(root, std::move(root_actor)), options_(options) {
    for (size_t idx = 0; idx < options_.size(); idx++) {
      auto el = std::make_shared<Element>(*this, idx);
      elements_.emplace_back(el);
      add_element(el);
    }
  }

  void handle_input(TickitKeyEventInfo *info) override {
    if (info->type == TICKIT_KEYEV_KEY) {
      if (!strcmp(info->str, "Enter")) {
        root()->hide_config_window();
        return;
      }
    } else {
      if (!strcmp(info->str, " ")) {
        auto el = get_active_element();
        if (el) {
          auto e = std::static_pointer_cast<Element>(std::move(el));
          auto &op = e->get_opt();
          op.update();
          change_element(e.get());
        }
        return;
      }
    }
    return PadWindow::handle_input(info);
  }

  auto &get_opt(size_t idx) {
    return options_[idx];
  }

  td::int32 best_height() override {
    return (td::int32)options_.size();
  }

 private:
  std::vector<Tdcurses::Option> &options_;
  std::vector<std::shared_ptr<Element>> elements_;
};

}  // namespace tdcurses
