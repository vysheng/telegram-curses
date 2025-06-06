#pragma once

#include "td/actor/impl/ActorId-decl.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "td/utils/Variant.h"
#include "windows/Output.hpp"
#include "windows/PadWindow.hpp"
#include "td/generate/auto/td/telegram/td_api.h"
#include "td/generate/auto/td/telegram/td_api.hpp"
#include "TdcursesWindowBase.hpp"
#include "ChatManager.hpp"
#include "CommandLineWindow.hpp"
#include <memory>

namespace tdcurses {

class Tdcurses;

class DialogListWindow
    : public windows::PadWindow
    , public TdcursesWindowBase {
 public:
  struct SublistGlobal {
    bool operator==(const SublistGlobal &) const {
      return true;
    }
  };
  struct SublistArchive {
    bool operator==(const SublistArchive &) const {
      return true;
    }
  };
  struct SublistSublist {
    bool operator==(const SublistSublist &other) const {
      return sublist_id_ == other.sublist_id_;
    }
    td::int32 sublist_id_;
  };
  struct SublistSearch {
    bool operator==(const SublistSearch &other) const {
      return search_pattern_ == other.search_pattern_;
    }
    std::string search_pattern_;
  };

  using Sublist = td::Variant<SublistGlobal, SublistArchive, SublistSublist, SublistSearch>;
  DialogListWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : TdcursesWindowBase(root, std::move(root_actor)) {
    set_pad_to(PadTo::Top);
    clear();
    set_title("global");
  }
  class Element;

  class Element
      : public windows::PadWindowElement
      , public Chat {
   public:
    Element(td::tl_object_ptr<td::td_api::chat> chat) : Chat(std::move(chat)) {
    }

    td::int32 render(windows::PadWindow &root, windows::WindowOutputter &rb, windows::SavedRenderedImagesDirectory &dir,
                     bool is_selected) override;

    bool is_less(const windows::PadWindowElement &elx) const override {
      const Element &el = static_cast<const Element &>(elx);

      return order_ > el.order_ || (order_ == el.order_ && chat_id() > el.chat_id());
    }
    bool is_visible() const override {
      return order_ != 0;
    }

    bool is_pinned(const Sublist &cur_sublist);
    void update_order(const Sublist &cur_sublist);
    void force_update_order(td::int64 order) {
      order_ = order;
    }
    void update_sublist(const Sublist &cur_sublist);

    auto cur_order() const {
      return order_;
    }

   private:
    td::int64 order_{0};
  };

  void handle_input(const windows::InputEvent &info) override;

  void request_bottom_elements() override;
  void received_bottom_elements(td::Result<td::tl_object_ptr<td::td_api::ok>> R);
  void received_bottom_chats(td::Result<td::tl_object_ptr<td::td_api::chats>> R);

  template <typename T>
  void process_update(T &upd) {
    chat_manager().process_update(upd);
    auto chat = chat_manager().get_chat(upd.chat_id_);
    if (chat) {
      change_element(static_cast<Element *>(chat.get()));
    }
  }

  template <typename T>
  void process_update(td::int64 chat_id, T &upd) {
    chat_manager().process_update(chat_id, upd);
    auto chat = chat_manager().get_chat(chat_id);
    if (chat) {
      change_element(static_cast<Element *>(chat.get()));
    }
  }

  template <typename T>
  void process_user_update(T &upd) {
    chat_manager().process_update(upd);
  }

  void process_update(td::td_api::updateNewChat &update);
  void process_update(td::td_api::updateChatLastMessage &update);
  void process_update(td::td_api::updateChatPosition &update);

  void set_search_pattern(std::string pattern);
  void update_sublist(Sublist new_sublist);
  void set_sublist(Sublist new_sublist);

  const auto &cur_sublist() const {
    return cur_sublist_;
  }

  void scroll_to_chat(td::int64 chat_id);

 private:
  bool running_req_{false};
  bool is_completed_{false};
  Sublist cur_sublist_{SublistGlobal{}};
};

}  // namespace tdcurses
