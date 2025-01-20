#pragma once

#include "td/telegram/StoryListId.h"
#include "td/tl/TlObject.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/Variant.h"
#include "td/utils/format.h"
#include "windows/Markup.hpp"

#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"

#include "td/utils/buffer.h"
#include "td/utils/StringBuilder.h"
#include "td/utils/int_types.h"
#include "windows/Output.hpp"
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>
#include <list>

namespace tdcurses {

using Color = windows::Color;

class ChatWindow;

class Outputter {
 public:
  Outputter() {
    //args_.resize(windows::MarkupElement::Attr::Max);
    bool_stack_.push_back(std::make_unique<ArgListBoolImpl<windows::MarkupElementUnderline>>());
    bool_stack_.push_back(std::make_unique<ArgListBoolImpl<windows::MarkupElementBold>>());
    bool_stack_.push_back(std::make_unique<ArgListBoolImpl<windows::MarkupElementItalic>>());
    bool_stack_.push_back(std::make_unique<ArgListBoolImpl<windows::MarkupElementReverse>>());
    bool_stack_.push_back(std::make_unique<ArgListBoolImpl<windows::MarkupElementBlink>>());
    bool_stack_.push_back(std::make_unique<ArgListBoolImpl<windows::MarkupElementStrike>>());
    bool_stack_.push_back(std::make_unique<ArgListBoolImpl<windows::MarkupElementNoLb>>());
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
  struct BgColorRgb {
    BgColorRgb(td::uint32 color) : color(color) {
    }
    td::uint32 color;
  };

  enum class ChangeBool { Enable, Disable, Revert };
  template <typename T, size_t idx>
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
    size_t get_idx() const {
      return idx;
    }
    ChangeBool type;
  };

  struct RightPad {
    RightPad(td::Slice data) : data(data) {
    }
    td::Slice data;
  };

  using Underline = ChangeBoolImpl<windows::MarkupElementUnderline, 0>;
  using Bold = ChangeBoolImpl<windows::MarkupElementBold, 1>;
  using Italic = ChangeBoolImpl<windows::MarkupElementItalic, 2>;
  using Reverse = ChangeBoolImpl<windows::MarkupElementReverse, 3>;
  using Blink = ChangeBoolImpl<windows::MarkupElementBlink, 4>;
  using Strike = ChangeBoolImpl<windows::MarkupElementStrike, 5>;
  using NoLb = ChangeBoolImpl<windows::MarkupElementNoLb, 6>;

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
  Outputter &operator<<(BgColorRgb color);
  Outputter &operator<<(Color color) {
    return *this << FgColor{color};
  }
  Outputter &operator<<(const RightPad &x);

  template <typename T, size_t x>
  Outputter &operator<<(const ChangeBoolImpl<T, x> &el) {
    bool_stack_[x]->push_arg(*this, sb_.as_cslice().size(), el.type);
    return *this;
  }

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
  void set_color(td::Variant<Color, td::uint32> color, bool is_fg);

  std::vector<windows::MarkupElement> markup_;
  ChatWindow *cur_chat_{nullptr};
  td::StringBuilder sb_;

  struct Arg {
    Arg(size_t from_pos, std::function<windows::MarkupElement(size_t, size_t)> create)
        : from_pos(from_pos), create(std::move(create)) {
    }
    size_t from_pos;
    std::function<windows::MarkupElement(size_t, size_t)> create;
  };
  class ArgList {
   private:
    std::list<Arg> args;

   public:
    virtual ~ArgList() = default;
    void push_arg(Outputter &out, size_t pos, std::function<windows::MarkupElement(size_t, size_t)> create) {
      if (args.size() > 0) {
        out.markup_.push_back(args.back().create(args.back().from_pos, pos));
      }
      args.emplace_back(pos, std::move(create));
    }
    void pop_arg(Outputter &out, size_t pos) {
      CHECK(args.size() > 0);
      out.markup_.push_back(args.back().create(args.back().from_pos, pos));
      args.pop_back();
    }
    void flush(Outputter &out, size_t pos) {
      if (args.size() > 0) {
        out.markup_.push_back(args.back().create(args.back().from_pos, pos));
        args.back().from_pos = pos;
      }
    }
    void flush_to(std::vector<windows::MarkupElement> &markup, size_t pos) {
      if (args.size() > 0) {
        markup.push_back(args.back().create(args.back().from_pos, pos));
      }
    }
  };

  class ArgListBool : public ArgList {
   public:
    virtual void push_arg(Outputter &out, size_t pos, ChangeBool value) = 0;
  };

  template <typename T>
  class ArgListBoolImpl : public ArgListBool {
   public:
    void push_arg(Outputter &out, size_t pos, ChangeBool value) override {
      switch (value) {
        case ChangeBool::Revert:
          pop_arg(out, pos);
          return;
        case ChangeBool::Enable:
          ArgList::push_arg(out, pos, [](size_t from, size_t to) -> windows::MarkupElement {
            return std::make_shared<T>(from, to, true);
          });
          return;
        case ChangeBool::Disable:
          ArgList::push_arg(out, pos, [](size_t from, size_t to) -> windows::MarkupElement {
            return std::make_shared<T>(from, to, false);
          });
          return;
      }
    }
  };

  ArgList fg_colors_stack_;
  ArgList bg_colors_stack_;
  std::vector<std::unique_ptr<ArgListBool>> bool_stack_;
};

}  // namespace tdcurses
