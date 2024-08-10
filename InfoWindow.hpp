#pragma once

#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "windows/EditorWindow.hpp"
#include "windows/PadWindow.hpp"
#include "TdcursesWindowBase.hpp"
#include "ChatManager.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace tdcurses {

class ChatInfoWindow
    : public windows::ViewWindow
    , public TdcursesWindowBase {
 public:
  class Callback : public windows::ViewWindow::Callback {
   public:
    void on_answer(ViewWindow *window) override {
      static_cast<ChatInfoWindow *>(window)->root()->hide_chat_info_window();
    }
    void on_abort(ViewWindow *window) override {
      static_cast<ChatInfoWindow *>(window)->root()->hide_chat_info_window();
    }
  };
  class Element : public windows::PadWindowElement {
   public:
    Element(ConfigWindow &win, size_t idx) : win_(win), idx_(idx) {
    }
    bool is_less(const PadWindowElement &other) const override {
      return idx_ < static_cast<const Element &>(other).idx_;
    }

    td::int32 render(windows::PadWindow &root, TickitRenderBuffer *rb, bool is_selected) override;

    Tdcurses::Option &get_opt() const;

   private:
    ConfigWindow &win_;
    size_t idx_;
  };
  ChatInfoWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::shared_ptr<Chat> chat)
      : windows::ViewWindow("", std::make_unique<Callback>())
      , TdcursesWindowBase(root, std::move(root_actor))
      , chat_(std::move(chat)) {
    generate_info();
  }

  void generate_info();
  void set_chat(std::shared_ptr<Chat> chat);
  void set_chat(std::shared_ptr<User> chat);
  void set_chat(std::shared_ptr<BasicGroup> chat);
  void set_chat(std::shared_ptr<Supergroup> chat);
  void set_chat(std::shared_ptr<SecretChat> chat);

  void handle_input(TickitKeyEventInfo *info) override {
    if (info->type == TICKIT_KEYEV_KEY) {
      if (!strcmp(info->str, "Enter") || !strcmp(info->str, "Escape")) {
        root()->hide_chat_info_window();
        return;
      }
    }
    return ViewWindow::handle_input(info);
  }

  td::int32 best_height() override {
    return 40;
  }

  auto chat() const {
    return chat_;
  }

  void got_chat(td::tl_object_ptr<td::td_api::chat> chat);
  void got_full_user(td::int64 chat_id, td::tl_object_ptr<td::td_api::userFullInfo> user);
  void got_full_basic_group(td::int64 chat_id, td::tl_object_ptr<td::td_api::basicGroupFullInfo> group);
  void got_full_super_group(td::int64 chat_id, td::tl_object_ptr<td::td_api::supergroupFullInfo> group);
  void got_chat_info(td::int64 chat_id, td::tl_object_ptr<td::td_api::chat> chat);

  void clear();
  bool is_empty() const {
    return chat_type_ == ChatType::Unknown && chat_inner_id_ == 0;
  }

 private:
  std::shared_ptr<Chat> chat_{nullptr};
  std::shared_ptr<User> user_{nullptr};
  td::tl_object_ptr<td::td_api::userFullInfo> user_full_;
  td::tl_object_ptr<td::td_api::basicGroupFullInfo> basic_group_full_;
  td::tl_object_ptr<td::td_api::supergroupFullInfo> supergroup_full_;
  ChatType chat_type_{ChatType::Unknown};
  td::int64 chat_inner_id_{0};
};

}  // namespace tdcurses
