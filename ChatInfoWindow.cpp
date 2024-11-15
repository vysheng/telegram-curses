#include "ChatInfoWindow.hpp"
#include "CommonGroupsWindow.hpp"
#include "GroupMembersWindow.hpp"
#include "FieldEditWindow.hpp"
#include "GlobalParameters.hpp"
#include "LoadingWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Promise.h"
#include "td/utils/Random.h"
#include "td/utils/Status.h"
#include "td/utils/common.h"
#include "td/utils/overloaded.h"
#include "TdObjectsOutput.h"
#include "FileManager.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tdcurses {

static std::shared_ptr<MenuWindow> spawn_rename_chat_window(ChatInfoWindow *window, std::shared_ptr<Chat> chat) {
  std::string prompt = "rename chat";
  std::string text = chat->title();
  // text

  if (chat->chat_type() == ChatType::User) {
    auto cb = [user_id = chat->chat_base_id(), root = window->root(), self = window,
               self_id = window->window_unique_id()](FieldEditWindow &w, td::Result<std::string> R) {
      if (R.is_error()) {
        return;
      }
      auto text = R.move_as_ok();
      auto p = text.find(" ");
      std::string first_name, last_name;
      if (p == std::string::npos) {
        first_name = text;
        last_name = "";
      } else {
        first_name = text.substr(0, p);
        last_name = text.substr(p + 1);
      }
      auto promise = td::PromiseCreator::lambda([root, self, self_id, first_name, last_name](td::Result<td::Unit> R) {
        if (R.is_error()) {
          return;
        }
        if (!root->window_exists(self_id)) {
          return;
        }
        self->updated_user_name(first_name, last_name);
      });
      auto x = w.rollback();
      auto user = chat_manager().get_user(user_id);
      bool is_self = user && (user->user_id() == global_parameters().my_user_id());
      if (!is_self) {
        auto contact = td::make_tl_object<td::td_api::contact>(user ? user->phone_number() : "", first_name, last_name,
                                                               "", user_id);
        auto req = td::make_tl_object<td::td_api::addContact>(std::move(contact), false);
        loading_window_send_request(
            *x, "renaming user", {}, std::move(req),
            [promise = std::move(promise)](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
              if (R.is_error()) {
                promise.set_error(R.move_as_error());
              } else {
                promise.set_value(td::Unit());
              }
            });
      } else {
        auto req = td::make_tl_object<td::td_api::setName>(first_name, last_name);
        loading_window_send_request(
            *x, "renaming user", {}, std::move(req),
            [promise = std::move(promise)](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
              if (R.is_error()) {
                promise.set_error(R.move_as_error());
              } else {
                promise.set_value(td::Unit());
              }
            });
      }
    };
    return window->spawn_submenu<FieldEditWindow>(prompt, text, FieldEditWindow::make_callback(std::move(cb)));
  } else {
    auto cb = [chat_id = chat->chat_id(), root = window->root(), self = window, self_id = window->window_unique_id()](
                  FieldEditWindow &w, td::Result<std::string> R) mutable {
      if (R.is_error()) {
        return;
      }

      auto x = w.rollback();
      auto text = R.move_as_ok();

      auto promise = td::PromiseCreator::lambda([root, self, self_id, text](td::Result<td::Unit> R) {
        if (R.is_error()) {
          return;
        }
        if (!root->window_exists(self_id)) {
          return;
        }
        self->updated_chat_title(text);
      });

      auto req = td::make_tl_object<td::td_api::setChatTitle>(chat_id, text);
      loading_window_send_request(
          *x, "renaming chat", {}, std::move(req),
          [promise = std::move(promise)](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
            if (R.is_error()) {
              promise.set_error(R.move_as_error());
            } else {
              promise.set_value(td::Unit());
            }
          });
    };
    return window->spawn_submenu<FieldEditWindow>(prompt, text, FieldEditWindow::make_callback(std::move(cb)));
  }
}

static MenuWindowSpawnFunction spawn_rename_chat_window_func(std::shared_ptr<Chat> chat) {
  return [chat](MenuWindow &parent) -> std::shared_ptr<MenuWindow> {
    return spawn_rename_chat_window(static_cast<ChatInfoWindow *>(&parent), chat);
  };
}

