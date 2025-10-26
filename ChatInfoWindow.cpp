#include "common-windows/Action.hpp"
#include "common-windows/ErrorWindow.hpp"
#include "ChatInfoWindow.hpp"
#include "CommonGroupsWindow.hpp"
#include "ChatPhotoWindow.hpp"
#include "ChatPhotoListWindow.hpp"
#include "GroupMembersWindow.hpp"
#include "common-windows/FieldEditWindow.hpp"
#include "managers/GlobalParameters.hpp"
#include "common-windows/LoadingWindow.hpp"
#include "managers/StickerManager.hpp"
#include "managers/FileManager.hpp"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Promise.h"
#include "td/utils/Random.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/Status.h"
#include "td/utils/common.h"
#include "td/utils/overloaded.h"
#include "TdObjectsOutput.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tdcurses {

static std::shared_ptr<MenuWindow> spawn_rename_group_chat_window(ChatInfoWindow *window, std::shared_ptr<Chat> chat) {
  std::string prompt = "rename chat";
  std::string text = chat->title();

  return spawn_field_edit_window(
      *window, "rename chat", chat->title(), [chat_id = chat->chat_id(), self = window](td::Result<std::string> R) {
        if (R.is_error()) {
          return;
        }

        auto text = R.move_as_ok();

        auto req = td::make_tl_object<td::td_api::setChatTitle>(chat_id, text);
        loading_window_send_request(
            *self, "renaming chat", {}, std::move(req),
            [self, text = std::move(text)](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
              if (R.is_error()) {
                return;
              }
              self->updated_chat_title(text);
            });
      });
}

static std::shared_ptr<MenuWindow> spawn_rename_private_chat_window(ChatInfoWindow *window,
                                                                    std::shared_ptr<Chat> chat) {
  std::string prompt = "rename chat";
  std::string text = chat->title();

  return spawn_field_edit_window(
      *window, "rename chat", chat->title(),
      [chat_id = chat->chat_id(), user_id = chat->chat_base_id(), self = window](td::Result<std::string> R) {
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

        auto user = chat_manager().get_user(user_id);
        auto contact = td::make_tl_object<td::td_api::importedContact>(user ? user->phone_number() : "", first_name,
                                                                       last_name, nullptr);
        auto req = td::make_tl_object<td::td_api::addContact>(user_id, std::move(contact), false);

        loading_window_send_request(*self, "renaming chat", {}, std::move(req),
                                    [self, first_name = std::move(first_name), last_name = std::move(last_name)](
                                        td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
                                      if (R.is_error()) {
                                        return;
                                      }
                                      self->updated_user_name(std::move(first_name), std::move(last_name));
                                    });
      });
}

static std::shared_ptr<MenuWindow> spawn_rename_chat_window(ChatInfoWindow *window, std::shared_ptr<Chat> chat) {
  if (chat->chat_type() == ChatType::Basicgroup || chat->chat_type() == ChatType::Supergroup ||
      chat->chat_type() == ChatType::Channel) {
    return spawn_rename_group_chat_window(window, std::move(chat));
  }
  if (chat->chat_type() == ChatType::User || chat->chat_type() == ChatType::SecretChat) {
    return spawn_rename_private_chat_window(window, std::move(chat));
  }
  return nullptr;
}

static MenuWindowSpawnFunction spawn_rename_chat_window_func(std::shared_ptr<Chat> chat) {
  return [chat](MenuWindow &parent) -> std::shared_ptr<MenuWindow> {
    return spawn_rename_chat_window(static_cast<ChatInfoWindow *>(&parent), chat);
  };
}

void ChatInfoWindow::add_open_chat_option() {
  Outputter out;
  out << chat_->title() << Outputter::RightPad{"<open>"};
  title_el1_ = add_element("chat", out.as_str(), out.markup(), [root = root(), chat_id = chat_->chat_id()]() {
    root->open_chat(chat_id);
    return true;
  });
}

void ChatInfoWindow::add_rename_chat_option() {
  auto can_rename = chat_type_ == ChatType::User || chat_->permissions()->can_change_info_;
  if (can_rename) {
    Outputter out;
    out << chat_->title() << Outputter::RightPad{"<rename>"};
    title_el2_ = add_element("title", out.as_str(), out.markup(), spawn_rename_chat_window_func(chat_));
  } else {
    Outputter out;
    out << chat_->title();
    title_el2_ = add_element("title", out.as_str(), out.markup());
  }
}

void ChatInfoWindow::add_chat_id_option() {
  add_element("id", PSTRING() << chat_->chat_base_id() << " / " << chat_->chat_id());
}

