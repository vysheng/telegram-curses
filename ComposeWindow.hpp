#pragma once

#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/EditorWindow.hpp"
#include "TdcursesWindowBase.hpp"

namespace tdcurses {

class ComposeWindow
    : public windows::EditorWindow
    , public TdcursesWindowBase {
 public:
  ComposeWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id, std::string text)
      : EditorWindow(std::move(text), nullptr), TdcursesWindowBase(root, std::move(root_actor)), chat_id_(chat_id) {
    install_callback();
  }

  void install_callback();
  void send_message(std::string message);
  void message_sent(td::Result<td::tl_object_ptr<td::td_api::message>> result);
  void set_draft(std::string message);

 private:
  td::int64 chat_id_;
};

}  // namespace tdcurses