void ChatInfoWindow::generate_info() {
  clear();
  if (is_empty() || !chat_ || chat_->chat_type() == ChatType::Unknown) {
    return;
  }
  {
    Outputter out;
    out << chat_->title() << Outputter::RightPad{"<open>"};
    title_el1_ = add_element("chat", out.as_str(), out.markup(), [root = root(), chat_id = chat_->chat_id()]() {
      root->open_chat(chat_id);
      return true;
    });
  }
  {
    Outputter out;
    out << chat_->title() << Outputter::RightPad{"<rename>"};
    title_el2_ = add_element("title", out.as_str(), out.markup(), spawn_rename_chat_window_func(chat_));
  }
  add_element("id", PSTRING() << chat_->chat_id());
  auto chat_type = chat_->chat_type();
  switch (chat_type) {
    case ChatType::User: {
      auto user = chat_ ? chat_manager().get_user(chat_->chat_base_id()) : user_;
      bool is_self = user && (user->user_id() == global_parameters().my_user_id());
      {
        Outputter out;
        if (!is_self) {
          out << "user [";
        } else {
          out << "self [";
        }
        if (user) {
          if (user->is_verified()) {
            out << " verified";
          }
          if (user->is_scam()) {
            out << " scam";
          }
          if (user->is_support()) {
            out << " support";
          }
          if (user->is_fake()) {
            out << " fake";
          }
          if (user->is_mutual_contact()) {
            out << " mutual_contact";
          } else if (user->is_contact()) {
            out << " contact";
          } else {
            out << " notcontact";
          }
          if (user_full_ && user_full_->block_list_) {
            out << " blocked";
          }
        }
        out << " ]";
        add_element("type", out.as_str(), out.markup());
      }
      if (user) {
        first_name_el_ = add_element("first name", user->first_name());
        last_name_el_ = add_element("last name", user->last_name());
        if (user->username().size()) {
          add_element("username", "@" + user->username());
        }
        if (user->phone_number().size() > 0) {
          add_element("phone", "+" + user->phone_number());
        }
        add_element("user_id", PSTRING() << user->user_id());

        {
          Outputter out;
          const auto &status = user->status();
          td::td_api::downcast_call(
              const_cast<td::td_api::UserStatus &>(*status),
              td::overloaded([&](td::td_api::userStatusEmpty &status) { out << "unknown"; },
                             [&](td::td_api::userStatusRecently &status) { out << "last seen recently"; },
                             [&](td::td_api::userStatusOnline &status) { out << "online"; },
                             [&](td::td_api::userStatusOffline &status) {
                               out << "last seen at " << Outputter::Date{status.was_online_};
                             },
                             [&](td::td_api::userStatusLastWeek &status) { out << "last seen last week"; },
                             [&](td::td_api::userStatusLastMonth &status) { out << "last seen last month"; }));
          add_element("status", out.as_str(), out.markup());
        }
      }
      if (user_full_) {
        if (user_full_->bio_) {
          Outputter out;
          out << *user_full_->bio_;
          add_element("bio", out.as_str(), out.markup());
        }
        if (user_full_->bot_info_) {
          add_element("botinfo", user_full_->bot_info_->description_);
        }
        {
          Outputter out;
          out << user_full_->group_in_common_count_ << " " << Outputter::RightPad{"<view>"};
          add_element("commongroups", out.as_str(), out.markup(),
                      create_menu_window_spawn_function<CommonGroupsWindow>(user));
        }
        if (user_full_->photo_) {
          Outputter out;
          out << user_full_->photo_;
          add_element("photo", out.as_str(), out.markup());
        }
        if (user_full_->personal_chat_id_) {
          auto C = chat_manager().get_chat(user_full_->personal_chat_id_);
          if (C) {
            Outputter out;
            out << C->title() << Outputter::RightPad{"<open>"};
            add_element("personalchat", out.as_str(), out.markup(), [root = root(), chat_id = C->chat_id()]() {
              root->open_chat(chat_id);
              return true;
            });
          }
        }
        if (user_full_->birthdate_) {
          auto &b = *user_full_->birthdate_;
          if (b.year_ || b.month_) {
            Outputter out;
            out << b;
            add_element("birthday", out.as_str(), out.markup());
          }
        }
      }
    } break;
    case ChatType::Basicgroup: {
      auto group = chat_manager().get_basic_group(chat_->chat_base_id());
      add_element("type", "group");
      if (group) {
        add_element("group_id", PSTRING() << group->group_id());
        {
          Outputter out;
          const auto &status = group->status();
          td::td_api::downcast_call(
              const_cast<td::td_api::ChatMemberStatus &>(*status),
              td::overloaded([&](td::td_api::chatMemberStatusLeft &status) { out << "left"; },
                             [&](td::td_api::chatMemberStatusBanned &status) {
                               out << "banned until " << Outputter::Date{status.banned_until_date_};
                             },
                             [&](td::td_api::chatMemberStatusMember &status) { out << "member"; },
                             [&](td::td_api::chatMemberStatusCreator &status) {
                               if (status.is_anonymous_) {
                                 out << "creator (anonymous)";
                               } else {
                                 out << "creator (public)";
                               }
                             },
                             [&](td::td_api::chatMemberStatusRestricted &status) {
                               out << "restricted until " << Outputter::Date{status.restricted_until_date_};
                             },
                             [&](td::td_api::chatMemberStatusAdministrator &status) {
                               if (status.custom_title_.size() > 0) {
                                 out << "administrator (title " << status.custom_title_ << ")";
                               } else {
                                 out << "administrator";
                               }
                             }));
          add_element("status", out.as_str(), out.markup());
        }
        {
          Outputter out;
          out << group->member_count() << " " << Outputter::RightPad{"<view>"};
          add_element("members", out.as_str(), out.markup(),
                      create_menu_window_spawn_function<GroupMembersWindow>(chat_));
        }
      }
      if (basic_group_full_) {
        if (basic_group_full_->description_.size() > 0) {
          add_element("description", basic_group_full_->description_);
        }
        if (basic_group_full_->photo_) {
          Outputter out;
          out << basic_group_full_->photo_;
          add_element("photo", out.as_str(), out.markup());
        };
      }
    } break;
    case ChatType::Supergroup: {
      auto supergroup = chat_manager().get_supergroup(chat_->chat_base_id());
      {
        Outputter out;
        out << "supergroup [";
        if (supergroup) {
          if (supergroup->is_verified()) {
            out << " verified";
          }
          if (supergroup->is_scam()) {
            out << " scam";
          }
          if (supergroup->is_fake()) {
            out << " fake";
          }
          if (supergroup->has_sensitive_content()) {
            out << " 18+";
          }
        }
        out << " ]";
        add_element("type", out.as_str(), out.markup());
      }

      if (supergroup) {
        {
          Outputter out;
          const auto &status = supergroup->status();
          td::td_api::downcast_call(
              const_cast<td::td_api::ChatMemberStatus &>(*status),
              td::overloaded([&](td::td_api::chatMemberStatusLeft &status) { out << "left"; },
                             [&](td::td_api::chatMemberStatusBanned &status) {
                               out << "banned until " << Outputter::Date{status.banned_until_date_};
                             },
                             [&](td::td_api::chatMemberStatusMember &status) { out << "member"; },
                             [&](td::td_api::chatMemberStatusCreator &status) {
                               if (status.is_anonymous_) {
                                 out << "creator (anonymous)";
                               } else {
                                 out << "creator (public)";
                               }
                             },
                             [&](td::td_api::chatMemberStatusRestricted &status) {
                               out << "restricted until " << Outputter::Date{status.restricted_until_date_};
                             },
                             [&](td::td_api::chatMemberStatusAdministrator &status) {
                               if (status.custom_title_.size() > 0) {
                                 out << "administrator (title " << status.custom_title_ << ")";
                               } else {
                                 out << "administrator";
                               }
                             }));
          add_element("status", out.as_str(), out.markup());
        }
        {
          Outputter out;
          out << supergroup->member_count() << " " << Outputter::RightPad{"<view>"};
          add_element("members", out.as_str(), out.markup(),
                      create_menu_window_spawn_function<GroupMembersWindow>(chat_));
        }
      }
      if (supergroup_full_) {
        if (supergroup_full_->description_.size() > 0) {
          add_element("description", supergroup_full_->description_);
        }
        if (supergroup_full_->photo_) {
          Outputter out;
          out << supergroup_full_->photo_;
          add_element("photo", out.as_str(), out.markup());
        }
        if (supergroup_full_->invite_link_) {
          add_element("invite_link", supergroup_full_->invite_link_->name_);
        }
      }
    } break;
    case ChatType::Channel: {
      auto channel = chat_manager().get_channel(chat_->chat_base_id());
      {
        Outputter out;
        out << "channel [";
        if (channel) {
          if (channel->is_verified()) {
            out << " verified";
          }
          if (channel->is_scam()) {
            out << " scam";
          }
          if (channel->is_fake()) {
            out << " fake";
          }
          if (channel->has_sensitive_content()) {
            out << " 18+";
          }
        }
        out << " ]";
        add_element("type", out.as_str(), out.markup());
      }

      if (channel) {
        {
          Outputter out;
          const auto &status = channel->status();
          td::td_api::downcast_call(
              const_cast<td::td_api::ChatMemberStatus &>(*status),
              td::overloaded([&](td::td_api::chatMemberStatusLeft &status) { out << "left"; },
                             [&](td::td_api::chatMemberStatusBanned &status) {
                               out << "banned until " << Outputter::Date{status.banned_until_date_};
                             },
                             [&](td::td_api::chatMemberStatusMember &status) { out << "member"; },
                             [&](td::td_api::chatMemberStatusCreator &status) {
                               if (status.is_anonymous_) {
                                 out << "creator (anonymous)";
                               } else {
                                 out << "creator (public)";
                               }
                             },
                             [&](td::td_api::chatMemberStatusRestricted &status) {
                               out << "restricted until " << Outputter::Date{status.restricted_until_date_};
                             },
                             [&](td::td_api::chatMemberStatusAdministrator &status) {
                               if (status.custom_title_.size() > 0) {
                                 out << "administrator (title " << status.custom_title_ << ")";
                               } else {
                                 out << "administrator";
                               }
                             }));
          add_element("status", out.as_str(), out.markup());
        }
        {
          Outputter out;
          out << channel->member_count() << " " << Outputter::RightPad{"<view>"};
          add_element("members", out.as_str(), out.markup(),
                      create_menu_window_spawn_function<GroupMembersWindow>(chat_));
        }
      }
      if (supergroup_full_) {
        if (supergroup_full_->description_.size() > 0) {
          add_element("description", supergroup_full_->description_);
        }
        if (supergroup_full_->photo_) {
          Outputter out;
          out << supergroup_full_->photo_;
          add_element("photo", out.as_str(), out.markup());
        }
        if (supergroup_full_->invite_link_) {
          add_element("invite_link", supergroup_full_->invite_link_->name_);
        }
      }

    } break;
    case ChatType::SecretChat: {
      add_element("type", "secret_chat");
    } break;
    case ChatType::Unknown: {
    }; break;
  }

  if (chat_->message_ttl() > 0) {
    add_element("ttl", PSTRING() << chat_->message_ttl());
  }
  add_element("online", PSTRING() << chat_->online_count());
  set_need_refresh();
}

