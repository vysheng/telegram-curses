#pragma once

#include "managers/FileManager.hpp"
#include "common-windows/MenuWindow.hpp"
#include "common-windows/ErrorWindow.hpp"
#include "managers/GlobalParameters.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/Input.hpp"
#include "windows/Output.hpp"
#include "windows/TextEdit.hpp"
#include "windows/Window.hpp"
#include <memory>
#include <vector>

namespace tdcurses {

class ChatPhotoListWindow
    : public MenuWindow
    , public windows::Window {
 public:
  struct PhotoInfo {
    PhotoInfo(td::tl_object_ptr<td::td_api::file> file, td::int32 date) : file(std::move(file)), date(date) {
    }
    td::tl_object_ptr<td::td_api::file> file;
    td::int32 date;
    td::int64 download_id{0};
  };

  ChatPhotoListWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 user_id, td::int32 height,
                      td::int32 width)
      : MenuWindow(root, root_actor), user_id_(user_id), height_(height), width_(width) {
    update();
    request_photos();
  }

  ~ChatPhotoListWindow() {
    for (auto &p : photos_) {
      if (p.download_id > 0) {
        file_manager().unsubscribe_from_file_updates(p.file->id_, p.download_id);
        p.download_id = -1;
      }
    }
  }

  void request_photos() {
    auto req = td::make_tl_object<td::td_api::getUserProfilePhotos>(user_id_, (int)photos_.size(), 100);
    send_request(std::move(req), [self = this](td::Result<td::tl_object_ptr<td::td_api::chatPhotos>> R) mutable {
      DROP_IF_DELETED(R);
      if (R.is_error()) {
        spawn_error_window(*self, PSTRING() << "leaving chat failed: " << R.move_as_error(), {});
        return;
      }
      self->got_photos(R.move_as_ok());
    });
  }

  void got_photos(td::tl_object_ptr<td::td_api::chatPhotos> r) {
    total_photos_ = r->total_count_;
    if (!total_photos_) {
      return;
    }
    for (auto &p : r->photos_) {
      photos_.emplace_back(std::move(p->sizes_.back()->photo_), p->added_date_);
    }
    if ((int)photos_.size() < total_photos_ && r->photos_.size() > 0) {
      request_photos();
    }
    update();
  }

  void download() {
    if (pos_ >= photos_.size()) {
      return;
    }
    auto &photo = photos_[pos_];
    if (photo.download_id > 0) {
      return;
    }
    auto file_id = photo.file->id_;
    photo.download_id = file_manager().subscribe_to_file_updates(
        file_id,
        [root = root(), self = this, self_id = window_unique_id(), pos = pos_](const td::td_api::updateFile &upd) {
          if (!root->window_exists(self_id)) {
            return;
          }
          auto file = clone_td_file(*upd.file_);
          self->photos_[pos].file = std::move(file);
          self->update();
        });
    send_request(td::make_tl_object<td::td_api::downloadFile>(file_id, 16, 0, 0, true),
                 [root = root(), self = this, self_id = window_unique_id(),
                  pos = pos_](td::Result<td::tl_object_ptr<td::td_api::file>> R) {
                   if (!root->window_exists(self_id)) {
                     return;
                   }
                   if (R.is_error()) {
                     return;
                   }
                   self->photos_[pos].file = R.move_as_ok();
                   self->update();
                 });
  }

  void update() {
    set_need_refresh();
  }

  void render(windows::WindowOutputter &rb, bool force) override {
    rb.erase_rect(0, 0, height(), width());
    if (pos_ >= photos_.size()) {
      return;
    }
    auto dir = windows::SavedRenderedImagesDirectory(std::move(saved_images_));
    auto &photo = photos_[pos_];
    Outputter out;
    out << (pos_ + 1) << "/" << total_photos_ << " " << Outputter::Date{photo.date} << "\n";
    if (photo.file->local_->is_downloading_completed_) {
      if (!global_parameters().image_path_is_allowed(photo.file->local_->path_)) {
        out << "IMAGES ARE DISABLED IN SETTINGS MENU";
      } else {
        Outputter::Photo p;
        p.path = photo.file->local_->path_;
        p.height = height_;
        p.width = width_;
        p.allow_pixel = global_parameters().show_pixel_images();
        out << p;
      }
    } else {
      out << "LOADING...";
      download();
    }
    windows::TextEdit::render(rb, width(), out.as_cslice(), 0, out.markup(), false, false, &dir);
    saved_images_ = dir.release();
  }

  void handle_input(const windows::InputEvent &info) override {
    if (info == "h" || info == "T-Left") {
      if (total_photos_) {
        if (pos_ > 0) {
          pos_--;
        } else {
          if ((int)photos_.size() == total_photos_) {
            pos_ = total_photos_ - 1;
          }
        }
      }
    } else if (info == "l" || info == "T-Right") {
      if (total_photos_) {
        if (pos_ + 1 < photos_.size()) {
          pos_++;
        } else {
          if ((int)photos_.size() == total_photos_) {
            pos_ = 0;
          }
        }
      }
    } else {
      menu_window_handle_input(info);
    }
    set_need_refresh();
  }

  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> w) override {
    return std::static_pointer_cast<ChatPhotoListWindow>(std::move(w));
  }

  td::int32 best_width() override {
    auto r = windows::empty_window_outputter().rendered_image_height(20, 40, height_, width_, "");
    return r.second;
  }
  td::int32 best_height() override {
    auto r = windows::empty_window_outputter().rendered_image_height(20, 40, height_, width_, "");
    return r.first + 1;
  }

 private:
  td::int64 user_id_;

  std::vector<PhotoInfo> photos_;
  td::int32 total_photos_{0};
  size_t pos_{0};
  td::int32 height_;
  td::int32 width_;

  windows::SavedRenderedImages saved_images_;
};

}  // namespace tdcurses
