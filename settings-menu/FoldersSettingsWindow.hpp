#pragma once

#include "common-windows/MenuWindowCommon.hpp"

namespace tdcurses {

class FoldersSettingsWindow : public MenuWindowCommon {
 public:
  FoldersSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor)
      : MenuWindowCommon(root, std::move(root_actor)) {
    build_menu();
  }
  void build_menu() {
  }
};

}  // namespace tdcurses