void ChatInfoWindow::set_chat(std::shared_ptr<Chat> chat) {
  if (chat && chat_ && chat->chat_type() == chat_type_ && chat->chat_base_id() == chat_inner_id_) {
    return;
  }
  if (!chat && is_empty() && chat_inner_id_ == 0) {
    return;
  }

  clear();
  chat_ = std::move(chat);

  if (chat_) {
    set_title(PSTRING() << "info " << chat_->title());
    chat_type_ = chat_->chat_type();
    chat_inner_id_ = chat_->chat_base_id();
    switch (chat_->chat_type()) {
      case ChatType::User: {
        send_request(
            td::make_tl_object<td::td_api::getUserFullInfo>(chat_->chat_base_id()),
            [chat_id = chat_->chat_id(), window = this](td::Result<td::tl_object_ptr<td::td_api::userFullInfo>> R) {
              if (R.is_error()) {
                if (R.error().code() != ErrorCodeWindowDeleted) {
                  LOG(ERROR) << "failed to get userFull: received error: " << R.move_as_error();
                }
                return;
              }
              window->got_full_user(chat_id, R.move_as_ok());
            });
      } break;
      case ChatType::Basicgroup: {
        send_request(td::make_tl_object<td::td_api::getBasicGroupFullInfo>(chat_->chat_base_id()),
                     [chat_id = chat_->chat_id(),
                      window = this](td::Result<td::tl_object_ptr<td::td_api::basicGroupFullInfo>> R) {
                       if (R.is_error()) {
                         if (R.error().code() != ErrorCodeWindowDeleted) {
                           LOG(ERROR) << "failed to get basicGroupFull: received error: " << R.move_as_error();
                         }
                         return;
                       }
                       window->got_full_basic_group(chat_id, R.move_as_ok());
                     });
      } break;
      case ChatType::Supergroup:
      case ChatType::Channel: {
        send_request(td::make_tl_object<td::td_api::getSupergroupFullInfo>(chat_->chat_base_id()),
                     [chat_id = chat_->chat_id(),
                      window = this](td::Result<td::tl_object_ptr<td::td_api::supergroupFullInfo>> R) {
                       if (R.is_error()) {
                         if (R.error().code() != ErrorCodeWindowDeleted) {
                           LOG(ERROR) << "failed to get basicGroupFull: received error: " << R.move_as_error();
                         }
                         return;
                       }
                       window->got_full_super_group(chat_id, R.move_as_ok());
                     });
      } break;
      case ChatType::SecretChat:
        break;
      case ChatType::Unknown: {
        send_request(td::make_tl_object<td::td_api::getChat>(chat_->chat_base_id()),
                     [chat_id = chat_->chat_id(), window = this](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
                       if (R.is_error()) {
                         if (R.error().code() != ErrorCodeWindowDeleted) {
                           LOG(ERROR) << "failed to get basicGroupFull: received error: " << R.move_as_error();
                         }
                         return;
                       }
                       window->got_chat_info(chat_id, R.move_as_ok());
                     });
      } break;
    }
  } else {
    chat_type_ = ChatType::Unknown;
    chat_inner_id_ = 0;
  }
  generate_info();
}

