#pragma once

#include "td/tl/TlObject.h"
#include "auto/td/telegram/td_api.h"
#include "windows/editorwindow.h"
#include <memory>

namespace tdcurses {

class DebugInfoWindow : public windows::ViewWindow {
 public:
  DebugInfoWindow() : windows::ViewWindow("", nullptr) {
  }

  void create_text(const td::td_api::message &message);

 private:
};

}  // namespace tdcurses
