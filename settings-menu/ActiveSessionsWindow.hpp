#pragma once

#include "MenuWindowPad.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/PadWindow.hpp"

namespace tdcurses {

class ActiveSessionsWindow : public MenuWindowPad {
 public:
  ActiveSessionsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindowPad(root, std::move(root_actor)) {
    request_sessions();
  }
  class Element : public windows::PadWindowElement {
   public:
    Element(td::tl_object_ptr<td::td_api::session> session, size_t idx) : session_(std::move(session)), idx_(idx) {
    }

    td::int32 render(windows::PadWindow &root, windows::WindowOutputter &rb, windows::SavedRenderedImagesDirectory &dir,
                     bool is_selected) override {
      Outputter out;
      out << session_->application_name_ << " " << session_->application_version_ << "\n";
      out << "    " << session_->platform_ << " " << session_->system_version_ << "\n";
      out << "    " << session_->device_model_ << "\n";
      out << "    " << session_->location_ << "\n";
      out << "    logged in   " << Outputter::Date{session_->log_in_date_} << "\n";
      out << "    last active " << Outputter::Date{session_->last_active_date_} << "\n";
      out << "\n";
      return render_plain_text(rb, out.as_cslice(), out.markup(), width(), 10, is_selected, &dir);
    }

    bool is_less(const PadWindowElement &other) const override {
      return idx_ < static_cast<const Element &>(other).idx_;
    }

   private:
    td::tl_object_ptr<td::td_api::session> session_;
    size_t idx_;
  };

  void request_sessions();
  void got_sessions(td::Result<td::tl_object_ptr<td::td_api::sessions>> R);

 private:
};

}  // namespace tdcurses