void ChatInfoWindow::set_chat(std::shared_ptr<User> user) {
  if (!user) {
    return set_chat(std::shared_ptr<Chat>());
  }
  if (chat_type_ == ChatType::User && chat_inner_id_ == user->user_id()) {
    return;
  }
  clear();
  chat_type_ = ChatType::User;
  chat_inner_id_ = user->user_id();
  send_request(td::make_tl_object<td::td_api::createPrivateChat>(chat_inner_id_, true),
               [window = this](td::Result<td::tl_object_ptr<td::td_api::chat>> R) {
                 if (R.is_error()) {
                   if (R.error().code() != ErrorCodeWindowDeleted) {
                     LOG(ERROR) << "failed to start chat: received error: " << R.move_as_error();
                   }
                   return;
                 }
                 window->got_chat(R.move_as_ok());
               });
}
void ChatInfoWindow::set_chat(std::shared_ptr<BasicGroup> chat) {
  if (!chat) {
    return set_chat(std::shared_ptr<Chat>());
  }
}
void ChatInfoWindow::set_chat(std::shared_ptr<Supergroup> chat) {
  if (!chat) {
    return set_chat(std::shared_ptr<Chat>());
  }
}
void ChatInfoWindow::set_chat(std::shared_ptr<SecretChat> chat) {
  if (!chat) {
    return set_chat(std::shared_ptr<Chat>());
  }
}

