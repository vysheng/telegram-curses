#include "BorderedWindow.hpp"
#include <vector>

namespace windows {

static const std::vector<std::string> border_none{" ", " ", " ", " ", " ", " ", " ", " ", " ", " "};
static const std::vector<std::string> border_single{"│", "└", "┴", "┘", "├", "┼", "┤", "┌", "┬", "┐", "─"};
static const std::vector<std::string> border_single_thic{"┃", "┗", "┻", "┛", "┣", "╋", "┫", "┏", "┳", "┓", "━"};
static const std::vector<std::string> border_double{"║", "╚", "╩", "╝", "╠", "╬", "╣", "╔", "╦", "╗", "═"};

const std::vector<std::string> &get_border_type_info(BorderType border_type, bool is_active) {
  switch (border_type) {
    case BorderType::None:
      return border_none;
    case BorderType::Simple:
      if (is_active) {
        return border_single_thic;
      } else {
        return border_single;
      }
      break;
    case BorderType::Double:
      return border_double;
    default:
      return border_none;
  }
}

void BorderedWindow::render(WindowOutputter &rb, bool force) {
  if (!force && !next_->need_refresh() && !need_refresh()) {
    return;
  }

  render_subwindow(rb, next_.get(), force, true, true);

  if (vert_border_thic_ != 0) {
    if (color_ != -1) {
      rb.set_fg_color((Color)color_);
    }
    const auto &border = get_border_type_info(border_type_, is_active());
    rb.fill_yx(0, 0, border[7], 1);
    rb.fill_yx(0, 1, border[10], width() - 2);
    rb.fill_yx(0, width() - 1, border[9], 1);
    rb.fill_vert_yx(1, 0, border[0], height() - 2);
    rb.fill_vert_yx(1, width() - 1, border[0], height() - 2);
    rb.fill_vert_yx(1, 1, ' ', height() - 2);
    rb.fill_vert_yx(1, width() - 2, ' ', height() - 2);
    rb.fill_yx(height() - 1, 0, border[1], 1);
    rb.fill_yx(height() - 1, 1, border[10], width() - 2);
    rb.fill_yx(height() - 1, width() - 1, border[3], 1);
  }
}

void BorderedWindow::handle_input(const InputEvent &info) {
  next_->handle_input(info);
}

}  // namespace windows
