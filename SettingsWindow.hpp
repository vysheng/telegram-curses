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

  static MenuWindowSpawnFunction spawn_function() {
    return [](Tdcurses *root, td::ActorId<Tdcurses> root_id) -> std::shared_ptr<MenuWindow> {
      return std::make_shared<AccountSettingsWindow>(root, root_id);
    };
  }

 private:
};

class MainSettingsWindow : public MenuWindowCommon {
 public:
  MainSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindowCommon(root, std::move(root_actor)) {
    build_menu();
  }

  void build_menu();

  static MenuWindowSpawnFunction spawn_function() {
    return [](Tdcurses *root, td::ActorId<Tdcurses> root_id) -> std::shared_ptr<MenuWindow> {
      return std::make_shared<MainSettingsWindow>(root, root_id);
    };
  }

 private:
};

}  // namespace tdcurses
