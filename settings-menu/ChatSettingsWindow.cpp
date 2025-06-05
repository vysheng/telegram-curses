#include "ChatSettingsWindow.hpp"
#include "TdcursesLayout.hpp"
#include "managers/GlobalParameters.hpp"
#include "common-windows/LoadingWindow.hpp"
#include "common-windows/FieldEditWindow.hpp"
#include "common-windows/ErrorWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/utils/Status.h"
#include "td/utils/misc.h"

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
  {
    bool log_window_enabled = global_parameters().log_window_enabled();
    Outputter out;
    if (log_window_enabled) {
      out << "visible" << Outputter::RightPad{"<hide>"};
    } else {
      out << "invisible" << Outputter::RightPad{"<show>"};
    }
    log_window_enabled_el_ = add_element("log window", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
      bool enabled = !global_parameters().log_window_enabled();
      loading_window_send_request(
          w, "changing settings", {},
          td::make_tl_object<td::td_api::setOption>("X-log-window-enabled",
                                                    td::make_tl_object<td::td_api::optionValueBoolean>(enabled)),
          [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });

      global_parameters().set_log_window_enabled(enabled);
      Outputter out;
      if (enabled) {
        out << "visible" << Outputter::RightPad{"<hide>"};
      } else {
        out << "invisible" << Outputter::RightPad{"<show>"};
      }
      static_cast<ChatSettingsWindow &>(w).log_window_enabled_el_->menu_element()->data = out.as_str();
      static_cast<ChatSettingsWindow &>(w).log_window_enabled_el_->menu_element()->markup = out.markup();

      w.root()->update_layout_parameters();

      return false;
    });
  }
  {
    auto log_window_height = global_parameters().log_window_height();
    Outputter out;
    out << log_window_height << "%" << Outputter::RightPad{"<change>"};
    log_window_height_el_ = add_element("log window height", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
      auto cur = PSTRING() << global_parameters().log_window_height();
      spawn_field_edit_window(
          w, "log window height", cur, [&w, id = w.window_unique_id(), root = w.root()](td::Result<std::string> R) {
            if (R.is_error()) {
              return;
            }
            if (!root->window_exists(id)) {
              return;
            }
            auto new_str = R.move_as_ok();
            auto R2 = td::to_integer_safe<td::int32>(new_str);
            if (R2.is_error()) {
              spawn_error_window(w, "log window height should be an integer in range 3-40 (procent)", {});
              return;
            }
            auto new_res = R2.move_as_ok();
            if (new_res < 3 || new_res > 40) {
              spawn_error_window(w, "log window height should be an integer in range 3-40 (procent)", {});
              return;
            }
            Outputter out;
            out << new_res << "%" << Outputter::RightPad{"<change>"};
            static_cast<ChatSettingsWindow &>(w).allowed_image_extensions_el_->menu_element()->data = out.as_str();
            static_cast<ChatSettingsWindow &>(w).allowed_image_extensions_el_->menu_element()->markup = out.markup();
            loading_window_send_request(
                w, "changing settings", {},
                td::make_tl_object<td::td_api::setOption>("X-log-window-height",
                                                          td::make_tl_object<td::td_api::optionValueInteger>(new_res)),
                [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });
            global_parameters().set_log_window_height(new_res);
            w.root()->update_layout_parameters();
          });
      return false;
    });
  }
  {
    auto dialog_list_window_width = global_parameters().dialog_list_window_width();
    Outputter out;
    out << dialog_list_window_width << "%" << Outputter::RightPad{"<change>"};
    dialog_list_window_width_el_ =
        add_element("dialogs window width", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
          auto cur = PSTRING() << global_parameters().dialog_list_window_width();
          spawn_field_edit_window(
              w, "dialogs window width", cur,
              [&w, id = w.window_unique_id(), root = w.root()](td::Result<std::string> R) {
                if (R.is_error()) {
                  return;
                }
                if (!root->window_exists(id)) {
                  return;
                }
                auto new_str = R.move_as_ok();
                auto R2 = td::to_integer_safe<td::int32>(new_str);
                if (R2.is_error()) {
                  spawn_error_window(w, "dialogs window width should be an integer in range 10-50 (procent)", {});
                  return;
                }
                auto new_res = R2.move_as_ok();
                if (new_res < 10 || new_res > 50) {
                  spawn_error_window(w, "dialogs window width should be an integer in range 10-50 (procent)", {});
                  return;
                }
                Outputter out;
                out << new_res << "%" << Outputter::RightPad{"<change>"};
                static_cast<ChatSettingsWindow &>(w).allowed_image_extensions_el_->menu_element()->data = out.as_str();
                static_cast<ChatSettingsWindow &>(w).allowed_image_extensions_el_->menu_element()->markup =
                    out.markup();
                loading_window_send_request(
                    w, "changing settings", {},
                    td::make_tl_object<td::td_api::setOption>(
                        "X-dialog-list-window-width", td::make_tl_object<td::td_api::optionValueInteger>(new_res)),
                    [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });
                global_parameters().set_dialog_list_window_width(new_res);
                w.root()->update_layout_parameters();
              });
          return false;
        });
  }
  {
    auto compose_window_height = global_parameters().compose_window_height();
    Outputter out;
    out << compose_window_height << "%" << Outputter::RightPad{"<change>"};
    compose_window_height_el_ =
        add_element("compose window height", out.as_str(), out.markup(), [](MenuWindowCommon &w) {
          auto cur = PSTRING() << global_parameters().compose_window_height();
          spawn_field_edit_window(
              w, "compose window width", cur,
              [&w, id = w.window_unique_id(), root = w.root()](td::Result<std::string> R) {
                if (R.is_error()) {
                  return;
                }
                if (!root->window_exists(id)) {
                  return;
                }
                auto new_str = R.move_as_ok();
                auto R2 = td::to_integer_safe<td::int32>(new_str);
                if (R2.is_error()) {
                  spawn_error_window(w, "compose window height should be an integer in range 10-50 (procent)", {});
                  return;
                }
                auto new_res = R2.move_as_ok();
                if (new_res < 10 || new_res > 50) {
                  spawn_error_window(w, "compose window height should be an integer in range 10-50 (procent)", {});
                  return;
                }
                Outputter out;
                out << new_res << "%" << Outputter::RightPad{"<change>"};
                static_cast<ChatSettingsWindow &>(w).allowed_image_extensions_el_->menu_element()->data = out.as_str();
                static_cast<ChatSettingsWindow &>(w).allowed_image_extensions_el_->menu_element()->markup =
                    out.markup();
                loading_window_send_request(
                    w, "changing settings", {},
                    td::make_tl_object<td::td_api::setOption>(
                        "X-compose-window-height", td::make_tl_object<td::td_api::optionValueInteger>(new_res)),
                    [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); });
                global_parameters().set_compose_window_height(new_res);
                w.root()->update_layout_parameters();
              });
          return false;
        });
  }
}

}  // namespace tdcurses
