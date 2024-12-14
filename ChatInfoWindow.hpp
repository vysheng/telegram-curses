#pragma once

#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "windows/EditorWindow.hpp"
#include "windows/Output.hpp"
#include "windows/PadWindow.hpp"
#include "TdcursesWindowBase.hpp"
#include "ChatManager.hpp"
#include "MenuWindowCommon.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace tdcurses {

class ChatInfoWindow : public MenuWindowCommon {
 public:
  ChatInfoWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::shared_ptr<Chat> chat)
      : MenuWindowCommon(root, std::move(root_actor)) {
    set_chat(std::move(chat));
  }
  ChatInfoWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::shared_ptr<User> user)
      : MenuWindowCommon(root, std::move(root_actor)) {
    set_chat(std::move(user));
  }
  ChatInfoWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::shared_ptr<BasicGroup> group)
      : MenuWindowCommon(root, std::move(root_actor)) {
    set_chat(std::move(group));
  }
  ChatInfoWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::shared_ptr<Supergroup> group)
      : MenuWindowCommon(root, std::move(root_actor)) {
    set_chat(std::move(group));
  }
  ChatInfoWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::shared_ptr<SecretChat> group)
      : MenuWindowCommon(root, std::move(root_actor)) {
    set_chat(std::move(group));
  }

  void generate_info();
  void set_chat(std::shared_ptr<Chat> chat);
  void set_chat(std::shared_ptr<User> chat);
  void set_chat(std::shared_ptr<BasicGroup> chat);
  void set_chat(std::shared_ptr<Supergroup> chat);
  void set_chat(std::shared_ptr<SecretChat> chat);

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

  bool is_empty() const {
    return chat_type_ == ChatType::Unknown && chat_inner_id_ == 0;
  }

  void updated_chat_title(std::string new_title);
  void updated_user_name(std::string first_name, std::string last_name);

 private:
  std::shared_ptr<Chat> chat_{nullptr};
  std::shared_ptr<User> user_{nullptr};
  td::tl_object_ptr<td::td_api::userFullInfo> user_full_;
  td::tl_object_ptr<td::td_api::basicGroupFullInfo> basic_group_full_;
  td::tl_object_ptr<td::td_api::supergroupFullInfo> supergroup_full_;
  ChatType chat_type_{ChatType::Unknown};
  td::int64 chat_inner_id_{0};

  std::shared_ptr<ElInfo> first_name_el_;
  std::shared_ptr<ElInfo> last_name_el_;
  std::shared_ptr<ElInfo> title_el1_;
  std::shared_ptr<ElInfo> title_el2_;
};

}  // namespace tdcurses
