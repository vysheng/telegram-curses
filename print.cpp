#include "print.h"
#include "td/utils/port/Clocks.h"
#include "td/utils/format.h"
#include "td/utils/format.h"
#include "chat-window.h"

namespace td {
namespace format {

template <typename T>
struct ZeroPaddedInt {
  T value;
  td::uint32 pad;
};

StringBuilder &operator<<(StringBuilder &sb, ZeroPaddedInt<td::uint32> v) {
  char buf[64];
  auto l = sprintf(buf, "%0*u", v.pad, v.value);
  return sb << td::CSlice(buf, buf + l);
}

}  // namespace format
}  // namespace td

namespace tdcurses {

Outputter &Outputter::operator<<(Date date) {
  time_t t = date.date;
  struct tm d;
  localtime_r(&t, &d);

  *this << "[";
  auto now = td::Clocks::system();
  if (now - date.date > 60 * 86400) {
    *this << (d.tm_year + 1900) << "/";
  }
  if (now - date.date > 36000) {
    *this << td::format::ZeroPaddedInt<td::uint32>{static_cast<td::uint32>(d.tm_mon + 1), 2} << "/";
    *this << td::format::ZeroPaddedInt<td::uint32>{static_cast<td::uint32>(d.tm_mday), 2} << " ";
  }
  *this << td::format::ZeroPaddedInt<td::uint32>{static_cast<td::uint32>(d.tm_hour), 2} << ":"
        << td::format::ZeroPaddedInt<td::uint32>{static_cast<td::uint32>(d.tm_min), 2} << ":"
        << td::format::ZeroPaddedInt<td::uint32>{static_cast<td::uint32>(d.tm_sec), 2};
  return *this << "]";
}

void Outputter::add_markup(td::int32 attr, size_t f, size_t l, td::int32 val) {
  markup_.emplace_back(f, l, attr, val);
}

void Outputter::set_attr_ex(td::int32 attr, td::int32 attr_val) {
  auto val = (td::int32)attr;
  if (attr_val == -1) {
    CHECK(args_[val].size() > 0);
    add_markup(attr, args_[val].back().first, sb_.as_cslice().size(), args_[val].back().second);
    args_[val].pop_back();
    if (args_[val].size() > 0) {
      args_[val].back().first = sb_.as_cslice().size();
    }
  } else {
    if (args_[val].size() > 0) {
      add_markup(attr, args_[val].back().first, sb_.as_cslice().size(), args_[val].back().second);
    }
    args_[val].emplace_back(sb_.as_cslice().size(), attr_val);
  }
}

void Outputter::set_attr(td::int32 attr, ChangeBool mode) {
  td::int32 attr_val = 0;
  auto val = (td::int32)attr;
  switch (mode) {
    case ChangeBool::Disable:
      attr_val = 0;
      break;
    case ChangeBool::Enable:
      attr_val = 1;
      break;
    case ChangeBool::Revert:
      attr_val = -1;
      break;
    case ChangeBool::Invert:
      attr_val = args_[val].size() > 0 ? !args_[val].back().second : 1;
      break;
  }
  set_attr_ex(attr, attr_val);
}

Outputter &Outputter::operator<<(FgColor color) {
  set_attr_ex(TickitPenAttr::TICKIT_PEN_FG, (td::int32)color.color);
  return *this;
}

Outputter &Outputter::operator<<(BgColor color) {
  set_attr_ex(TickitPenAttr::TICKIT_PEN_BG, (td::int32)color.color);
  return *this;
}

std::vector<windows::MarkupElement> Outputter::markup() {
  auto res = markup_;
  for (size_t i = 0; i < args_.size(); i++) {
    if (args_[i].size() > 0) {
      res.emplace_back(args_[i].back().first, sb_.as_cslice().size(), (td::int32)i, args_[i].back().second);
    }
  }
  return res;
}

td::td_api::message *Outputter::get_message(td::int64 chat_id, td::int64 message_id) {
  if (!cur_chat_) {
    return nullptr;
  }
  if (cur_chat_->chat_id() != chat_id) {
    return nullptr;
  }
  auto el = cur_chat_->get_message(message_id);
  if (!el) {
    return nullptr;
  }
  return el->message.get();
}

}  // namespace tdcurses
