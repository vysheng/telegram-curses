#pragma once

#include "common-windows/MenuWindowCommon.hpp"
#include "managers/ChatManager.hpp"
#include "AttachType.h"
#include "AttachDocumentMenu.hpp"
#include "td/tl/TlObject.h"
#include <memory>
#include <vector>

namespace tdcurses {

class AttachMenu : public MenuWindowCommon {
 public:
  AttachMenu(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id, std::string text,
             AttachType attach_type, std::vector<std::string> attached_files, ComposeWindow *compose_window,
             td::int64 compose_window_id)
      : MenuWindowCommon(root, root_actor)
      , chat_id_(chat_id)
      , text_(std::move(text))
      , attach_type_(attach_type)
      , attached_files_(std::move(attached_files))
      , compose_window_(std::move(compose_window))
      , compose_window_id_(compose_window_id) {
    build_menu();
  }

  void build_menu() {
    auto chat = chat_manager().get_chat(chat_id_);
    if (attach_type_ == AttachType::Photo ||
        (attach_type_ == AttachType::None && (!chat || chat->permissions()->can_send_photos_))) {
      add_option_attach_file(AttachType::Photo);
    }
    if (attach_type_ == AttachType::Audio ||
        (attach_type_ == AttachType::None && (!chat || chat->permissions()->can_send_audios_))) {
      add_option_attach_file(AttachType::Audio);
    }
    if (attach_type_ == AttachType::Video ||
        (attach_type_ == AttachType::None && (!chat || chat->permissions()->can_send_videos_))) {
      add_option_attach_file(AttachType::Video);
    }
    if (attach_type_ == AttachType::Doc ||
        (attach_type_ == AttachType::None && (!chat || chat->permissions()->can_send_documents_))) {
      add_option_attach_file(AttachType::Doc);
    }
    if (attach_type_ == AttachType::Geo ||
        (attach_type_ == AttachType::None && (!chat || chat->permissions()->can_send_other_messages_))) {
      add_option_attach_geo();
    }
    if (attach_type_ == AttachType::Contact ||
        (attach_type_ == AttachType::None && (!chat || chat->permissions()->can_send_other_messages_))) {
      add_option_attach_contact();
    }
    if (attach_type_ == AttachType::Poll ||
        (attach_type_ == AttachType::None && (!chat || chat->permissions()->can_send_polls_))) {
      add_option_attach_poll();
    }
    add_option_clear();
  }

  static std::string get_attach_type(AttachType type) {
    switch (type) {
      case AttachType::None:
        return "*";
      case AttachType::Photo:
        return "photo";
      case AttachType::Audio:
        return "audio";
      case AttachType::Video:
        return "video";
      case AttachType::Doc:
        return "document";
      case AttachType::Geo:
        return "location";
      case AttachType::Contact:
        return "contact";
      case AttachType::Poll:
        return "poll";
      default:
        return "?";
    }
  }

  void add_option_attach_file(AttachType attach_type) {
    add_element("add", get_attach_type(attach_type), {},
                create_menu_window_spawn_function<AttachDocumentMenu>(attach_type, attached_files_, compose_window_,
                                                                      compose_window_id_));
  }
  void add_option_attach_geo() {
    add_element("add", "location");
  }
  void add_option_attach_contact() {
    add_element("add", "contact");
  }
  void add_option_attach_poll() {
    add_element("add", "poll");
  }

  void add_option_clear();

 private:
  td::int64 chat_id_;
  std::string text_;
  AttachType attach_type_;
  std::vector<std::string> attached_files_;
  ComposeWindow *compose_window_;
  td::int64 compose_window_id_;
};

}  // namespace tdcurses
