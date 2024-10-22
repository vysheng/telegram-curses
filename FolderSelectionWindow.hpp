#pragma once

#include "MenuWindowPad.hpp"
#include "DialogListWindow.hpp"
#include "GlobalParameters.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include <memory>

namespace tdcurses {

class FolderSelectionWindow : public MenuWindowPad {
 public:
  FolderSelectionWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindowPad(root, root_actor) {
    build_folders();
  }
  class Element : public windows::PadWindowElement {
   public:
    Element(std::string text, DialogListWindow::Sublist chat_list, size_t idx)
        : text_(std::move(text)), chat_list_(std::move(chat_list)), idx_(idx) {
    }
    bool is_less(const PadWindowElement &other) const override {
      return idx_ < static_cast<const Element &>(other).idx_;
    }
    void handle_input(PadWindow &root, TickitKeyEventInfo *info) override {
      if (info->type == TICKIT_KEYEV_KEY) {
        if (!strcmp(info->str, "Enter")) {
          auto &w = static_cast<FolderSelectionWindow &>(root);
          w.root()->dialog_list_window()->set_sublist(std::move(chat_list_));
          w.exit();
          return;
        }
      }
    }

    td::int32 render(PadWindow &root, TickitRenderBuffer *rb, bool is_selected) override {
      return render_plain_text(rb, text_, width(), 1, is_selected);
    }

   private:
    std::string text_;
    DialogListWindow::Sublist chat_list_;
    size_t idx_;
  };

  void add_folder(std::string name, DialogListWindow::Sublist chat_list) {
    auto e = std::make_shared<Element>(std::move(name), std::move(chat_list), ++last_id_);
    add_element(std::move(e));
  }

  void build_folders() {
    add_folder("main", DialogListWindow::SublistGlobal{});
    add_folder("archive", DialogListWindow::SublistArchive{});
    for (auto &f : global_parameters().chat_folders()) {
      add_folder(f->title_, DialogListWindow::SublistSublist{f->id_});
    }
    set_need_refresh();
  }

 private:
  size_t last_id_{0};
};

}  // namespace tdcurses
