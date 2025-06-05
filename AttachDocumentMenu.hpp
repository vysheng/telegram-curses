#pragma once

#include "common-windows/MenuWindowCommon.hpp"
#include "common-windows/FileSelectionWindow.hpp"
#include "AttachType.h"
#include <memory>
#include <vector>

namespace tdcurses {

class AttachDocumentMenu : public MenuWindowCommon {
 public:
  AttachDocumentMenu(Tdcurses *root, td::ActorId<Tdcurses> root_actor, AttachType attach_type,
                     std::vector<std::string> attached_files, ComposeWindow *compose_window,
                     td::int64 compose_window_id)
      : MenuWindowCommon(root, root_actor)
      , attach_type_(attach_type)
      , attached_files_(std::move(attached_files))
      , compose_window_(std::move(compose_window))
      , compose_window_id_(compose_window_id) {
    build_menu();
  }

  void build_menu();

  static bool allowed_extension(td::Slice fname, AttachType type) {
    auto c = fname.rfind('/');
    if (c != td::Slice::npos) {
      fname.remove_prefix(c + 1);
    }
    c = fname.rfind('.');
    if (c != td::Slice::npos) {
      fname.remove_prefix(c + 1);
    } else {
      fname = "";
    }
    auto ext = fname;
    if (type == AttachType::Audio) {
      return ext == "mp3";
    } else if (type == AttachType::Doc) {
      return true;
    } else if (type == AttachType::Photo) {
      return ext == "jpeg" || ext == "jpg" || ext == "png";
    } else if (type == AttachType::Video) {
      return ext == "mp4";
    } else {
      LOG(ERROR) << "type=" << (td::int32)type;
      return false;
    }
  }

  std::shared_ptr<FileSelectionWindow::Callback> create_file_selection_callback() {
    class Cb : public FileSelectionWindow::Callback {
     public:
      Cb(AttachType type) : type_(type) {
      }
      void on_abort(FileSelectionWindow &w) override {
        w.rollback();
      }
      void on_answer(FileSelectionWindow &w, std::string answer) override {
        auto s = w.rollback();
        static_cast<AttachDocumentMenu *>(s.get())->add_file(answer);
      }
      bool allow_file(td::Slice path, const FileInfo &info) override {
        return info.is_dir || AttachDocumentMenu::allowed_extension(path, type_);
      }

     private:
      AttachType type_;
    };
    return std::make_shared<Cb>(attach_type_);
  }

  void add_file(std::string answer) {
    attached_files_.push_back(answer);
    clear();
    build_menu();
  }

  void del_file(size_t idx) {
    CHECK(idx < attached_files_.size());
    attached_files_.erase(attached_files_.begin() + idx);
    clear();
    build_menu();
  }

 private:
  AttachType attach_type_;
  std::vector<std::string> attached_files_;
  ComposeWindow *compose_window_;
  td::int64 compose_window_id_;
};

}  // namespace tdcurses
