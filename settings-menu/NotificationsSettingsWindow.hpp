#pragma once

#include "MenuWindowCommon.hpp"

namespace tdcurses {

class NotificationsSettingsWindow : public MenuWindowCommon {
 public:
  NotificationsSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor)
      : MenuWindowCommon(root, std::move(root_actor)) {
    build_menu();
  }
  void build_menu();
  void enabled_disable();

 private:
  std::shared_ptr<ElInfo> enabled_el_;
};

}  // namespace tdcurses
