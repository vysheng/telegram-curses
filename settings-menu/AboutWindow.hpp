#pragma once

#include "MenuWindowView.hpp"

namespace tdcurses {

class AboutWindow : public MenuWindowView {
 public:
  AboutWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindowView(root, std::move(root_actor)) {
    build();
  }
  void build();
};

}  // namespace tdcurses
