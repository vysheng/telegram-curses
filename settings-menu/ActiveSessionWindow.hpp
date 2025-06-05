#pragma once

#include "common-windows/MenuWindowCommon.hpp"

namespace tdcurses {

class ActiveSessionWindow : public MenuWindowCommon {
 public:
  ActiveSessionWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::td_api::session *session)
      : MenuWindowCommon(root, std::move(root_actor)), session_(session) {
    build_menu();
  }

  void build_menu();

 private:
  td::td_api::session *session_;
};

}  // namespace tdcurses
