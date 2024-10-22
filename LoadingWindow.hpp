#pragma once

#include "MenuWindow.hpp"
#include "td/utils/Promise.h"
#include "td/utils/Status.h"
#include "windows/EditorWindow.hpp"

namespace tdcurses {

class LoadingWindow
    : public MenuWindow
    , public windows::ViewWindow {
 public:
  LoadingWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor)
      : MenuWindow(root, root_actor), windows::ViewWindow("Running request...", nullptr) {
  }
  void handle_input(TickitKeyEventInfo *info) override {
    if (menu_window_handle_input(info)) {
      return;
    }
    return windows::ViewWindow::handle_input(info);
  }

  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> value) override {
    return std::static_pointer_cast<LoadingWindow>(std::move(value));
  }

  template <class T>
  void loading_window_send_request(td::tl_object_ptr<T> func, td::Promise<typename T::ReturnType> P) {
    auto promise =
        td::PromiseCreator::lambda([self = this, P = std::move(P)](td::Result<typename T::ReturnType> R) mutable {
          DROP_IF_DELETED(R);
          self->rollback();
          P.set_result(std::move(R));
        });
    send_request(std::move(func), std::move(promise));
  }
};

}  // namespace tdcurses
