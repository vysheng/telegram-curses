#include "ActiveSessionWindow.hpp"
#include "common-windows/Action.hpp"
#include "common-windows/ErrorWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "td/utils/overloaded.h"

namespace tdcurses {

void ActiveSessionWindow::build_menu() {
  //ip_address:string location:string = Session;
  {
    Outputter out;
    out << session_->id_ << Outputter::RightPad{"<terminate>"};
    add_element("session", out.as_str(), out.markup(), [](MenuWindowCommon &w) -> bool {
      auto &self = static_cast<ActiveSessionWindow &>(w);
      spawn_yes_no_window_and_loading_windows(
          self, "Are you sure, you want to terminate this session?", {}, true, "terminating", {},
          td::make_tl_object<td::td_api::terminateSession>(self.session_->id_),
          [&self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
            DROP_IF_DELETED(R);
            if (R.is_error()) {
              spawn_error_window(self, PSTRING() << "failed to terminate: " << R.move_as_error(), {});
            } else {
              self.exit();
            }
          });
      return false;
    });
  }
  {
    Outputter out;
    out << (session_->is_password_pending_ ? "YES" : "NO");
    add_element("password_pending", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << (session_->is_unconfirmed_ ? "YES" : "NO");
    if (session_->is_unconfirmed_) {
      out << Outputter::RightPad{"<confirm>"};
    }
    add_element("unconfirmed", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << (session_->can_accept_secret_chats_ ? "YES" : "NO");
    add_element("can_accept_secret_chats", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << (session_->can_accept_calls_ ? "YES" : "NO");
    add_element("can_accept_calls", out.as_str(), out.markup());
  }
  {
    Outputter out;
    td::td_api::downcast_call(*session_->device_type_,
                              td::overloaded([&](td::td_api::sessionDeviceTypeAndroid &) { out << "Android"; },
                                             [&](td::td_api::sessionDeviceTypeApple &) { out << "Apple"; },
                                             [&](td::td_api::sessionDeviceTypeBrave &) { out << "Brave"; },
                                             [&](td::td_api::sessionDeviceTypeChrome &) { out << "Chrome"; },
                                             [&](td::td_api::sessionDeviceTypeEdge &) { out << "Edge"; },
                                             [&](td::td_api::sessionDeviceTypeFirefox &) { out << "Firefox"; },
                                             [&](td::td_api::sessionDeviceTypeIpad &) { out << "Ipad"; },
                                             [&](td::td_api::sessionDeviceTypeIphone &) { out << "Iphone"; },
                                             [&](td::td_api::sessionDeviceTypeLinux &) { out << "Linux"; },
                                             [&](td::td_api::sessionDeviceTypeMac &) { out << "Mac"; },
                                             [&](td::td_api::sessionDeviceTypeOpera &) { out << "Opera"; },
                                             [&](td::td_api::sessionDeviceTypeSafari &) { out << "Safari"; },
                                             [&](td::td_api::sessionDeviceTypeUbuntu &) { out << "Ubuntu"; },
                                             [&](td::td_api::sessionDeviceTypeUnknown &) { out << "Unknown"; },
                                             [&](td::td_api::sessionDeviceTypeVivaldi &) { out << "Vivaldi"; },
                                             [&](td::td_api::sessionDeviceTypeWindows &) { out << "Windows"; },
                                             [&](td::td_api::sessionDeviceTypeXbox &) { out << "Xbox"; }));
    add_element("type", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << session_->api_id_;
    add_element("api_id", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << session_->application_name_;
    add_element("application_name", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << session_->application_version_;
    add_element("application_version", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << (session_->is_official_application_ ? "YES" : "NO");
    add_element("is_official", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << session_->device_model_;
    add_element("device_model", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << session_->platform_;
    add_element("platform", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << session_->system_version_;
    add_element("system_version", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << Outputter::Date{session_->log_in_date_};
    add_element("log_in_date", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << Outputter::Date{session_->last_active_date_};
    add_element("last_active_date", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << (session_->ip_address_.size() ? session_->ip_address_ : "<unknown>");
    add_element("ip_address", out.as_str(), out.markup());
  }
  {
    Outputter out;
    out << session_->location_;
    add_element("location", out.as_str(), out.markup());
  }
}

}  // namespace tdcurses
