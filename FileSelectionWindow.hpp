#pragma once

#include "MenuWindowPad.hpp"
#include "GlobalParameters.hpp"
#include "td/telegram/PromoDataManager.h"
#include "td/utils/Promise.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/Status.h"
#include "td/utils/format.h"
#include "td/utils/port/path.h"
#include "port/dirlist.h"
#include "windows/Input.hpp"
#include "windows/Output.hpp"
#include <memory>

namespace tdcurses {

class FileSelectionWindow : public MenuWindowPad {
 public:
  enum SortMode : td::uint32 { Name, Mtime, Count };
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_abort(FileSelectionWindow &) = 0;
    virtual void on_answer(FileSelectionWindow &, std::string answer) = 0;
  };

  FileSelectionWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::shared_ptr<Callback> callback = nullptr)
      : MenuWindowPad(root, root_actor), callback_(std::move(callback)) {
    change_folder("/");
  }
  class Element : public windows::PadWindowElement {
   public:
    Element(std::string name, std::string display_name, const FileInfo &info, SortMode sort_mode)
        : name_(std::move(name)), display_name_(std::move(display_name)), info_(info), sort_mode_(sort_mode) {
    }
    bool is_less(const PadWindowElement &other) const override {
      const auto &o = static_cast<const Element &>(other);
      if (info_.is_dir) {
        if (o.info_.is_dir) {
          // FALLTHROUGH
        } else {
          return true;
        }
      } else {
        if (o.info_.is_dir) {
          return false;
        } else {
          // FALLTHROUGH
        }
      }
      switch (sort_mode_) {
        case SortMode::Name:
          return display_name_ < o.display_name_;
        case SortMode::Mtime:
          return info_.modified_at > o.info_.modified_at;
        default:
          UNREACHABLE();
      };
    }
    void handle_input(PadWindow &root, const windows::InputEvent &info) override {
      auto &w = static_cast<FileSelectionWindow &>(root);
      if (info == "T-Enter") {
        if (info_.is_dir) {
          w.change_folder(name_);
        } else {
          w.on_result(name_);
        }
      } else if (info == "T-Escape") {
        if (!w.sent_answer_) {
          w.callback_->on_abort(w);
        }
        w.sent_answer_ = true;
      } else if (info == "s") {
        w.set_sort_mode((SortMode)((w.sort_mode_ + 1) % SortMode::Count));
      }
    }

    td::int32 render(PadWindow &root, windows::WindowOutputter &rb, windows::SavedRenderedImagesDirectory &dir,
                     bool is_selected) override {
      std::string s;
      if (!info_.has_access) {
        s = "<INACCESSIBLE>";
      } else if (info_.is_dir) {
        s = "<DIR>";
      } else {
        s = PSTRING() << td::format::as_size(info_.size);
      }
      Outputter out;
      out << display_name_ << Outputter::RightPad{s};
      return render_plain_text(rb, out.as_cslice(), out.markup(), width(), 1, is_selected, &dir);
    }

   private:
    std::string name_;
    std::string display_name_;
    FileInfo info_;
    SortMode sort_mode_;
  };

  void add_file(std::string path, const FileInfo &info) {
    auto x = path.rfind('/');
    std::string display_path = (x == std::string::npos) ? path : path.substr(x + 1);
    auto e = std::make_shared<Element>(std::move(path), std::move(display_path), info, sort_mode_);
    add_element(std::move(e));
  }

  void on_result(std::string name) {
    if (!sent_answer_) {
      callback_->on_answer(*this, std::move(name));
    }
    sent_answer_ = true;
  }
  void change_folder(std::string name) {
    cur_folder_ = name;
    clear();

    set_title(name);

    iterate_files_in_directory(name, [&](const std::string &path, const FileInfo &info) { add_file(path, info); });

    set_need_refresh();
  }

  void install_callback(std::shared_ptr<Callback> callback) {
    callback_ = std::move(callback);
  }

  auto sort_mode() const {
    return sort_mode_;
  }

  void set_sort_mode(SortMode sort_mode) {
    sort_mode_ = sort_mode;
    change_folder(cur_folder_);
  }

  /*static MenuWindowSpawnFunction spawn_function(std::string text, td::Promise<std::string> promise) {
    class Cb : public Callback {
     public:
      Cb(td::Promise<std::string> promise) : promise_(std::move(promise)) {
      }

      void on_abort(FileSelectionWindow &) override {
        promise_.set_error(td::Status::Error("aborted"));
      }
      void on_answer(FileSelectionWindow &, std::string answer) override {
        promise_.set_value(std::move(answer));
      }

     private:
      td::Promise<std::string> promise_;
    };
    return [callback = std::make_shared<Cb>(std::move(promise))](
               Tdcurses *root, td::ActorId<Tdcurses> root_id) -> std::shared_ptr<MenuWindow> {
      return std::make_shared<FileSelectionWindow>(root, root_id, callback);
    };
  }*/

  ~FileSelectionWindow() {
    if (!sent_answer_) {
      callback_->on_abort(*this);
    }
    sent_answer_ = true;
  }

 private:
  std::shared_ptr<Callback> callback_;
  bool sent_answer_{false};
  SortMode sort_mode_{SortMode::Name};
  std::string cur_folder_;
};

template <typename T, typename F>
std::enable_if_t<std::is_base_of<MenuWindow, T>::value, std::shared_ptr<FileSelectionWindow>>
spawn_file_selection_window(T &cur_window, std::string caption, std::string initial_dir, F &&cb) {
  class Cb : public FileSelectionWindow::Callback {
   public:
    Cb(F &&cb, T *win) : cb_(std::move(cb)), self_(win), self_id_(win->window_unique_id()) {
    }
    void on_abort(FileSelectionWindow &w) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
        cb_(td::Status::Error("aborted"));
      }
    }
    void on_answer(FileSelectionWindow &w, std::string answer) override {
      if (w.root()->window_exists(self_id_)) {
        w.rollback();
        cb_(std::move(answer));
      }
    }

   private:
    F cb_;
    T *self_;
    td::int64 self_id_;
  };
  auto callback = std::make_unique<Cb>(std::move(cb), &cur_window);
  return cur_window.template spawn_submenu<FileSelectionWindow>(std::move(callback));
}

}  // namespace tdcurses
