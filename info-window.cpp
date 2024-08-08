#include "info-window.h"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Random.h"
#include "td/utils/Status.h"
#include "td/utils/overloaded.h"
#include "telegram-curses-output.h"
#include <memory>
#include <vector>

namespace tdcurses {

void ChatInfoWindow::generate_info() {
  if (is_empty() || !chat_ || chat_->chat_type() == ChatType::Unknown) {
    replace_text("");
    return;
  }
  Outputter out;
  out << Outputter::NoLb(true) << "title          " << chat_->title() << Outputter::NoLb(false) << "\n";
  out << "id             " << chat_->chat_id() << "\n";
  auto chat_type = chat_->chat_type();
  switch (chat_type) {
    case ChatType::User: {
      out << "type           "
          << "user [";
      auto user = chat_ ? ChatManager::instance->get_user(chat_->chat_base_id()) : user_;
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
      out << " ]\n";
      if (user) {
        out << Outputter::NoLb(true) << "first name     " << user->first_name() << Outputter::NoLb(false) << "\n";
        out << Outputter::NoLb(true) << "last name      " << user->last_name() << Outputter::NoLb(false) << "\n";
        if (user->username().size()) {
          out << Outputter::NoLb(true) << "username       @" << user->username() << Outputter::NoLb(false) << "\n";
        }
        if (user->phone_number().size() > 0) {
          out << Outputter::NoLb(true) << "phone          +" << user->phone_number() << Outputter::NoLb(false) << "\n";
        }
        out << "user_id        " << user->user_id() << "\n";
        out << "status         ";

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
        out << "\n";
      }
      if (user_full_) {
        if (user_full_->bio_) {
          out << "bio            " << user_full_->bio_ << "\n";
        }
        if (user_full_->bot_info_) {
          out << "botinfo        " << user_full_->bot_info_->description_ << "\n";
        }
        out << "commongroups   " << user_full_->group_in_common_count_ << "\n";
        if (user_full_->photo_) {
          out << "photo          " << user_full_->photo_ << "\n";
        };
      }
    } break;
    case ChatType::Basicgroup: {
      out << "type           "
          << "group"
          << "\n";
      auto group = ChatManager::instance->get_basic_group(chat_->chat_base_id());
      if (group) {
        out << "group_id       " << group->group_id() << "\n";
        out << "status         ";
        const auto &status = group->status();
        td::td_api::downcast_call(const_cast<td::td_api::ChatMemberStatus &>(*status),
                                  td::overloaded([&](td::td_api::chatMemberStatusLeft &status) { out << "left"; },
                                                 [&](td::td_api::chatMemberStatusBanned &status) {
                                                   out << "banned until " << Outputter::Date{status.banned_until_date_};
                                                 },
                                                 [&](td::td_api::chatMemberStatusMember &status) { out << "member"; },
                                                 [&](td::td_api::chatMemberStatusCreator &status) {
                                                   out << "creator";
                                                   if (status.is_anonymous_) {
                                                     out << " (anonymous)";
                                                   }
                                                 },
                                                 [&](td::td_api::chatMemberStatusRestricted &status) {
                                                   out << "restricted until "
                                                       << Outputter::Date{status.restricted_until_date_};
                                                 },
                                                 [&](td::td_api::chatMemberStatusAdministrator &status) {
                                                   out << "administrator";
                                                   if (status.custom_title_.size() > 0) {
                                                     out << "(title " << status.custom_title_ << ")";
                                                   }
                                                 }));
        out << "\n";
        out << "members        " << group->member_count() << "\n";
      } else {
        out << "group_id       " << chat_->chat_base_id() << "\n";
      }
      if (basic_group_full_) {
        if (basic_group_full_->description_.size() > 0) {
          out << "description    " << basic_group_full_->description_ << "\n";
        }
        if (basic_group_full_->photo_) {
          out << "photo          " << basic_group_full_->photo_ << "\n";
        };
      }
    } break;
    case ChatType::Supergroup: {
      out << "type           "
          << "supergroup [";
      auto supergroup = ChatManager::instance->get_supergroup(chat_->chat_base_id());
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
      }
      out << " ]\n";

      if (supergroup) {
        const auto &status = supergroup->status();
        if (status) {
          out << "status         ";
          td::td_api::downcast_call(
              const_cast<td::td_api::ChatMemberStatus &>(*status),
              td::overloaded([&](td::td_api::chatMemberStatusLeft &status) { out << "left"; },
                             [&](td::td_api::chatMemberStatusBanned &status) {
                               out << "banned until " << Outputter::Date{status.banned_until_date_};
                             },
                             [&](td::td_api::chatMemberStatusMember &status) { out << "member"; },
                             [&](td::td_api::chatMemberStatusCreator &status) {
                               out << "creator";
                               if (status.is_anonymous_) {
                                 out << " (anonymous)";
                               }
                             },
                             [&](td::td_api::chatMemberStatusRestricted &status) {
                               out << "restricted until " << Outputter::Date{status.restricted_until_date_};
                             },
                             [&](td::td_api::chatMemberStatusAdministrator &status) {
                               out << "administrator";
                               if (status.custom_title_.size() > 0) {
                                 out << "(title " << status.custom_title_ << ")";
                               }
                             }));
          out << "\n";
        }
        out << "members        " << supergroup->member_count() << "\n";
      }
      if (supergroup_full_) {
        if (supergroup_full_->description_.size() > 0) {
          out << "description    " << supergroup_full_->description_ << "\n";
        }
        if (supergroup_full_->photo_) {
          out << "photo          " << supergroup_full_->photo_ << "\n";
        }
        if (supergroup_full_->invite_link_) {
          out << "invite_link    " << supergroup_full_->invite_link_->name_ << "\n";
        }
      }
    } break;
    case ChatType::Channel: {
      out << "type           "
          << "channel [";
      auto channel = ChatManager::instance->get_channel(chat_->chat_base_id());
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
      }
      out << " ]\n";

      if (channel) {
        const auto &status = channel->status();
        if (status) {
          out << "status         ";
          td::td_api::downcast_call(
              const_cast<td::td_api::ChatMemberStatus &>(*status),
              td::overloaded([&](td::td_api::chatMemberStatusLeft &status) { out << "left"; },
                             [&](td::td_api::chatMemberStatusBanned &status) {
                               out << "banned until " << Outputter::Date{status.banned_until_date_};
                             },
                             [&](td::td_api::chatMemberStatusMember &status) { out << "member"; },
                             [&](td::td_api::chatMemberStatusCreator &status) {
                               out << "creator";
                               if (status.is_anonymous_) {
                                 out << " (anonymous)";
                               }
                             },
                             [&](td::td_api::chatMemberStatusRestricted &status) {
                               out << "restricted until " << Outputter::Date{status.restricted_until_date_};
                             },
                             [&](td::td_api::chatMemberStatusAdministrator &status) {
                               out << "administrator";
                               if (status.custom_title_.size() > 0) {
                                 out << "(title " << status.custom_title_ << ")";
                               }
                             }));
          out << "\n";
        }
        out << "members        " << channel->member_count() << "\n";
      }
      if (supergroup_full_) {
        if (supergroup_full_->description_.size() > 0) {
          out << "description    " << supergroup_full_->description_ << "\n";
        }
        if (supergroup_full_->photo_) {
          out << "photo          " << supergroup_full_->photo_ << "\n";
        }
        if (supergroup_full_->invite_link_) {
          out << "invite_link    " << supergroup_full_->invite_link_->name_ << "\n";
        }
      }
    } break;
    case ChatType::SecretChat: {
      out << "type           "
          << "secret chat"
          << "\n";
    } break;
    case ChatType::Unknown: {
    }; break;
  }

  if (chat_->message_ttl() > 0) {
    out << "ttl            " << chat_->message_ttl() << "\n";
  }
  out << "online         " << chat_->online_count() << "\n";
  if (chat_->theme_name().size() > 0) {
    out << "theme          " << chat_->theme_name() << "\n";
  }
  replace_text(out.as_str(), out.markup());
  set_need_refresh();
}

