#include "ChatSettingsWindow.hpp"
#include "GlobalParameters.hpp"
#include "LoadingWindow.hpp"
#include "FieldEditWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/utils/Status.h"

namespace tdcurses {

void ChatSettingsWindow::build_menu() {
  {
    bool markdown_enabled = global_parameters().use_markdown();
    Outputter out;
    if (markdown_enabled) {
      out << "enabled" << Outputter::RightPad{"<disable>"};
    } else {
      out << "disabled" << Outputter::RightPad{"<enable>"};
    }
    markdown_enabled_el_ = add_element("parse markdown", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
      bool enabled = !global_parameters().use_markdown();
      loading_window_send_request(
          w, "changing settings", {},
          td::make_tl_object<td::td_api::setOption>("X-markdown-enabled",
                                                    td::make_tl_object<td::td_api::optionValueBoolean>(enabled)),
          [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });

      global_parameters().set_notifications_enabled(enabled);
      Outputter out;
      if (enabled) {
        out << "enabled" << Outputter::RightPad{"<disable>"};
      } else {
        out << "disabled" << Outputter::RightPad{"<enable>"};
      }
      static_cast<ChatSettingsWindow &>(w).markdown_enabled_el_->menu_element()->data = out.as_str();
      static_cast<ChatSettingsWindow &>(w).markdown_enabled_el_->menu_element()->markup = out.markup();
      return false;
    });
  }
  {
    bool show_images_enabled = global_parameters().show_images();
    Outputter out;
    if (show_images_enabled) {
      out << "enabled" << Outputter::RightPad{"<disable>"};
    } else {
      out << "disabled" << Outputter::RightPad{"<enable>"};
    }
    show_images_enabled_el_ = add_element("show images", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
      bool enabled = !global_parameters().show_images();
      loading_window_send_request(
          w, "changing settings", {},
          td::make_tl_object<td::td_api::setOption>("X-show-images-enabled",
                                                    td::make_tl_object<td::td_api::optionValueBoolean>(enabled)),
          [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });

      global_parameters().set_show_images(enabled);
      Outputter out;
      if (enabled) {
        out << "enabled" << Outputter::RightPad{"<disable>"};
      } else {
        out << "disabled" << Outputter::RightPad{"<enable>"};
      }
      static_cast<ChatSettingsWindow &>(w).show_images_enabled_el_->menu_element()->data = out.as_str();
      static_cast<ChatSettingsWindow &>(w).show_images_enabled_el_->menu_element()->markup = out.markup();
      return false;
    });
  }
  {
    bool show_images_enabled = global_parameters().show_pixel_images();
    Outputter out;
    if (show_images_enabled) {
      out << "enabled" << Outputter::RightPad{"<disable>"};
    } else {
      out << "disabled" << Outputter::RightPad{"<enable>"};
    }
    show_pixel_images_enabled_el_ =
        add_element("show pixel images", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
          bool enabled = !global_parameters().show_pixel_images();
          loading_window_send_request(
              w, "changing settings", {},
              td::make_tl_object<td::td_api::setOption>("X-show-pixel-images-enabled",
                                                        td::make_tl_object<td::td_api::optionValueBoolean>(enabled)),
              [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });

          global_parameters().set_show_pixel_images(enabled);
          Outputter out;
          if (enabled) {
            out << "enabled" << Outputter::RightPad{"<disable>"};
          } else {
            out << "disabled" << Outputter::RightPad{"<enable>"};
          }
          static_cast<ChatSettingsWindow &>(w).show_pixel_images_enabled_el_->menu_element()->data = out.as_str();
          static_cast<ChatSettingsWindow &>(w).show_pixel_images_enabled_el_->menu_element()->markup = out.markup();
          return false;
        });
  }
  {
    std::string allowed_extensions = global_parameters().export_allowed_image_extensions();
    Outputter out;
    out << allowed_extensions << Outputter::RightPad{"<change>"};
    allowed_image_extensions_el_ =
        add_element("images extensions", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
          std::string allowed_extensions = global_parameters().export_allowed_image_extensions();
          spawn_field_edit_window(
              w, "image extensions", allowed_extensions,
              [&w, id = w.window_unique_id(), root = w.root()](td::Result<std::string> R) {
                if (R.is_error()) {
                  return;
                }
                if (!root->window_exists(id)) {
                  return;
                }
                auto allowed_extensions = R.move_as_ok();
                Outputter out;
                out << allowed_extensions << Outputter::RightPad{"<change>"};
                static_cast<ChatSettingsWindow &>(w).allowed_image_extensions_el_->menu_element()->data = out.as_str();
                static_cast<ChatSettingsWindow &>(w).allowed_image_extensions_el_->menu_element()->markup =
                    out.markup();
                loading_window_send_request(w, "changing settings", {},
                                            td::make_tl_object<td::td_api::setOption>(
                                                "X-allowed-image-extensions",
                                                td::make_tl_object<td::td_api::optionValueString>(allowed_extensions)),
                                            [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });
              });
          return false;
        });
  }
}

}  // namespace tdcurses
