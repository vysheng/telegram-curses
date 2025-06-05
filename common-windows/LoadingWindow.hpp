#pragma once

#include "MenuWindow.hpp"
#include "td/utils/Promise.h"
#include "td/utils/Status.h"
#include "td/utils/common.h"
#include "windows/ViewWindow.hpp"
#include <memory>

namespace tdcurses {

class LoadingWindow
    : public MenuWindow
    , public windows::ViewWindow {
 public:
  LoadingWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::string text = "Running request...",
                std::vector<windows::MarkupElement> markup = {})
      : MenuWindow(root, root_actor), windows::ViewWindow(std::move(text), std::move(markup), nullptr) {
  }
  void handle_input(const windows::InputEvent &info) override {
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

  template <class Func>
  void run_lambda(Func &&f, td::Promise<td::Unit> P) {
    auto promise = td::PromiseCreator::lambda([self = this, P = std::move(P)](td::Result<td::Unit> R) mutable {
      DROP_IF_DELETED(R);
      self->rollback();
      P.set_result(std::move(R));
    });
    f(*this, std::move(promise));
  }
};

template <typename T, typename T1>
std::enable_if_t<std::is_base_of<MenuWindow, T>::value, std::shared_ptr<MenuWindow>> loading_window_send_request(
    T &cur_window, std::string text, std::vector<windows::MarkupElement> markup, td::tl_object_ptr<T1> func,
    td::Promise<typename T1::ReturnType> P) {
  std::shared_ptr<LoadingWindow> r =
      cur_window.template spawn_submenu<LoadingWindow>(std::move(text), std::move(markup));
  r->loading_window_send_request(std::move(func), std::move(P));
  return r;
}

template <typename T, typename T1>
std::enable_if_t<!std::is_base_of<MenuWindow, T>::value && std::is_base_of<TdcursesWindowBase, T>::value,
                 std::shared_ptr<MenuWindow>>
loading_window_send_request(T &cur_window, std::string text, std::vector<windows::MarkupElement> markup,
                            td::tl_object_ptr<T1> func, td::Promise<typename T1::ReturnType> P) {
  std::shared_ptr<LoadingWindow> r = create_menu_window<LoadingWindow>(cur_window.root(), cur_window.root_actor_id(),
                                                                       std::move(text), std::move(markup));
  r->loading_window_send_request(std::move(func), std::move(P));
  return r;
}

}  // namespace tdcurses
