#pragma once

#include "common-windows/MenuWindowCommon.hpp"

namespace tdcurses {

class ChatSettingsWindow : public MenuWindowCommon {
 public:
  ChatSettingsWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindowCommon(root, std::move(root_actor)) {
    build_menu();
  }
  void build_menu();
  void enabled_disable();

 private:
  std::shared_ptr<ElInfo> markdown_enabled_el_;
  std::shared_ptr<ElInfo> show_images_enabled_el_;
  std::shared_ptr<ElInfo> show_pixel_images_enabled_el_;
  std::shared_ptr<ElInfo> allowed_image_extensions_el_;
  std::shared_ptr<ElInfo> log_window_enabled_el_;
  std::shared_ptr<ElInfo> log_window_height_el_;
  std::shared_ptr<ElInfo> dialog_list_window_width_el_;
  std::shared_ptr<ElInfo> compose_window_height_el_;
};

}  // namespace tdcurses