void ChatInfoWindow::got_chat(td::tl_object_ptr<td::td_api::chat> pchat) {
  auto chat = chat_manager().get_chat(pchat->id_);
  if (!chat) {
    return;
  }
  set_chat(chat);
}

void ChatInfoWindow::got_full_user(td::int64 chat_id, td::tl_object_ptr<td::td_api::userFullInfo> user) {
  if (!chat_ || chat_->chat_id() != chat_id) {
    return;
  }
  user_full_ = std::move(user);
  generate_info();
}

void ChatInfoWindow::got_full_basic_group(td::int64 chat_id, td::tl_object_ptr<td::td_api::basicGroupFullInfo> group) {
  if (!chat_ || chat_->chat_id() != chat_id) {
    return;
  }
  basic_group_full_ = std::move(group);
  generate_info();
}

void ChatInfoWindow::got_full_super_group(td::int64 chat_id, td::tl_object_ptr<td::td_api::supergroupFullInfo> group) {
  if (!chat_ || chat_->chat_id() != chat_id) {
    return;
  }
  supergroup_full_ = std::move(group);
  generate_info();
}

void ChatInfoWindow::got_chat_info(td::int64 chat_id, td::tl_object_ptr<td::td_api::chat> chat) {
  if (!chat_ || chat_->chat_id() != chat_id) {
    return;
  }
  chat_->full_update(std::move(chat));
  set_chat(chat_);
}

void ChatInfoWindow::updated_chat_title(std::string new_title) {
  if (title_el1_) {
    change_element(title_el1_, [&]() { title_el1_->menu_element()->data = new_title; });
  }
  if (title_el2_) {
    change_element(title_el2_, [&]() { title_el2_->menu_element()->data = new_title; });
  }
  set_need_refresh();
}

void ChatInfoWindow::updated_user_name(std::string first_name, std::string last_name) {
  if (first_name_el_) {
    change_element(first_name_el_, [&]() { first_name_el_->menu_element()->data = first_name; });
  }
  if (last_name_el_) {
    change_element(last_name_el_, [&]() { last_name_el_->menu_element()->data = last_name; });
  }
  if (title_el1_) {
    change_element(title_el1_, [&]() { title_el1_->menu_element()->data = first_name + " " + last_name; });
  }
  if (title_el2_) {
    change_element(title_el2_, [&]() { title_el2_->menu_element()->data = first_name + " " + last_name; });
  }
  set_need_refresh();
}

}  // namespace tdcurses
