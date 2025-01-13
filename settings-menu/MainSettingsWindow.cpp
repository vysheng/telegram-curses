#include "MainSettingsWindow.hpp"
#include "TdObjectsOutput.h"
#include "AccountSettingsWindow.hpp"
#include "NotificationsSettingsWindow.hpp"
#include "ChatSettingsWindow.hpp"
#include "FoldersSettingsWindow.hpp"
#include "AdvancedSettingsWindow.hpp"
#include "PrivacyAndSecuritySettingsWindow.hpp"

namespace tdcurses {

void MainSettingsWindow::build_menu() {
  add_element("account settings", "", {}, create_menu_window_spawn_function<AccountSettingsWindow>());
  add_element("notification settings", "", {}, create_menu_window_spawn_function<NotificationsSettingsWindow>());
  add_element("privacy and security settings", "", {},
              create_menu_window_spawn_function<PrivacyAndSecuritySettingsWindow>());
  add_element("chat settings", "", {}, create_menu_window_spawn_function<ChatSettingsWindow>());
  add_element("folders settings", "", {}, create_menu_window_spawn_function<FoldersSettingsWindow>());
  add_element("advanced settings", "", {}, create_menu_window_spawn_function<AdvancedSettingsWindow>());

  premium_ = add_element("premium", "");
  {
    const auto &stars_amount = global_parameters().stars_owned();
    if (stars_amount) {
      auto amount = (double)stars_amount->star_count_ + 1e-9 * stars_amount->nanostar_count_;
      add_element("stars owned", PSTRING() << amount);
    } else {
      add_element("stars owned", "0");
    }
  }
}

void MainSettingsWindow::send_requests() {
  auto req = td::make_tl_object<td::td_api::getPremiumState>();
  send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::premiumState>> R) {
    if (R.is_error()) {
      return;
    }
    self->got_premium(R.move_as_ok());
  });
}

void MainSettingsWindow::got_premium(td::tl_object_ptr<td::td_api::premiumState> res) {
  if (!res) {
    return;
  }
  Outputter out;
  out << *res->state_;

  change_element(premium_, [&]() {
    premium_->menu_element()->data = out.as_str();
    premium_->menu_element()->markup = out.markup();
  });
  set_need_refresh();
}

}  // namespace tdcurses
