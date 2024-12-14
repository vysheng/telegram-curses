#pragma once

#include "td/utils/Time.h"
#include "td/utils/logging.h"
#include <atomic>
#include <memory>
#include <utility>
#include <vector>
#include "Window.hpp"

namespace windows {

enum class BorderType { None, Simple, Double };

const std::vector<std::string> &get_border_type_info(BorderType border_type, bool is_active);

class BorderedWindow : public Window {
 public:
  BorderedWindow(std::shared_ptr<Window> window, BorderType border_type)
      : next_(std::move(window)), border_type_(border_type) {
    switch (border_type_) {
      case BorderType::None:
        hor_border_thic_ = 0;
        vert_border_thic_ = 0;
        break;
      case BorderType::Simple:
      case BorderType::Double:
        hor_border_thic_ = 2;
        vert_border_thic_ = 1;
        break;
    }
    next_->set_parent(this);
  }

  bool need_refresh() override {
    return Window::need_refresh() || next_->need_refresh();
  }
  void set_refreshed() override {
    Window::set_refreshed();
    next_->set_refreshed();
  }
  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    next_->resize(new_width - 2 * hor_border_thic_, new_height - 2 * vert_border_thic_);
    next_->set_parent_offset(vert_border_thic_, hor_border_thic_);
  }

  void render(WindowOutputter &rb, bool force) override;
  void handle_input(const InputEvent &info) override;

  td::int32 min_width() override {
    return 2 * hor_border_thic_ + next_->min_width();
  }
  td::int32 min_height() override {
    return 2 * vert_border_thic_ + next_->min_height();
  }
  td::int32 best_width() override {
    return 2 * hor_border_thic_ + next_->best_width();
  }
  td::int32 best_height() override {
    return 2 * vert_border_thic_ + next_->best_height();
  }

  void set_border_type(BorderType border_type, td::int32 color) {
    border_type_ = border_type;
    color_ = color;
    set_need_refresh();
  }

  auto border_type() const {
    return border_type_;
  }

 private:
  std::shared_ptr<Window> next_;

  BorderType border_type_;
  td::int32 vert_border_thic_;
  td::int32 hor_border_thic_;

  td::int32 color_{-1};
};

}  // namespace windows
