#pragma once

#include "TdcursesWindowBase.hpp"
#include "MenuWindowCommon.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"

namespace tdcurses {

class AccountSettingsWindow : public MenuWindowCommon {
 public:
  AccountSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor)
      : MenuWindowCommon(root, std::move(root_actor)) {
    build_menu();
  }

  void build_menu();
  void got_full_user(td::tl_object_ptr<td::td_api::userFullInfo> user_full);

  void run_change_name(std::string first_name, std::string last_name);

 private:
};

class MainSettingsWindow : public MenuWindowCommon {
 public:
  MainSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindowCommon(root, std::move(root_actor)) {
    build_menu();
  }

  void build_menu();

 private:
};

}  // namespace tdcurses
