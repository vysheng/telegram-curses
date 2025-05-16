#include "AttachMenu.hpp"
#include "ComposeWindow.hpp"

namespace tdcurses {

void AttachMenu::add_option_clear() {
  add_element("clear", "clear all attachments", {},
              [root = root(), compose_window = compose_window_, compose_window_id = compose_window_id_]() {
                if (!root->window_exists(compose_window_id)) {
                  return true;
                }
                compose_window->update_attach(AttachType::None, {});
                return true;
              });
}

}  // namespace tdcurses
