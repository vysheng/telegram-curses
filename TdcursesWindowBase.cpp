#include "TdcursesWindowBase.hpp"

namespace tdcurses {

std::atomic<td::int64> TdcursesWindowBase::last_unique_id{0};

}  // namespace tdcurses
