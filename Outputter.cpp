#include "Outputter.hpp"
#include "td/utils/Slice-decl.h"
#include "td/utils/overloaded.h"
#include "td/utils/port/Clocks.h"
#include "td/utils/format.h"
#include "td/utils/format.h"
#include "ChatWindow.hpp"
#include "windows/Markup.hpp"
#include "windows/unicode.h"
#include <memory>

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

Outputter &Outputter::operator<<(FgColor color) {
  set_color(color.color, true);
  return *this;
}

Outputter &Outputter::operator<<(FgColorRgb color) {
  set_color(color.color, true);
  return *this;
}

Outputter &Outputter::operator<<(BgColor color) {
  set_color(color.color, false);
  return *this;
}

Outputter &Outputter::operator<<(BgColorRgb color) {
  set_color(color.color, false);
  return *this;
}

void Outputter::set_color(td::Variant<Color, ColorRGB> color, bool is_fg) {
  auto &l = (is_fg ? fg_colors_stack_ : bg_colors_stack_);
  if (color.get_offset() == color.offset<Color>() && color.get<Color>() == Color::Revert) {
    l.pop_arg(*this, sb_.as_cslice().size(), markup_idx_++);
  } else {
    l.push_arg(*this, sb_.as_cslice().size(), markup_idx_++,
               [color = std::move(color), is_fg](size_t from_pos, size_t from_idx, size_t to_pos,
                                                 size_t to_idx) -> windows::MarkupElement {
                 windows::MarkupElementPos from(from_pos, from_idx);
                 windows::MarkupElementPos to(to_pos, to_idx);
                 windows::MarkupElement el;
                 if (is_fg) {
                   color.visit(td::overloaded(
                       [&](Color c) { el = std::make_shared<windows::MarkupElementFgColor>(from, to, c); },
                       [&](ColorRGB c) { el = std::make_shared<windows::MarkupElementFgColorRGB>(from, to, c); }));
                 } else {
                   color.visit(td::overloaded(
                       [&](Color c) { el = std::make_shared<windows::MarkupElementBgColor>(from, to, c); },
                       [&](ColorRGB c) { el = std::make_shared<windows::MarkupElementBgColorRGB>(from, to, c); }));
                 }
                 return el;
               });
  }
}

std::vector<windows::MarkupElement> Outputter::markup() {
  auto res = markup_;
  fg_colors_stack_.flush_to(res, sb_.as_cslice().size(), markup_idx_++);
  bg_colors_stack_.flush_to(res, sb_.as_cslice().size(), markup_idx_++);
  pad_left_stack_.flush_to(res, sb_.as_cslice().size(), markup_idx_++);
  for (auto &e : bool_stack_) {
    e->flush_to(res, sb_.as_cslice().size(), markup_idx_++);
  }
  return res;
}

const td::td_api::message *Outputter::get_message(td::int64 chat_id, td::int64 message_id) {
  if (!cur_chat_) {
    return nullptr;
  }
  return cur_chat_->get_message_as_message(chat_id, message_id);
}

Outputter &Outputter::operator<<(const LeftPad &x) {
  if (x.pad.size() > 0) {
    pad_left_stack_.push_arg(
        *this, sb_.as_cslice().size(), markup_idx_++,
        [pad = std::move(x.pad), color = std::move(x.color)](size_t from_pos, size_t from_idx, size_t to_pos,
                                                             size_t to_idx) -> windows::MarkupElement {
          windows::MarkupElementPos from(from_pos, from_idx);
          windows::MarkupElementPos to(to_pos, to_idx);
          windows::MarkupElement el;
          color.visit(td::overloaded(
              [&](Color c) { el = std::make_shared<windows::MarkupElementLeftPad>(from, to, pad, c); },
              [&](ColorRGB c) { el = std::make_shared<windows::MarkupElementLeftPad>(from, to, pad, c); }));
          return el;
        });
  } else {
    pad_left_stack_.pop_arg(*this, sb_.as_cslice().size(), markup_idx_++);
  }
  return *this;
}

Outputter &Outputter::operator<<(const RightPad &x) {
  td::int32 code = RIGHT_ALIGN_BLOCK_START + (td::int32)x.data.size();
  char buf[6];
  utf8_code_to_str(code, buf);
  return *this << td::CSlice((char *)buf) << x.data;
}

Outputter &Outputter::operator<<(const Photo &obj) {
  windows::MarkupElementPos from(sb_.as_cslice().size(), markup_idx_++);
  windows::MarkupElementPos to(sb_.as_cslice().size(), markup_idx_++);
  markup_.push_back(
      std::make_shared<windows::MarkupElementImage>(from, to, obj.path.str(), obj.height, obj.width, 20, 1000, true));
  return *this;
}

Outputter &Outputter::operator<<(const UserpicPhoto &obj) {
  windows::MarkupElementPos from(sb_.as_cslice().size(), markup_idx_++);
  windows::MarkupElementPos to(sb_.as_cslice().size(), markup_idx_++);
  markup_.push_back(
      std::make_shared<windows::MarkupElementImage>(from, to, obj.path.str(), obj.height, obj.width, 2, 4, true));
  return *this;
}

Outputter &Outputter::operator<<(const UserpicPhotoData &obj) {
  windows::MarkupElementPos from(sb_.as_cslice().size(), markup_idx_++);
  windows::MarkupElementPos to(sb_.as_cslice().size(), markup_idx_++);
  markup_.push_back(
      std::make_shared<windows::MarkupElementImageData>(from, to, obj.data.str(), obj.height, obj.width, 2, 4, true));
  return *this;
}

Outputter &Outputter::operator<<(const SoftTab &c) {
  char buf[6];
  td::uint32 cp = SOFT_TAB_START_CP + (c.size & 0x1f);
  td::uint32 r = utf8_code_to_str(cp, buf);
  return *this << td::Slice(buf, buf + r);
}

}  // namespace tdcurses