void ChatInfoWindow::clear() {
  change_unique_id();
  chat_ = nullptr;
  user_full_ = nullptr;
  basic_group_full_ = nullptr;
  supergroup_full_ = nullptr;
  chat_type_ = ChatType::User;
  chat_inner_id_ = 0;
  generate_info();
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
    chat_type_ = chat_->chat_type();
    chat_inner_id_ = chat_->chat_base_id();
    switch (chat_->chat_type()) {
      case ChatType::User: {
        send_request(
            td::make_tl_object<td::td_api::getUserFullInfo>(chat_->chat_base_id()),
            [chat_id = chat_->chat_id(), window = this](td::Result<td::tl_object_ptr<td::td_api::userFullInfo>> R) {
              if (R.is_error()) {
                LOG(ERROR) << "failed to get userFull: received error: " << R.move_as_error();
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
                         LOG(ERROR) << "failed to get basicGroupFull: received error: " << R.move_as_error();
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
                         LOG(ERROR) << "failed to get basicGroupFull: received error: " << R.move_as_error();
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
                         LOG(ERROR) << "failed to get basicGroupFull: received error: " << R.move_as_error();
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
                   LOG(ERROR) << "failed to start chat: received error: " << R.move_as_error();
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
  auto chat = ChatManager::instance->get_chat(pchat->id_);
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

}  // namespace tdcurses
