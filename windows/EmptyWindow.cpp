#include "EmptyWindow.hpp"

namespace windows {

void EmptyWindow::render(WindowOutputter &rb, bool force) {
  rb.cursor_move_yx(0, 0, WindowOutputter::CursorShape::None);
  rb.erase_rect(0, 0, height() - 1, width() - 1);
}

}  // namespace windows
