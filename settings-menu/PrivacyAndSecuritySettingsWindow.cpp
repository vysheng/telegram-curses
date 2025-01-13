#include "PrivacyAndSecuritySettingsWindow.hpp"
#include "ActiveSessionsWindow.hpp"

namespace tdcurses {

void PrivacyAndSecuritySettingsWindow::build_menu() {
  add_element("active sessions", "", {}, create_menu_window_spawn_function<ActiveSessionsWindow>());
}

}  // namespace tdcurses
