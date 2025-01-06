#include "SettingsWindow.hpp"
#include "ChatManager.hpp"
#include "GlobalParameters.hpp"
#include "FileSelectionWindow.hpp"
#include "LoadingWindow.hpp"
#include "td/telegram/files/ResourceState.h"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "TdObjectsOutput.h"
#include "ChatSearchWindow.hpp"
#include "FieldEditWindow.hpp"
#include "FileSelectionWindow.hpp"
#include "ErrorWindow.hpp"
#include "td/utils/Slice-decl.h"
#include "td/utils/Status.h"
#include "td/utils/misc.h"
#include "Action.hpp"
#include "SelectionWindow.hpp"
#include <limits>
#include <memory>

namespace tdcurses {

td::tl_object_ptr<td::td_api::scopeNotificationSettings> ScopeNotificationsSettingsWindow::settings_tl() {
  return td::make_tl_object<td::td_api::scopeNotificationSettings>(
      settings_->mute_for_, settings_->sound_id_, settings_->show_preview_, settings_->use_default_mute_stories_,
      settings_->mute_stories_, settings_->story_sound_id_, settings_->show_story_sender_,
      settings_->disable_pinned_message_notifications_, settings_->disable_mention_notifications_);
}

td::tl_object_ptr<td::td_api::NotificationSettingsScope> ScopeNotificationsSettingsWindow::scope_tl() {
  if (scope_ == NotificationScope::NotificationScopePrivateChats) {
    return td::make_tl_object<td::td_api::notificationSettingsScopePrivateChats>();
  } else if (scope_ == NotificationScope::NotificationScopeGroupChats) {
    return td::make_tl_object<td::td_api::notificationSettingsScopeGroupChats>();
  } else if (scope_ == NotificationScope::NotificationScopeChannelChats) {
    return td::make_tl_object<td::td_api::notificationSettingsScopeChannelChats>();
  } else {
    UNREACHABLE();
  }
}

void ScopeNotificationsSettingsWindow::send_requests() {
  send_request(td::make_tl_object<td::td_api::getScopeNotificationSettings>(scope_tl()),
               [window = this](td::Result<td::tl_object_ptr<td::td_api::scopeNotificationSettings>> R) {
                 if (R.is_error()) {
                   if (R.error().code() != ErrorCodeWindowDeleted) {
                     R.ensure();
                   }
                   return;
                 }
                 window->got_notification_settings(R.move_as_ok());
               });
  send_request(td::make_tl_object<td::td_api::getChatNotificationSettingsExceptions>(scope_tl(), true),
               [window = this](td::Result<td::tl_object_ptr<td::td_api::chats>> R) {
                 if (R.is_error()) {
                   if (R.error().code() != ErrorCodeWindowDeleted) {
                     R.ensure();
                   }
                   return;
                 }
                 window->got_exceptions(R.move_as_ok());
               });
}

void ScopeNotificationsSettingsWindow::build_menu() {
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_mute_for_) {
        ex++;
      }
    }
    Outputter out;
    if (settings_->mute_for_) {
      out << "mute for " << settings_->mute_for_;
    } else {
      out << "unmuted";
    }
    out << " (" << ex << " exceptions)" << Outputter::RightPad{"<change>"};
    add_element("mute", out.as_str(), out.markup(), [](MenuWindowCommon &w) -> bool {
      spawn_selection_window(
          w, "change mute for", {"unmute", "mute forever", "30 minutes", "1 hour", "8 hours"},
          [&self = static_cast<ScopeNotificationsSettingsWindow &>(w)](td::Result<size_t> R) {
            if (R.is_error()) {
              return;
            }
            auto tl = self.settings_tl();
            auto scope = self.scope_tl();
            switch (R.move_as_ok()) {
              case 0:
                tl->mute_for_ = 0;
                break;
              case 1:
                tl->mute_for_ = std::numeric_limits<td::int32>::max();
                break;
              case 2:
                tl->mute_for_ = 30 * 60;
                break;
              case 3:
                tl->mute_for_ = 60 * 60;
                break;
              case 4:
                tl->mute_for_ = 8 * 60 * 60;
                break;
            }
            loading_window_send_request(
                self, "updating notification settings", {},
                td::make_tl_object<td::td_api::setScopeNotificationSettings>(std::move(scope), std::move(tl)),
                [&self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                  if (R.is_error()) {
                    if (R.error().code() != ErrorCodeWindowDeleted) {
                      spawn_error_window(
                          self, PSTRING() << "failed to update notification settings " << R.move_as_error(), {});
                    }
                  }
                });
          });
      return false;
    });
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_sound_) {
        ex++;
      }
    }
    Outputter out;
    out << settings_->sound_id_;
    out << " (" << ex << " exceptions)";
    add_element("sound_id", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_show_preview_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->show_preview_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("show preview", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_mute_stories_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->use_default_mute_stories_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("default mute stories", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_mute_stories_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->mute_stories_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("mute stories", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_story_sound_) {
        ex++;
      }
    }
    Outputter out;
    out << settings_->story_sound_id_;
    out << " (" << ex << " exceptions)";
    add_element("stories sound", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_show_story_sender_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->show_story_sender_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("show story sender", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_disable_pinned_message_notifications_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->disable_pinned_message_notifications_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("disable pin notification", out.as_str(), out.markup());
  }
  {
    td::int32 ex = 0;
    for (auto &c : exceptions_->chat_ids_) {
      auto C = chat_manager().get_chat(c);
      if (C && !C->notification_settins()->use_default_disable_mention_notifications_) {
        ex++;
      }
    }
    Outputter out;
    out << (settings_->disable_mention_notifications_ ? "YES" : "NO");
    out << " (" << ex << " exceptions)";
    add_element("disable mention notification", out.as_str(), out.markup());
  }
}

void NotificationsSettingsWindow::build_menu() {
  {
    bool notifications_enabled = global_parameters().notifications_enabled();
    Outputter out;
    if (notifications_enabled) {
      out << "enabled" << Outputter::RightPad{"<disable>"};
    } else {
      out << "disabled" << Outputter::RightPad{"<enable>"};
    }
    enabled_el_ = add_element("desktop notifications", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
      bool enabled = !global_parameters().notifications_enabled();
      loading_window_send_request(
          w, "changing settings", {},
          td::make_tl_object<td::td_api::setOption>("X-notifications-enabled",
                                                    td::make_tl_object<td::td_api::optionValueBoolean>(enabled)),
          [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });

      global_parameters().set_notifications_enabled(enabled);
      Outputter out;
      if (enabled) {
        out << "enabled" << Outputter::RightPad{"<disable>"};
      } else {
        out << "disabled" << Outputter::RightPad{"<enable>"};
      }
      static_cast<NotificationsSettingsWindow &>(w).enabled_el_->menu_element()->data = out.as_str();
      static_cast<NotificationsSettingsWindow &>(w).enabled_el_->menu_element()->markup = out.markup();
      return false;
    });
  }
  {
    Outputter out;
    out << Outputter::RightPad{"<modify>"};
    add_element("private chats settings", out.as_str(), out.markup(),
                create_menu_window_spawn_function<ScopeNotificationsSettingsWindow>(
                    NotificationScope::NotificationScopePrivateChats));
  }
  {
    Outputter out;
    out << Outputter::RightPad{"<modify>"};
    add_element("groups settings", out.as_str(), out.markup(),
                create_menu_window_spawn_function<ScopeNotificationsSettingsWindow>(
                    NotificationScope::NotificationScopeGroupChats));
  }
  {
    Outputter out;
    out << Outputter::RightPad{"<modify>"};
    add_element("channel settings", out.as_str(), out.markup(),
                create_menu_window_spawn_function<ScopeNotificationsSettingsWindow>(
                    NotificationScope::NotificationScopeChannelChats));
  }
}

static std::shared_ptr<MenuWindow> spawn_rename_chat_window(MenuWindow &_window) {
  auto window = static_cast<AccountSettingsWindow *>(&_window);
  auto u = chat_manager().get_user(global_parameters().my_user_id());
  std::string text = PSTRING() << u->first_name() + " " << u->last_name();

  return spawn_field_edit_window(*window, "rename", text, [self = window](td::Result<std::string> R) {
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

    auto req = td::make_tl_object<td::td_api::setName>(first_name, last_name);

    spawn_yes_no_window_and_loading_windows(
        *self, PSTRING() << "Are you sure you want to change name to '" << first_name << " " << last_name << "'", {},
        true, "renaming", {}, std::move(req),
        [self, first_name, last_name](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
          DROP_IF_DELETED(R);
          if (R.is_error()) {
            spawn_error_window(*self, PSTRING() << "renaiming failed: " << R.move_as_error(), {});
            return;
          }
          self->updated_user_name(std::move(first_name), std::move(last_name));
        });
  });
}

void AccountSettingsWindow::updated_user_name(std::string first_name, std::string last_name) {
  Outputter out;
  out << first_name << " " << last_name << Outputter::RightPad{"<change>"};
  change_element(name_el_, [&]() {
    name_el_->menu_element()->data = out.as_str();
    name_el_->menu_element()->markup = out.markup();
  });
}

static std::shared_ptr<MenuWindow> spawn_change_photo_window(MenuWindow &_window) {
  auto window = static_cast<AccountSettingsWindow *>(&_window);
  auto u = chat_manager().get_user(global_parameters().my_user_id());

  return spawn_file_selection_window(*window, "user photo", "/", [self = window](td::Result<std::string> R) {
    if (R.is_error()) {
      return;
    }

    auto res = R.move_as_ok();

    if (res.size() > 0) {
      auto photo_file = td::make_tl_object<td::td_api::inputFileLocal>(res);
      auto photo = td::make_tl_object<td::td_api::inputChatPhotoStatic>(std::move(photo_file));
      auto req = td::make_tl_object<td::td_api::setProfilePhoto>(std::move(photo), false);
      spawn_yes_no_window_and_loading_windows(
          *self, "are you sure you want to change userpic?", {}, true, "changing userpic", {}, std::move(req),
          [self, res](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
            DROP_IF_DELETED(R);
            if (R.is_error()) {
              spawn_error_window(*self, PSTRING() << "setting user photo failed: " << R.move_as_error(), {});
              return;
            }
            self->updated_profile_photo(res);
          });
    }
  });
}

void AccountSettingsWindow::updated_profile_photo(std::string photo) {
  Outputter out;
  out << "updated" << Outputter::RightPad{"<change>"};
  change_element(photo_el_, [&]() {
    photo_el_->menu_element()->data = out.as_str();
    photo_el_->menu_element()->markup = out.markup();
  });
  set_need_refresh();
}

void AccountSettingsWindow::deleted_profile_photo() {
  Outputter out;
  out << "<empty>" << Outputter::RightPad{"<change>"};
  change_element(photo_el_, [&]() {
    photo_el_->menu_element()->data = out.as_str();
    photo_el_->menu_element()->markup = out.markup();
  });
  set_need_refresh();
}

static std::shared_ptr<MenuWindow> spawn_change_username_window(MenuWindow &_window) {
  auto window = static_cast<AccountSettingsWindow *>(&_window);
  auto u = chat_manager().get_user(global_parameters().my_user_id());
  std::string text = u->username();

  return spawn_field_edit_window(*window, "changeusername", text, [self = window](td::Result<std::string> R) {
    if (R.is_error()) {
      return;
    }

    auto text = R.move_as_ok();
    auto req = td::make_tl_object<td::td_api::setUsername>(text);

    spawn_yes_no_window_and_loading_windows(
        *self, PSTRING() << "are you sure, that you want to set username to '" << text << "'", {}, true,
        "updating username", {}, std::move(req),
        [self, username = text](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
          DROP_IF_DELETED(R);
          if (R.is_error()) {
            spawn_error_window(*self, PSTRING() << "setting username failed: " << R.move_as_error(), {});
            return;
          }
          self->updated_username(std::move(username));
        });
  });
}

void AccountSettingsWindow::updated_username(std::string username) {
  Outputter out;
  if (username.size() > 0) {
    out << "@" << username << Outputter::RightPad{"<change>"};
  } else {
    out << "<empty>" << Outputter::RightPad{"<set>"};
  }
  change_element(username_el_, [&]() {
    username_el_->menu_element()->data = out.as_str();
    username_el_->menu_element()->markup = out.markup();
  });
  set_need_refresh();
}

static std::shared_ptr<MenuWindow> spawn_change_bio_window(MenuWindow &_window) {
  auto window = static_cast<AccountSettingsWindow *>(&_window);
  std::string prompt = "update bio";
  std::string text = window->bio();

  return spawn_field_edit_window(*window, "bio", text, [self = window](td::Result<std::string> R) {
    if (R.is_error()) {
      return;
    }

    auto text = R.move_as_ok();
    auto req = td::make_tl_object<td::td_api::setBio>(text);

    spawn_yes_no_window_and_loading_windows(
        *self, PSTRING() << "are you sure, that you want to update bio to '" << text << "'", {}, true, "updating bio",
        {}, std::move(req), [self, bio = text](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
          DROP_IF_DELETED(R);
          if (R.is_error()) {
            spawn_error_window(*self, PSTRING() << "updating bio failed: " << R.move_as_error(), {});
            return;
          }
          self->updated_bio(std::move(bio));
        });
  });
}

void AccountSettingsWindow::updated_bio(std::string bio) {
  bio_ = bio;
  Outputter out;
  if (bio.size() > 0) {
    out << bio << Outputter::RightPad{"<change>"};
  } else {
    out << "<empty>" << Outputter::RightPad{"<set>"};
  }
  change_element(bio_el_, [&]() {
    bio_el_->menu_element()->data = out.as_str();
    bio_el_->menu_element()->markup = out.markup();
  });
  set_need_refresh();
}

static std::shared_ptr<MenuWindow> spawn_change_channel_window(MenuWindow &_window) {
  auto window = static_cast<AccountSettingsWindow *>(&_window);

  return spawn_chat_search_window(
      *window, ChatSearchWindow::Mode::Local, [self = window](td::Result<std::shared_ptr<Chat>> R) {
        if (R.is_error()) {
          return;
        }

        auto chat = R.move_as_ok();
        auto req = td::make_tl_object<td::td_api::setPersonalChat>(chat->chat_id());

        spawn_yes_no_window_and_loading_windows(
            *self, PSTRING() << "are you sure, that you want to set personal chat to '" << chat->title() << "'", {},
            true, "setting chat", {}, std::move(req),
            [self, chat = std::move(chat)](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
              DROP_IF_DELETED(R);
              if (R.is_error()) {
                spawn_error_window(*self, PSTRING() << "updating channel failed: " << R.move_as_error(), {});
                return;
              }
              self->updated_channel(chat);
            });
      });
}

void AccountSettingsWindow::updated_channel(std::shared_ptr<Chat> chat) {
  Outputter out;
  if (chat) {
    out << chat->title() << Outputter::RightPad{"<change>"};
  } else {
    out << "<empty>" << Outputter::RightPad{"<set>"};
  }
  change_element(channel_el_, [&]() {
    channel_el_->menu_element()->data = out.as_str();
    channel_el_->menu_element()->markup = out.markup();
  });
  set_need_refresh();
}

static std::shared_ptr<MenuWindow> spawn_change_birthdate_window(MenuWindow &_window) {
  auto window = static_cast<AccountSettingsWindow *>(&_window);
  std::string prompt = "update birthdate";
  auto u = chat_manager().get_user(global_parameters().my_user_id());
  std::string text = window->birthdate();

  return spawn_field_edit_window(*window, "birthdate", text, [self = window](td::Result<std::string> R) {
    if (R.is_error()) {
      return;
    }

    auto text = R.move_as_ok();

    auto x = td::full_split<td::Slice>(text, '.');
    if (x.size() != 2 && x.size() != 3) {
      LOG(ERROR) << "text = '" << text << "': size=" << x.size();
      return;
    }

    td::int32 day = 0;
    td::int32 month = 0;
    td::int32 year = 0;
    auto S = [&]() -> td::Status {
      TRY_RESULT_ASSIGN(day, td::to_integer_safe<td::int32>(x[0]));
      if (day < 0 || day > 31) {
        return td::Status::Error("out of range");
      }
      TRY_RESULT_ASSIGN(month, td::to_integer_safe<td::int32>(x[1]));
      if (month < 0 || month > 12) {
        return td::Status::Error("out of range");
      }
      if (x.size() == 3) {
        TRY_RESULT_ASSIGN(year, td::to_integer_safe<td::int32>(x[2]));
      }
      return td::Status::OK();
    }();
    if (S.is_error()) {
      LOG(ERROR) << "err=" << S;
      return;
    }

    auto req =
        td::make_tl_object<td::td_api::setBirthdate>(td::make_tl_object<td::td_api::birthdate>(day, month, year));

    spawn_yes_no_window_and_loading_windows(
        *self, PSTRING() << "are you sure, that you want to update birth date to " << text, {}, true,
        "updating birthdate", {}, std::move(req),
        [self, text](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
          DROP_IF_DELETED(R);
          if (R.is_error()) {
            spawn_error_window(*self, PSTRING() << "updating birth date failed: " << R.move_as_error(), {});
            return;
          }
          self->updated_birthdate(std::move(text));
        });
  });
}

void AccountSettingsWindow::updated_birthdate(std::string birthdate) {
  birthdate_ = birthdate;
  Outputter out;
  if (birthdate_.size() > 0) {
    out << birthdate_ << Outputter::RightPad{"<change>"};
  } else {
    out << "<empty>" << Outputter::RightPad{"<set>"};
  }
  change_element(birthdate_el_, [&]() {
    birthdate_el_->menu_element()->data = out.as_str();
    birthdate_el_->menu_element()->markup = out.markup();
  });
  set_need_refresh();
}

void AccountSettingsWindow::got_full_user(td::tl_object_ptr<td::td_api::userFullInfo> user_full) {
  auto user = chat_manager().get_user(global_parameters().my_user_id());
  CHECK(user);
  {
    Outputter out;
    out << user->first_name() << " " << user->last_name() << Outputter::RightPad{"<change>"};
    name_el_ = add_element("name", out.as_str(), out.markup(), spawn_rename_chat_window);
  }
  {
    Outputter out;
    out << "+" << user->phone_number() << Outputter::RightPad{"<can't change>"};
    add_element("phone", out.as_str(), out.markup());
  }
  if (user->photo()) {
    Outputter out;
    out << *user->photo() << Outputter::RightPad{"<change>"};
    photo_el_ = add_element("photo", out.as_str(), out.markup(), spawn_change_photo_window);
  } else {
    photo_el_ = add_element("photo", "<empty>", {}, spawn_change_photo_window);
  }
  if (user->username().size() > 0) {
    Outputter out;
    out << "@" << user->username() << Outputter::RightPad{"<change>"};
    username_el_ = add_element("username", out.as_str(), out.markup(), spawn_change_username_window);
  } else {
    Outputter out;
    out << "<empty>" << Outputter::RightPad{"<set>"};
    username_el_ = add_element("username", out.as_str(), out.markup(), spawn_change_username_window);
  }
  if (user_full->bio_ && user_full->bio_->text_.size() > 0) {
    bio_ = user_full->bio_->text_;
    Outputter out;
    out << *user_full->bio_ << Outputter::RightPad{"<change>"};
    bio_el_ = add_element("bio", out.as_str(), out.markup(), spawn_change_bio_window);
  } else {
    bio_ = "";
    Outputter out;
    out << "<empty>" << Outputter::RightPad{"<set>"};
    bio_el_ = add_element("bio", out.as_str(), out.markup(), spawn_change_bio_window);
  }
  if (user_full->personal_chat_id_) {
    auto chat = chat_manager().get_chat(user_full->personal_chat_id_);
    CHECK(chat);
    Outputter out;
    out << chat->title() << Outputter::RightPad{"<change>"};
    channel_el_ = add_element("channel", out.as_str(), out.markup(), spawn_change_channel_window);
  } else {
    Outputter out;
    out << "<empty>" << Outputter::RightPad{"<change>"};
    channel_el_ = add_element("channel", out.as_str(), out.markup(), spawn_change_channel_window);
  }
  if (user_full->birthdate_) {
    Outputter out;
    out << *user_full->birthdate_ << Outputter::RightPad{"<change>"};
    birthdate_el_ = add_element("birthdate", out.as_str(), out.markup(), spawn_change_birthdate_window);
  } else {
    Outputter out;
    out << "<empty>" << Outputter::RightPad{"<set>"};
    birthdate_el_ = add_element("birthdate", out.as_str(), out.markup(), spawn_change_birthdate_window);
  }
}

void AccountSettingsWindow::build_menu() {
  send_request(td::make_tl_object<td::td_api::getUserFullInfo>(global_parameters().my_user_id()),
               [window = this](td::Result<td::tl_object_ptr<td::td_api::userFullInfo>> R) {
                 if (R.is_error()) {
                   if (R.error().code() != ErrorCodeWindowDeleted) {
                     LOG(ERROR) << "failed to get userFull: received error: " << R.move_as_error();
                   }
                   return;
                 }
                 window->got_full_user(R.move_as_ok());
               });
}

void MainSettingsWindow::build_menu() {
  add_element("account settings", "", {}, create_menu_window_spawn_function<AccountSettingsWindow>());
  add_element("notification settings", "", {}, create_menu_window_spawn_function<NotificationsSettingsWindow>());
  add_element("privacy and security settings", "", {},
              create_menu_window_spawn_function<PrivacyAndSecuritySettingsWindow>());
  add_element("chat settings", "", {}, create_menu_window_spawn_function<ChatSettingsWindow>());
  add_element("folders settings", "", {}, create_menu_window_spawn_function<FoldersSettingsWindow>());
  add_element("advanced settings", "", {}, create_menu_window_spawn_function<FoldersSettingsWindow>());

  premium_ = add_element("premium", "");
  {
    const auto &stars_amount = global_parameters().stars_owned();
    if (stars_amount) {
      auto amount = (double)stars_amount->star_count_ + 1e-9 * stars_amount->nanostar_count_;
      add_element("stars owned", PSTRING() << amount);
    } else {
      add_element("stars owned", "0");
    }
  }
}

void MainSettingsWindow::send_requests() {
  auto req = td::make_tl_object<td::td_api::getPremiumState>();
  send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::premiumState>> R) {
    if (R.is_error()) {
      return;
    }
    self->got_premium(R.move_as_ok());
  });
}

void MainSettingsWindow::got_premium(td::tl_object_ptr<td::td_api::premiumState> res) {
  if (!res) {
    return;
  }
  Outputter out;
  out << *res->state_;

  change_element(premium_, [&]() {
    premium_->menu_element()->data = out.as_str();
    premium_->menu_element()->markup = out.markup();
  });
  set_need_refresh();
}

}  // namespace tdcurses
