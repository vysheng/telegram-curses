#pragma once

#include "MenuWindowCommon.hpp"

namespace tdcurses {

class AccountSettingsWindow : public MenuWindowCommon {
 public:
  AccountSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor)
      : MenuWindowCommon(root, std::move(root_actor)) {
    build_menu();
  }

  void build_menu();
  void got_full_user(td::tl_object_ptr<td::td_api::userFullInfo> user_full);

  void updated_user_name(std::string first_name, std::string last_name);
  void updated_profile_photo(std::string photo);
  void deleted_profile_photo();
  void updated_username(std::string username);
  void updated_bio(std::string bio);
  void updated_channel(std::shared_ptr<Chat> chat);
  void updated_birthdate(std::string birthdate);

  auto bio() const {
    return bio_;
  }
  auto birthdate() const {
    return birthdate_;
  }

 private:
  std::shared_ptr<ElInfo> name_el_;
  std::shared_ptr<ElInfo> photo_el_;
  std::shared_ptr<ElInfo> username_el_;
  std::shared_ptr<ElInfo> bio_el_;
  std::shared_ptr<ElInfo> channel_el_;
  std::shared_ptr<ElInfo> birthdate_el_;

  std::string bio_;
  std::string birthdate_;
};

}  // namespace tdcurses
