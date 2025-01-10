#pragma once

#include "FileManager.hpp"
#include "MenuWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "windows/Input.hpp"
#include "windows/Output.hpp"
#include "windows/TextEdit.hpp"
#include "windows/ViewWindow.hpp"
#include "windows/Window.hpp"
#include <memory>

namespace tdcurses {

class ChatPhotoWindow
    : public MenuWindow
    , public windows::Window {
 public:
  ChatPhotoWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::tl_object_ptr<td::td_api::file> photo,
                  td::int32 height, td::int32 width)
      : MenuWindow(root, root_actor), photo_(std::move(photo)), height_(height), width_(width) {
    update();
  }

  ~ChatPhotoWindow() {
    if (download_id_ > 0) {
      file_manager().unsubscribe_from_file_updates(photo_->id_, download_id_);
      download_id_ = -1;
    }
  }

  void download() {
    if (download_id_ > 0) {
      return;
    }
    auto file_id = photo_->id_;
    download_id_ = file_manager().subscribe_to_file_updates(
        file_id, [root = root(), self = this, self_id = window_unique_id()](const td::td_api::updateFile &upd) {
          if (!root->window_exists(self_id)) {
            return;
          }
          auto file = clone_td_file(*upd.file_);
          self->photo_ = std::move(file);
          self->update();
        });
    send_request(
        td::make_tl_object<td::td_api::downloadFile>(file_id, 16, 0, 0, true),
        [root = root(), self = this, self_id = window_unique_id()](td::Result<td::tl_object_ptr<td::td_api::file>> R) {
          if (!root->window_exists(self_id)) {
            return;
          }
          if (R.is_error()) {
            return;
          }
          self->photo_ = R.move_as_ok();
          self->update();
        });
  }

  void update() {
    set_need_refresh();
  }

  void render(windows::WindowOutputter &rb, bool force) override {
    auto dir = windows::SavedRenderedImagesDirectory(std::move(saved_images_));
    Outputter out;
    if (photo_->local_->is_downloading_completed_) {
      Outputter::Photo p;
      p.path = photo_->local_->path_;
      p.height = height_;
      p.width = width_;
      out << p;
      rb.erase_rect(0, 0, height(), width());
    } else {
      out << "LOADING ";
      rb.erase_rect(0, 0, height(), width());
      download();
    }
    windows::TextEdit::render(rb, width(), out.as_cslice(), 0, out.markup(), false, false, &dir);
    saved_images_ = dir.release();
  }

  void handle_input(const windows::InputEvent &info) override {
    menu_window_handle_input(info);
  }

  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> w) override {
    return std::static_pointer_cast<ChatPhotoWindow>(std::move(w));
  }

  td::int32 best_width() override {
    auto r = windows::empty_window_outputter().rendered_image_height(20, 40, height_, width_, "");
    return r.second;
  }
  td::int32 best_height() override {
    auto r = windows::empty_window_outputter().rendered_image_height(20, 40, height_, width_, "");
    return r.first;
  }

 private:
  td::tl_object_ptr<td::td_api::file> photo_;
  td::int32 height_;
  td::int32 width_;

  td::int64 download_id_{-1};

  windows::SavedRenderedImages saved_images_;
};

}  // namespace tdcurses
