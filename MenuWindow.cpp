#include "MenuWindow.hpp"

namespace tdcurses {

std::shared_ptr<MenuWindow> create_menu_window(Tdcurses *root, td::ActorId<Tdcurses> root_actor,
                                               MenuWindowSpawnFunction cb) {
  auto res = cb(root, root_actor);
  res->update_history(std::vector<MenuWindowSpawnFunction>{cb});
  MenuWindow::create_popup(res);
  return res;
}

}  // namespace tdcurses
