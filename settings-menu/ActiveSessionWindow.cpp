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
    td::td_api::downcast_call(*session_->type_,
                              td::overloaded([&](td::td_api::sessionTypeAndroid &) { out << "Android"; },
                                             [&](td::td_api::sessionTypeApple &) { out << "Apple"; },
                                             [&](td::td_api::sessionTypeBrave &) { out << "Brave"; },
                                             [&](td::td_api::sessionTypeChrome &) { out << "Chrome"; },
                                             [&](td::td_api::sessionTypeEdge &) { out << "Edge"; },
                                             [&](td::td_api::sessionTypeFirefox &) { out << "Firefox"; },
                                             [&](td::td_api::sessionTypeIpad &) { out << "Ipad"; },
                                             [&](td::td_api::sessionTypeIphone &) { out << "Iphone"; },
                                             [&](td::td_api::sessionTypeLinux &) { out << "Linux"; },
                                             [&](td::td_api::sessionTypeMac &) { out << "Mac"; },
                                             [&](td::td_api::sessionTypeOpera &) { out << "Opera"; },
                                             [&](td::td_api::sessionTypeSafari &) { out << "Safari"; },
                                             [&](td::td_api::sessionTypeUbuntu &) { out << "Ubuntu"; },
                                             [&](td::td_api::sessionTypeUnknown &) { out << "Unknown"; },
                                             [&](td::td_api::sessionTypeVivaldi &) { out << "Vivaldi"; },
                                             [&](td::td_api::sessionTypeWindows &) { out << "Windows"; },
                                             [&](td::td_api::sessionTypeXbox &) { out << "Xbox"; }));
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
