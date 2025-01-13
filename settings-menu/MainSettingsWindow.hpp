#pragma once

#include "MenuWindowCommon.hpp"

namespace tdcurses {

class MainSettingsWindow : public MenuWindowCommon {
 public:
  MainSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindowCommon(root, std::move(root_actor)) {
    build_menu();
    send_requests();
  }

  void build_menu();
  void send_requests();

  void got_premium(td::tl_object_ptr<td::td_api::premiumState> res);

 private:
  std::shared_ptr<ElInfo> premium_;
};

}  // namespace tdcurses
