#include "NotificationsSettingsWindow.hpp"
#include "ScopeNotificationsSettingsWindow.hpp"
#include "common-windows/LoadingWindow.hpp"

namespace tdcurses {

void NotificationsSettingsWindow::build_menu() {
  {
    bool notifications_enabled = global_parameters().notifications_enabled();
    Outputter out;
    if (notifications_enabled) {
      out << "enabled" << Outputter::RightPad{"<disable>"};
    } else {
      out << "disabled" << Outputter::RightPad{"<enable>"};
    }
    enabled_el_ = add_element("desktop notifications", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
      bool enabled = !global_parameters().notifications_enabled();
      loading_window_send_request(
          w, "changing settings", {},
          td::make_tl_object<td::td_api::setOption>("X-notifications-enabled",
                                                    td::make_tl_object<td::td_api::optionValueBoolean>(enabled)),
          [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });

      global_parameters().set_notifications_enabled(enabled);
      Outputter out;
      if (enabled) {
        out << "enabled" << Outputter::RightPad{"<disable>"};
      } else {
        out << "disabled" << Outputter::RightPad{"<enable>"};
      }
      static_cast<NotificationsSettingsWindow &>(w).enabled_el_->menu_element()->data = out.as_str();
      static_cast<NotificationsSettingsWindow &>(w).enabled_el_->menu_element()->markup = out.markup();
      return false;
    });
  }
  {
    Outputter out;
    out << Outputter::RightPad{"<modify>"};
    add_element("private chats settings", out.as_str(), out.markup(),
                create_menu_window_spawn_function<ScopeNotificationsSettingsWindow>(
                    NotificationScope::NotificationScopePrivateChats));
  }
  {
    Outputter out;
    out << Outputter::RightPad{"<modify>"};
    add_element("groups settings", out.as_str(), out.markup(),
                create_menu_window_spawn_function<ScopeNotificationsSettingsWindow>(
                    NotificationScope::NotificationScopeGroupChats));
  }
  {
    Outputter out;
    out << Outputter::RightPad{"<modify>"};
    add_element("channel settings", out.as_str(), out.markup(),
                create_menu_window_spawn_function<ScopeNotificationsSettingsWindow>(
                    NotificationScope::NotificationScopeChannelChats));
  }
}

}  // namespace tdcurses
