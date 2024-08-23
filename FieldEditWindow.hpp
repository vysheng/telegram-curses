#pragma once

#include "MenuWindow.hpp"
#include "td/utils/Promise.h"
#include "td/utils/Status.h"
#include "td/utils/common.h"
#include "windows/EditorWindow.hpp"
#include <functional>
#include <memory>

namespace tdcurses {

class FieldEditWindow
    : public MenuWindow
    , public windows::OneLineInputWindow {
 public:
  FieldEditWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::string prompt, std::string text,
                  std::function<void(FieldEditWindow &, std::string, td::Promise<td::Unit>)> update)
      : MenuWindow(root, root_actor), windows::OneLineInputWindow(prompt, text, false, nullptr) {
    class Callback : public windows::OneLineInputWindow::Callback {
     public:
      Callback(std::function<void(FieldEditWindow &, std::string, td::Promise<td::Unit>)> update) : update_(update) {
      }
      void on_answer(windows::OneLineInputWindow *_self, std::string text) override {
        auto self = static_cast<FieldEditWindow *>(_self);
        update_(*self, std::move(text),
                td::PromiseCreator::lambda([self](td::Result<td::Unit> R) { self->rollback(); }));
      }
      void on_abort(windows::OneLineInputWindow *self, std::string text) override {
        static_cast<FieldEditWindow *>(self)->rollback();
      }

     private:
      std::function<void(FieldEditWindow &, std::string, td::Promise<td::Unit>)> update_;
    };
    set_callback(std::make_unique<Callback>(update));
  }

  void handle_input(TickitKeyEventInfo *info) override {
    if (info->type == TICKIT_KEYEV_KEY) {
      if (menu_window_handle_input(info)) {
        return;
      }
    }
    return OneLineInputWindow::handle_input(info);
  }

  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> value) override {
    return std::static_pointer_cast<FieldEditWindow>(std::move(value));
  }

  static MenuWindowSpawnFunction spawn_function(
      std::string prompt, std::string text,
      std::function<void(FieldEditWindow &, std::string, td::Promise<td::Unit>)> update) {
    return [prompt, text, update](Tdcurses *root, td::ActorId<Tdcurses> root_id) -> std::shared_ptr<MenuWindow> {
      return std::make_shared<FieldEditWindow>(root, root_id, prompt, text, update);
    };
  }

  td::int32 best_width() override {
    return 10000;
  }
};

}  // namespace tdcurses
