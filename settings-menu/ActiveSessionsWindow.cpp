#include "ActiveSessionsWindow.hpp"
#include "common-windows/ErrorWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include <memory>

namespace tdcurses {

void ActiveSessionsWindow::request_sessions() {
  auto req = td::make_tl_object<td::td_api::getActiveSessions>();
  send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::sessions>> R) {
    self->got_sessions(std::move(R));
  });
}

void ActiveSessionsWindow::got_sessions(td::Result<td::tl_object_ptr<td::td_api::sessions>> R) {
  if (R.is_error()) {
    spawn_error_window(*this, PSTRING() << "failed to get active sessions " << R.move_as_error(), {});
    return;
  }

  auto res = R.move_as_ok();
  td::int32 last_idx = 0;
  for (auto &s : res->sessions_) {
    auto e = std::make_shared<Element>(std::move(s), ++last_idx);
    add_element(e);
  }
}

}  // namespace tdcurses
