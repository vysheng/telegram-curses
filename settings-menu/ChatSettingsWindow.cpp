#include "ChatSettingsWindow.hpp"
#include "GlobalParameters.hpp"
#include "LoadingWindow.hpp"

namespace tdcurses {

void ChatSettingsWindow::build_menu() {
  {
    bool markdown_enabled = global_parameters().use_markdown();
    Outputter out;
    if (markdown_enabled) {
      out << "enabled" << Outputter::RightPad{"<disable>"};
    } else {
      out << "disabled" << Outputter::RightPad{"<enable>"};
    }
    enabled_el_ = add_element("parse markdown", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
      bool enabled = !global_parameters().notifications_enabled();
      loading_window_send_request(
          w, "changing settings", {},
          td::make_tl_object<td::td_api::setOption>("X-markdown-enabled",
                                                    td::make_tl_object<td::td_api::optionValueBoolean>(enabled)),
          [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });

      global_parameters().set_notifications_enabled(enabled);
      Outputter out;
      if (enabled) {
        out << "enabled" << Outputter::RightPad{"<disable>"};
      } else {
        out << "disabled" << Outputter::RightPad{"<enable>"};
      }
      static_cast<ChatSettingsWindow &>(w).enabled_el_->menu_element()->data = out.as_str();
      static_cast<ChatSettingsWindow &>(w).enabled_el_->menu_element()->markup = out.markup();
      return false;
    });
  }
}

}  // namespace tdcurses
