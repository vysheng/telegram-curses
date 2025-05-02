#pragma once

#include "td/telegram/StickerType.h"
#include "td/utils/Promise.h"
#include "td/utils/Status.h"
#include "td/utils/common.h"
#include "windows/EditorWindow.hpp"
#include "windows/Markup.hpp"
#include <memory>
#include <vector>
#include "YesNoWindow.hpp"
#include "LoadingWindow.hpp"
#include "MenuWindowView.hpp"

namespace tdcurses {

template <typename T, typename T1>
void spawn_yes_no_window_and_loading_windows(T &cur_window, std::string yes_no_text,
                                             std::vector<windows::MarkupElement> yes_no_markup,
                                             bool yes_no_default_value, std::string loading_text,
                                             std::vector<windows::MarkupElement> loading_markup,
                                             td::tl_object_ptr<T1> func, td::Promise<typename T1::ReturnType> P) {
  spawn_yes_no_window(
      cur_window, std::move(yes_no_text), std::move(yes_no_markup),
      [&cur_window, loading_text = std::move(loading_text), loading_markup = std::move(loading_markup),
       func = std::move(func), P = std::move(P)](td::Result<bool> R) mutable {
        if (R.is_error()) {
          P.set_error(R.move_as_error());
          return;
        }
        if (!R.move_as_ok()) {
          P.set_error(td::Status::Error(0, "Cancelled"));
          return;
        }
        loading_window_send_request(cur_window, std::move(loading_text), std::move(loading_markup), std::move(func),
                                    std::move(P));
      },
      yes_no_default_value);
}

template <typename T>
std::enable_if_t<std::is_base_of<MenuWindow, T>::value, std::shared_ptr<MenuWindowView>> spawn_view_window(
    T &cur_window, std::string text, std::vector<windows::MarkupElement> markup = {}) {
  return cur_window.template spawn_submenu<MenuWindowView>(std::move(text), std::move(markup));
}

}  // namespace tdcurses
