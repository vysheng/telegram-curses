#pragma once

#include "td/utils/Slice-decl.h"
#include "windows/markup.h"

#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"

#include "td/utils/buffer.h"
#include "td/utils/StringBuilder.h"
#include "td/utils/int_types.h"
#include <vector>

namespace tdcurses {

enum class Color : td::int32 {
  Revert = -1,
  Black = 0,
  Maroon,
  Green,
  Olive,
  Navy,
  Purple,
  Teal,
  Silver,
  Grey,
  Red,
  Lime,
  Yellow,
  Blue,
  Fuchsia,
  Aqua,
  White
};

class Outputter {
 public:
  Outputter() {
    args_.resize(TickitPenAttr::TICKIT_N_PEN_ATTRS);
  }
  struct Date {
    td::int32 date;
  };
  struct FgColor {
    FgColor(Color color) : color(color) {
    }
    Color color;
  };
  struct BgColor {
    BgColor(Color color) : color(color) {
    }
    Color color;
  };

  enum class ChangeBool { Enable, Disable, Revert, Invert };
  template <TickitPenAttr x>
  struct ChangeBoolImpl {
    explicit ChangeBoolImpl(bool v) {
      if (v) {
        type = ChangeBool::Enable;
      } else {
        type = ChangeBool::Revert;
      }
    }
    ChangeBoolImpl(ChangeBool t) : type(t) {
    }
    ChangeBool type;
    static constexpr TickitPenAttr attr() {
      return x;
    }
  };

  using Underline = ChangeBoolImpl<TickitPenAttr::TICKIT_PEN_UNDER>;
  using Bold = ChangeBoolImpl<TickitPenAttr::TICKIT_PEN_BOLD>;
  using Italic = ChangeBoolImpl<TickitPenAttr::TICKIT_PEN_ITALIC>;
  using Reverse = ChangeBoolImpl<TickitPenAttr::TICKIT_PEN_REVERSE>;
  using Blink = ChangeBoolImpl<TickitPenAttr::TICKIT_PEN_BLINK>;
  using Strike = ChangeBoolImpl<TickitPenAttr::TICKIT_PEN_STRIKE>;

  template <typename T>
  Outputter &operator<<(T x) {
    sb_ << x;
    return *this;
  }
  template <class T>
  std::enable_if_t<std::is_base_of<td::TlObject, T>::value, Outputter &> operator<<(const td::tl_object_ptr<T> &obj) {
    return *this << *obj.get();
  }
  template <class T>
  std::enable_if_t<!std::is_constructible<T>::value && std::is_base_of<td::TlObject, T>::value, Outputter &> operator<<(
      const T &X) {
    td::td_api::downcast_call(const_cast<T &>(X), [&](const auto &obj) { *this << obj; });
    return *this;
  }

  operator td::CSlice() {
    return sb_.as_cslice();
  }

  std::vector<windows::MarkupElement> markup();
  std::string as_str() {
    return sb_.as_cslice().str();
  }
  td::CSlice as_cslice() {
    return sb_.as_cslice();
  }

  Outputter &operator<<(Date date);
  Outputter &operator<<(FgColor color);
  Outputter &operator<<(BgColor color);
  Outputter &operator<<(Color color) {
    return *this << FgColor{color};
  }

  template <TickitPenAttr x>
  Outputter &operator<<(const ChangeBoolImpl<x> &el) {
    set_attr(el.attr(), el.type);
    return *this;
  }

  void add_markup(TickitPenAttr attr, size_t f, size_t l, td::int32 val);
  void set_attr_ex(TickitPenAttr attr, td::int32 val);
  void set_attr(TickitPenAttr attr, ChangeBool mode);

 private:
  std::vector<std::vector<std::pair<size_t, td::int32>>> args_;
  std::vector<windows::MarkupElement> markup_;
  td::StringBuilder sb_;
};

}  // namespace tdcurses
