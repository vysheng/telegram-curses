#include "telegram-curses-window-root.h"
#include "telegram-curses.hpp"

namespace tdcurses {

std::atomic<td::int64> TdcursesWindowBase::last_unique_id{0};

}  // namespace tdcurses