void ChatInfoWindow::add_chat_type_option() {
  auto add_verification_status =
      [&, self = this](Outputter &out, const td::tl_object_ptr<td::td_api::verificationStatus> &verification_status) {
        if (!verification_status) {
          return;
        }
        if (verification_status->is_verified_) {
          out << " verified";
        }
        if (verification_status->is_scam_) {
          out << " scam";
        }
        if (verification_status->is_fake_) {
          out << " fake";
        }
        if (verification_status->bot_verification_icon_custom_emoji_id_) {
          auto S = sticker_manager().get_custom_emoji(verification_status->bot_verification_icon_custom_emoji_id_);
          if (S.size() > 0) {
            out << " " << S;
          } else {
            self->send_request(td::make_tl_object<td::td_api::getCustomEmojiStickers>(
                                   std::vector<td::int64>(verification_status->bot_verification_icon_custom_emoji_id_)),
                               [self](td::Result<td::tl_object_ptr<td::td_api::stickers>> R) {
                                 if (R.is_ok()) {
                                   sticker_manager().process_custom_emoji_stickers(*R.move_as_ok());
                                   self->set_need_refresh();
                                 }
                               });
            out << "?";
          }
        }
      };
  switch (chat_type_) {
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
          add_verification_status(out, user->verification_status());
          if (user->is_support()) {
            out << " support";
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
    } break;
    case ChatType::Basicgroup: {
      auto group = chat_manager().get_basic_group(chat_->chat_base_id());
      Outputter out;
      out << "group [";
      out << " ]";
      add_element("type", out.as_str());
    } break;
    case ChatType::Supergroup: {
      auto supergroup = chat_manager().get_supergroup(chat_->chat_base_id());
      Outputter out;
      out << "supergroup [";
      if (supergroup) {
        add_verification_status(out, supergroup->verification_status());
      }
      out << " ]";
      add_element("type", out.as_str(), out.markup());
    } break;
    case ChatType::Channel: {
      auto channel = chat_manager().get_channel(chat_->chat_base_id());
      Outputter out;
      out << "channel [";
      if (channel) {
        add_verification_status(out, channel->verification_status());
      }
      out << " ]";
      add_element("type", out.as_str(), out.markup());
    } break;
    case ChatType::SecretChat: {
      add_element("type", "secret_chat");
    } break;
    case ChatType::Unknown: {
    }; break;
  }
}

void ChatInfoWindow::add_user_name_option(td::CSlice first_name, td::CSlice last_name) {
  first_name_el_ = add_element("first name", first_name.str());
  last_name_el_ = add_element("last name", last_name.str());
}

void ChatInfoWindow::add_username_option(const td::tl_object_ptr<td::td_api::usernames> &usernames) {
  if (!usernames) {
    return;
  }
  Outputter out;
  bool is_first = true;
  for (const auto &x : usernames->active_usernames_) {
    if (!is_first) {
      out << " aka ";
    }
    out << "@" << x;
    is_first = false;
  }

  add_element("username", out.as_str());
}

void ChatInfoWindow::add_phone_number_option(td::CSlice phone_number) {
  if (phone_number.size() == 0) {
    return;
  }
  add_element("phone", PSTRING() << '+' << phone_number);
}

void ChatInfoWindow::add_bio_option(const td::tl_object_ptr<td::td_api::formattedText> &bio) {
  if (!bio) {
    return;
  }
  Outputter out;
  out << *bio;
  add_element("bio", out.as_str(), out.markup());
}

void ChatInfoWindow::add_user_status_option(const td::tl_object_ptr<td::td_api::UserStatus> &status) {
  if (!status) {
    return;
  }
  Outputter out;
  td::td_api::downcast_call(
      const_cast<td::td_api::UserStatus &>(*status),
      td::overloaded([&](const td::td_api::userStatusEmpty &status) { out << "unknown"; },
                     [&](const td::td_api::userStatusRecently &status) { out << "last seen recently"; },
                     [&](const td::td_api::userStatusOnline &status) { out << "online"; },
                     [&](const td::td_api::userStatusOffline &status) {
                       out << "last seen at " << Outputter::Date{status.was_online_};
                     },
                     [&](const td::td_api::userStatusLastWeek &status) { out << "last seen last week"; },
                     [&](const td::td_api::userStatusLastMonth &status) { out << "last seen last month"; }));
  add_element("status", out.as_str(), out.markup());
}

void ChatInfoWindow::add_bot_info_option(const td::tl_object_ptr<td::td_api::botInfo> &info) {
  if (!info) {
    return;
  }
  add_element("botinfo", info->description_);
}

