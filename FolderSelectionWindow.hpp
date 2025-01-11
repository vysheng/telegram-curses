#pragma once

#include "MenuWindowPad.hpp"
#include "DialogListWindow.hpp"
#include "GlobalParameters.hpp"
#include "TdObjectsOutput.h"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "windows/Markup.hpp"
#include "windows/Output.hpp"
#include <memory>
#include <vector>

namespace tdcurses {

class FolderSelectionWindow : public MenuWindowPad {
 public:
  FolderSelectionWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindowPad(root, root_actor) {
    build_folders();
  }
  class Element : public windows::PadWindowElement {
   public:
    Element(std::string text, std::vector<windows::MarkupElement> markup, DialogListWindow::Sublist chat_list,
            size_t idx)
        : text_(std::move(text)), markup_(std::move(markup)), chat_list_(std::move(chat_list)), idx_(idx) {
    }
    bool is_less(const PadWindowElement &other) const override {
      return idx_ < static_cast<const Element &>(other).idx_;
    }
    void handle_input(PadWindow &root, const windows::InputEvent &info) override {
      if (info == "T-Enter") {
        auto &w = static_cast<FolderSelectionWindow &>(root);
        w.root()->dialog_list_window()->set_sublist(std::move(chat_list_));
        w.exit();
        return;
      }
    }

    td::int32 render(PadWindow &root, windows::WindowOutputter &rb, windows::SavedRenderedImagesDirectory &dir,
                     bool is_selected) override {
      return render_plain_text(rb, text_, markup_, width(), 1, is_selected, &dir);
    }

   private:
    std::string text_;
    std::vector<windows::MarkupElement> markup_;
    DialogListWindow::Sublist chat_list_;
    size_t idx_;
  };

  void add_folder(std::string name, std::vector<windows::MarkupElement> markup, DialogListWindow::Sublist chat_list) {
    auto e = std::make_shared<Element>(std::move(name), std::move(markup), std::move(chat_list), ++last_id_);
    add_element(std::move(e));
  }

  void build_folders() {
    add_folder("main", {}, DialogListWindow::SublistGlobal{});
    add_folder("archive", {}, DialogListWindow::SublistArchive{});
    for (auto &f : global_parameters().chat_folders()) {
      Outputter out;
      out << *f->name_->text_;
      add_folder(out.as_str(), out.markup(), DialogListWindow::SublistSublist{f->id_});
    }
    set_need_refresh();
  }

  td::int32 best_width() override {
    size_t min = 7; /*archive*/
    for (auto &f : global_parameters().chat_folders()) {
      if (f->name_->text_->text_.size() > min) {
        min = f->name_->text_->text_.size();
      }
    }
    return (td::int32)min + 2;
  }
  td::int32 best_height() override {
    return 4 + (td::int32)global_parameters().chat_folders().size();
  }

 private:
  size_t last_id_{0};
};

}  // namespace tdcurses
