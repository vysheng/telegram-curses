#include "config-window.h"
#include "telegram-curses-output.h"

namespace tdcurses {

td::int32 ConfigWindow::Element::render(windows::PadWindow &root, TickitRenderBuffer *rb, bool is_selected) {
  auto &opt = get_opt();
  Outputter out;
  out << opt.name << " " << opt.values[opt.cur_selected];
  return render_plain_text(rb, out.as_cslice(), out.markup(), width(), 1, is_selected);
}

Tdcurses::Option &ConfigWindow::Element::get_opt() const {
  return win_.get_opt(idx_);
}

}  // namespace tdcurses