void ChatInfoWindow::add_common_groups_option(td::int32 count) {
  auto user = chat_ ? chat_manager().get_user(chat_->chat_base_id()) : user_;
  if (!count) {
    Outputter out;
    out << count;
    add_element("commongroups", out.as_str(), out.markup());
  } else {
    CHECK(user);
    Outputter out;
    out << count << " " << Outputter::RightPad{"<view>"};
    add_element("commongroups", out.as_str(), out.markup(),
                create_menu_window_spawn_function<CommonGroupsWindow>(user));
  }
}

void ChatInfoWindow::add_chat_photo_option(const td::tl_object_ptr<td::td_api::chatPhoto> &photo) {
  if (!photo || !photo->sizes_.size()) {
    add_element("photo", "<empty>");
    return;
  }
  auto &photo_size = photo->sizes_.back();
  Outputter out;
  out << "photo" << Outputter::RightPad{"<view>"};
  add_element("photo", out.as_str(), out.markup(),
              [photo = photo_size->photo_.get(), height = photo_size->height_,
               width = photo_size->width_](MenuWindowCommon &w) -> bool {
                w.spawn_submenu<ChatPhotoWindow>(clone_td_file(*photo), height, width);
                return false;
              });
}

void ChatInfoWindow::add_user_photo_option(td::int64 user_id) {
  if (!user_id) {
    return;
  }
  Outputter out;
  out << "all user photos" << Outputter::RightPad{"<view>"};
  add_element("photos", out.as_str(), out.markup(), [user_id](MenuWindowCommon &w) -> bool {
    w.spawn_submenu<ChatPhotoListWindow>(user_id, 200, 200);
    return false;
  });
}

void ChatInfoWindow::add_personal_chat_option(td::int64 chat_id) {
  if (!chat_id) {
    return;
  }
  auto C = chat_manager().get_chat(chat_id);
  if (C) {
    Outputter out;
    out << C->title() << Outputter::RightPad{"<open>"};
    add_element("personalchat", out.as_str(), out.markup(), [root = root(), chat_id = C->chat_id()]() {
      root->open_chat(chat_id);
      return true;
    });
  }
}

void ChatInfoWindow::add_birthdate_option(const td::tl_object_ptr<td::td_api::birthdate> &birthdate) {
  if (!birthdate) {
    return;
  }
  if (!birthdate->month_) {
    return;
  }
  Outputter out;
  out << *birthdate;
  add_element("birthday", out.as_str(), out.markup());
}

void ChatInfoWindow::add_chat_member_status_option(const td::tl_object_ptr<td::td_api::ChatMemberStatus> &status) {
  Outputter out;
  std::function<bool(ChatInfoWindow &)> cb = [](ChatInfoWindow &) { return false; };
  auto leave_cb = [chat_id = chat_->chat_id(), self = this](ChatInfoWindow &w) {
    spawn_yes_no_window_and_loading_windows(
        *self, PSTRING() << "Are you sure, you want to leave chat '" << self->chat_->title() << "'?", {}, true,
        "leaving chat....", {}, td::make_tl_object<td::td_api::leaveChat>(chat_id),
        [self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
          DROP_IF_DELETED(R);
          if (R.is_error()) {
            spawn_error_window(*self, PSTRING() << "leaving chat failed: " << R.move_as_error(), {});
            return;
          }
        });
    return false;
  };
  auto join_cb = [chat_id = chat_->chat_id(), self = this](ChatInfoWindow &w) {
    spawn_yes_no_window_and_loading_windows(
        *self, PSTRING() << "Are you sure, you want to join chat '" << self->chat_->title() << "'?", {}, true,
        "joining chat....", {}, td::make_tl_object<td::td_api::joinChat>(chat_id),
        [self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
          DROP_IF_DELETED(R);
          if (R.is_error()) {
            spawn_error_window(*self, PSTRING() << "joining chat failed: " << R.move_as_error(), {});
            return;
          }
        });
    return false;
  };
  td::td_api::downcast_call(const_cast<td::td_api::ChatMemberStatus &>(*status),
                            td::overloaded(
                                [&](td::td_api::chatMemberStatusLeft &status) {
                                  out << "left" << Outputter::RightPad{"<join>"};
                                  cb = join_cb;
                                },
                                [&](td::td_api::chatMemberStatusBanned &status) {
                                  out << "banned until " << Outputter::Date{status.banned_until_date_}
                                      << Outputter::RightPad{"<leave>"};
                                  cb = leave_cb;
                                },
                                [&](td::td_api::chatMemberStatusMember &status) {
                                  out << "member" << Outputter::RightPad{"<leave>"};
                                  cb = leave_cb;
                                },
                                [&](td::td_api::chatMemberStatusCreator &status) {
                                  if (status.is_anonymous_) {
                                    out << "creator (anonymous)";
                                  } else {
                                    out << "creator (public)";
                                  }
                                },
                                [&](td::td_api::chatMemberStatusRestricted &status) {
                                  out << "restricted until " << Outputter::Date{status.restricted_until_date_}
                                      << Outputter::RightPad{"<leave>"};
                                  cb = leave_cb;
                                },
                                [&](td::td_api::chatMemberStatusAdministrator &status) {
                                  if (status.custom_title_.size() > 0) {
                                    out << "administrator (title " << status.custom_title_ << ")";
                                  } else {
                                    out << "administrator";
                                  }
                                }));
  add_element("status", out.as_str(), out.markup(),
              [cb = std::move(cb)](MenuWindowCommon &w) { return cb(static_cast<ChatInfoWindow &>(w)); });
}

