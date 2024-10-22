#include "SettingsWindow.hpp"
#include "ChatManager.hpp"
#include "GlobalParameters.hpp"
#include "FileSelectionWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "TdObjectsOutput.h"
#include "FieldEditWindow.hpp"
#include "FileSelectionWindow.hpp"
#include "td/utils/Status.h"
#include <memory>

namespace tdcurses {

static std::shared_ptr<MenuWindow> spawn_rename_chat_window(MenuWindow &_window) {
  AccountSettingsWindow &window = static_cast<AccountSettingsWindow &>(_window);
  auto user = chat_manager().get_user(global_parameters().my_user_id());
  std::string prompt = "change name";
  std::string text = user->first_name() + " " + user->last_name();

  auto cb = [self = &window](FieldEditWindow &w, td::Result<std::string> R) {
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
    self->run_change_name(first_name, last_name);
    /*auto req = td::make_tl_object<td::td_api::setName>(first_name, last_name);
    w.send_request(std::move(req),
                   td::PromiseCreator::lambda(
                       [promise = std::move(promise)](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
                         if (R.is_error()) {
                           promise.set_error(R.move_as_error());
                         } else {
                           promise.set_value(td::Unit());
                         }
                       }));*/
  };

  return window.spawn_submenu<FieldEditWindow>(prompt, text, FieldEditWindow::make_callback(std::move(cb)));
}

void AccountSettingsWindow::run_change_name(std::string first_name, std::string last_name) {
}

/*static MenuWindowSpawnFunction spawn_change_chat_photo_window(AccountSettingsWindow *window) {
  std::string prompt = "change name";

  std::function<void(FileSelectionWindow &, std::string, td::Promise<td::Unit>)> cb;

  cb = [](FileSelectionWindow &w, std::string text, td::Promise<td::Unit> promise) {
    auto req = td::make_tl_object<td::td_api::setProfilePhoto>(
        td::make_tl_object<td::td_api::inputChatPhotoStatic>(td::make_tl_object<td::td_api::inputFileLocal>(text)),
        true);
    w.send_request(std::move(req),
                   td::PromiseCreator::lambda(
                       [promise = std::move(promise)](td::Result<td::tl_object_ptr<td::td_api::ok>> R) mutable {
                         if (R.is_error()) {
                           promise.set_error(R.move_as_error());
                         } else {
                           promise.set_value(td::Unit());
                         }
                       }));
  };

  return FileSelectionWindow::spawn_function(prompt, cb);
}*/

void AccountSettingsWindow::got_full_user(td::tl_object_ptr<td::td_api::userFullInfo> user_full) {
  auto user = chat_manager().get_user(global_parameters().my_user_id());
  CHECK(user);
  {
    Outputter out;
    out << user->first_name() << " " << user->last_name() << Outputter::RightPad{"<change>"};
    add_element("name", out.as_str(), out.markup(), spawn_rename_chat_window);
  }
  {
    Outputter out;
    out << "+" << user->phone_number() << Outputter::RightPad{"<can't change>"};
    add_element("phone", out.as_str(), out.markup());
  }
  if (user->photo()) {
    Outputter out;
    out << *user->photo();
    add_element("photo", out.as_str(), out.markup());
  } else {
    add_element("photo", "<empty>", {});
  }
  if (user->username().size() > 0) {
    add_element("username", "@" + user->username());
  } else {
    add_element("username", "<empty>");
  }
  if (user_full->bio_ && user_full->bio_->text_.size() > 0) {
    Outputter out;
    add_element("bio", out.as_str(), out.markup());
  } else {
    add_element("bio", "<empty>");
  }
  if (user_full->personal_chat_id_) {
    auto chat = chat_manager().get_chat(user_full->personal_chat_id_);
    CHECK(chat);
    add_element("channel", chat->title());
  } else {
    add_element("channel", "<empty>");
  }
  if (user_full->birthdate_) {
    Outputter out;
    out << *user_full->birthdate_;
    add_element("birthdate", out.as_str(), out.markup());
  } else {
    add_element("birthdate", "<empty>");
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
}

}  // namespace tdcurses