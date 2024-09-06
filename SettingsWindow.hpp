#pragma once

#include "TdcursesWindowBase.hpp"
#include "windows/Window.hpp"

namespace tdcurses {

class SettingsWindow
    : public windows::PadWindow
    , public TdcursesWindowBase {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_exit() = 0;
  };
  SettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : TdcursesWindowBase(root, std::move(root_actor)) {
    set_pad_to(PadTo::Top);
    scroll_first_line();
  }

 private:
  td::int64 user_id_;
  std::string first_name_;
  std::string last_name_;
  std::string phone_number_;
  std::string username_;
};

}  // namespace tdcurses
