#include "AttachDocumentMenu.hpp"
#include "ComposeWindow.hpp"

namespace tdcurses {

void AttachDocumentMenu::build_menu() {
  for (size_t i = 0; i < attached_files_.size(); i++) {
    auto &x = attached_files_[i];
    Outputter out;
    out << x << Outputter::RightPad{"<remove>"};
    add_element("file", out.as_str(), out.markup(), [self = this, self_id = window_unique_id(), root = root(), i]() {
      if (!root->window_exists(self_id)) {
        return false;
      }
      self->del_file(i);
      return false;
    });
  }
  if (attached_files_.size() < 10) {
    add_element("add", "new file", {},
                create_menu_window_spawn_function<FileSelectionWindow>(create_file_selection_callback()));
  }
  add_element("save", "update attachments", {},
              [self_id = window_unique_id(), root = root(), type = attach_type_, files = attached_files_,
               compose_window = compose_window_, compose_window_id = compose_window_id_]() mutable {
                if (!root->window_exists(self_id)) {
                  return false;
                }

                if (!root->window_exists(compose_window_id)) {
                  return true;
                }

                compose_window->update_attach(type, std::move(files));
                return true;
              });
}

}  // namespace tdcurses