void ChatInfoWindow::add_chat_members_option(td::int32 count, bool can_get_members) {
  if (can_get_members) {
    Outputter out;
    out << count << " " << Outputter::RightPad{"<view>"};
    add_element("members", out.as_str(), out.markup(), create_menu_window_spawn_function<GroupMembersWindow>(chat_));
  } else {
    Outputter out;
    out << count;
    add_element("members", out.as_str(), out.markup());
  }
}

void ChatInfoWindow::add_chat_description_option(td::CSlice desc) {
  if (desc.size() == 0) {
    return;
  }
  add_element("description", desc.str());
}

void ChatInfoWindow::add_chat_invite_link_option(const td::tl_object_ptr<td::td_api::chatInviteLink> &link) {
  if (!link) {
    return;
  }
  add_element("invitelink", link->invite_link_);
}

void ChatInfoWindow::add_chat_ttl_option(td::int32 ttl) {
  add_element("ttl", PSTRING() << chat_->message_ttl());
}

void ChatInfoWindow::add_chat_online_members_option(td::int32 count) {
  add_element("online", PSTRING() << count);
}

void ChatInfoWindow::generate_info() {
  clear();
  if (is_empty() || !chat_ || chat_->chat_type() == ChatType::Unknown) {
    return;
  }
  add_open_chat_option();
  add_rename_chat_option();
  add_chat_id_option();
  add_chat_type_option();

  switch (chat_type_) {
    case ChatType::User: {
      auto user = chat_ ? chat_manager().get_user(chat_->chat_base_id()) : user_;
      if (user) {
        add_user_name_option(user->first_name(), user->last_name());
        add_username_option(user->usernames());
        add_phone_number_option(user->phone_number());
        add_user_status_option(user->status());
      }
      if (user && user_full_) {
        add_bio_option(user_full_->bio_);
        add_bot_info_option(user_full_->bot_info_);
        add_common_groups_option(user_full_->group_in_common_count_);
        add_chat_photo_option(user_full_->photo_);
        add_user_photo_option(user->user_id());
        add_personal_chat_option(user_full_->personal_chat_id_);
        add_birthdate_option(user_full_->birthdate_);
      }
    } break;
    case ChatType::Basicgroup: {
      auto group = chat_manager().get_basic_group(chat_->chat_base_id());
      if (group) {
        add_chat_member_status_option(group->status());
        add_chat_members_option(group->member_count(), true);
      }
      if (group && basic_group_full_) {
        add_chat_description_option(basic_group_full_->description_);
        add_chat_photo_option(basic_group_full_->photo_);
        add_chat_invite_link_option(basic_group_full_->invite_link_);
      }
    } break;
    case ChatType::Supergroup: {
      auto supergroup = chat_manager().get_supergroup(chat_->chat_base_id());

      if (supergroup) {
        add_username_option(supergroup->usernames());
        add_chat_member_status_option(supergroup->status());
        add_chat_members_option(supergroup->member_count(), supergroup_full_ && supergroup_full_->can_get_members_);
      }
      if (supergroup && supergroup_full_) {
        add_chat_description_option(supergroup_full_->description_);
        add_chat_photo_option(supergroup_full_->photo_);
        add_chat_invite_link_option(supergroup_full_->invite_link_);
      }
    } break;
    case ChatType::Channel: {
      auto channel = chat_manager().get_channel(chat_->chat_base_id());

      if (channel) {
        add_chat_member_status_option(channel->status());
        add_chat_members_option(channel->member_count(), supergroup_full_ && supergroup_full_->can_get_members_);
      }
      if (channel && supergroup_full_) {
        add_chat_description_option(supergroup_full_->description_);
        add_chat_photo_option(supergroup_full_->photo_);
        add_chat_invite_link_option(supergroup_full_->invite_link_);
      }

    } break;
    case ChatType::SecretChat: {
    } break;
    case ChatType::Unknown: {
    }; break;
  }

  add_chat_ttl_option(chat_->message_ttl());
  add_chat_online_members_option(chat_->online_count());
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
