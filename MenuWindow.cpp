#include "MenuWindow.hpp"
#include <memory>

namespace tdcurses {

/*std::shared_ptr<MenuWindow> create_menu_window(Tdcurses *root, td::ActorId<Tdcurses> root_actor,
                                               MenuWindowSpawnFunction cb) {
  auto res = cb(root, root_actor);
  res->update_history(std::vector<std::shared_ptr<MenuWindow>>{res});
  auto boxed = MenuWindow::create_boxed_window(res);
  res->root()->add_popup_window(boxed, 3);
  return res;
}*/

}  // namespace tdcurses
