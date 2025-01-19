#pragma once

#include "td/telegram/StoryListId.h"
#include "td/tl/TlObject.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/format.h"
#include "windows/Markup.hpp"

#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"

#include "td/utils/buffer.h"
#include "td/utils/StringBuilder.h"
#include "td/utils/int_types.h"
#include "windows/Output.hpp"
#include <type_traits>
#include <vector>

namespace tdcurses {

using Color = windows::Color;

class ChatWindow;

class Outputter {
 public:
  Outputter() {
    args_.resize(windows::MarkupElement::Attr::Max);
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
  struct FgColorRgb {
    FgColorRgb(td::uint32 color) : color(color) {
    }
    td::uint32 color;
  };

  enum class ChangeBool { Enable, Disable, Revert, Invert };
  template <td::int32 x>
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
    static constexpr td::int32 attr() {
      return x;
    }
  };

  struct RightPad {
    RightPad(td::Slice data) : data(data) {
    }
    td::Slice data;
  };

  using Underline = ChangeBoolImpl<windows::MarkupElement::Attr::Underline>;
  using Bold = ChangeBoolImpl<windows::MarkupElement::Attr::Bold>;
  using Italic = ChangeBoolImpl<windows::MarkupElement::Attr::Italic>;
  using Reverse = ChangeBoolImpl<windows::MarkupElement::Attr::Reverse>;
  using Blink = ChangeBoolImpl<windows::MarkupElement::Attr::Blink>;
  using Strike = ChangeBoolImpl<windows::MarkupElement::Attr::Strike>;
  using NoLb = ChangeBoolImpl<windows::MarkupElement::Attr::NoLB>;

  template <typename T>
  std::enable_if_t<std::is_copy_constructible<T>::value, Outputter &> operator<<(T x) {
    sb_ << x;
    return *this;
  }
  template <class T>
  std::enable_if_t<std::is_constructible<T>::value && std::is_base_of<td::TlObject, T>::value, Outputter &> operator<<(
      const td::tl_object_ptr<T> &obj) {
    return *this << *obj.get();
  }
  template <class T>
  std::enable_if_t<!std::is_constructible<T>::value && std::is_base_of<td::TlObject, T>::value, Outputter &> operator<<(
      const T &X) {
    td::td_api::downcast_call(const_cast<T &>(X), [&](const auto &obj) { *this << obj; });
    return *this;
  }
  template <class T>
  std::enable_if_t<!std::is_constructible<T>::value && std::is_base_of<td::TlObject, T>::value, Outputter &> operator<<(
      const td::tl_object_ptr<T> &X) {
    td::td_api::downcast_call(const_cast<T &>(*X), [&](const auto &obj) { *this << obj; });
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
  Outputter &operator<<(FgColorRgb color);
  Outputter &operator<<(BgColor color);
  Outputter &operator<<(Color color) {
    return *this << FgColor{color};
  }
  Outputter &operator<<(const RightPad &x);

  template <td::int32 x>
  Outputter &operator<<(const ChangeBoolImpl<x> &el) {
    set_attr(el.attr(), el.type);
    return *this;
  }

  void add_markup(td::int32 attr, size_t f, size_t l, td::int32 val);
  void set_attr_ex(td::int32 attr, td::int32 val);
  void set_attr(td::int32 attr, ChangeBool mode);

  const td::td_api::message *get_message(td::int64 chat_id, td::int64 message_id);

  void set_chat(ChatWindow *chat) {
    cur_chat_ = chat;
  }

  struct Photo {
    td::CSlice path;
    td::int32 width;
    td::int32 height;
  };
  struct UserpicPhoto {
    td::CSlice path;
    td::int32 width;
    td::int32 height;
  };

  Outputter &operator<<(const Photo &);
  Outputter &operator<<(const UserpicPhoto &);

 private:
  std::vector<std::vector<std::pair<size_t, td::int32>>> args_;
  std::vector<windows::MarkupElement> markup_;
  ChatWindow *cur_chat_{nullptr};
  td::StringBuilder sb_;
};

}  // namespace tdcurses
