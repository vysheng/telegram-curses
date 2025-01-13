#pragma once

#include "MenuWindowCommon.hpp"

namespace tdcurses {

class AdvancedSettingsWindow : public MenuWindowCommon {
 public:
  AdvancedSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor)
      : MenuWindowCommon(root, std::move(root_actor)) {
    build_menu();
  }
  void build_menu() {
  }
};

}  // namespace tdcurses
