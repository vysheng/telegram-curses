#pragma once

#include "MenuWindowPad.hpp"
#include "GlobalParameters.hpp"
#include "td/utils/Promise.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/format.h"
#include "td/utils/port/path.h"
#include "port/dirlist.h"
#include <memory>

namespace tdcurses {

class FileSelectionWindow
    : public windows::PadWindow
    , public TdcursesWindowBase {
 public:
  enum SortMode : td::uint32 { Name, Mtime, Count };
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_abort() = 0;
    virtual void on_answer(std::string answer) = 0;
  };

  FileSelectionWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::unique_ptr<Callback> callback = nullptr)
      : TdcursesWindowBase(root, root_actor), callback_(std::move(callback)) {
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
    void handle_input(PadWindow &root, TickitKeyEventInfo *info) override {
      auto &w = static_cast<FileSelectionWindow &>(root);
      if (info->type == TICKIT_KEYEV_KEY) {
        if (!strcmp(info->str, "Enter")) {
          if (info_.is_dir) {
            w.change_folder(name_);
          } else {
            w.on_result(name_);
          }
        } else if (!strcmp(info->str, "Escape")) {
          w.callback_->on_abort();
        }
      } else {
        if (!strcmp(info->str, "s")) {
          w.set_sort_mode((SortMode)((w.sort_mode_ + 1) % SortMode::Count));
        }
      }
    }

    td::int32 render(PadWindow &root, TickitRenderBuffer *rb, bool is_selected) override {
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
      return render_plain_text(rb, out.as_cslice(), out.markup(), width(), 1, is_selected);
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
    callback_->on_answer(std::move(name));
  }
  void change_folder(std::string name) {
    cur_folder_ = name;
    clear();

    set_title(name);

    iterate_files_in_directory(name, [&](const std::string &path, const FileInfo &info) { add_file(path, info); });

    set_need_refresh();
  }

  void install_callback(std::unique_ptr<Callback> callback) {
    callback_ = std::move(callback);
  }

  auto sort_mode() const {
    return sort_mode_;
  }

  void set_sort_mode(SortMode sort_mode) {
    sort_mode_ = sort_mode;
    change_folder(cur_folder_);
  }

 private:
  std::unique_ptr<Callback> callback_;
  SortMode sort_mode_{SortMode::Name};
  std::string cur_folder_;
};

}  // namespace tdcurses
