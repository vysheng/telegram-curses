#pragma once

#include "td/utils/Slice-decl.h"
#include "td/utils/Slice.h"
#include <memory>

namespace windows {

class Window;

enum class Color : td::int32 {
  Revert = -1,
  Black = 0,
  Maroon,
  Green,
  Olive,
  Navy,
  Purple,
  Teal,
  Silver,
  Grey,
  Red,
  Lime,
  Yellow,
  Blue,
  Fuchsia,
  Aqua,
  White
};

class RenderedImage {
 public:
  virtual ~RenderedImage() = default;
  virtual td::int32 rendered_to_width() = 0;
  virtual td::int32 width() = 0;
  virtual td::int32 height() = 0;
};

class WindowOutputter {
 public:
  enum class CursorShape { None, Underscore, Block, LeftBar };
  enum class Mode { Tickit, Notcurses };
  WindowOutputter() {
  }
  virtual ~WindowOutputter() = default;
  virtual td::int32 putstr_yx(td::int32 y, td::int32 x, const char *s, size_t len = 0) = 0;
  td::int32 putstr_yx(td::int32 y, td::int32 x, td::CSlice s) {
    return putstr_yx(y, x, s.c_str(), s.size());
  }
  void erase_yx(td::int32 y, td::int32 x, td::int32 size) {
    if (!is_real()) {
      return;
    }
    fill_yx(y, x, ' ', size);
  }
  void erase_vert_yx(td::int32 y, td::int32 x, td::int32 size) {
    if (!is_real()) {
      return;
    }
    fill_vert_yx(y, x, ' ', size);
  }
  void fill_yx(td::int32 y, td::int32 x, char c, td::int32 size) {
    if (!is_real()) {
      return;
    }
    char s[2];
    s[0] = c;
    s[1] = 0;
    for (td::int32 i = 0; i < size; i++) {
      putstr_yx(y, x + i, s, 1);
    }
  }
  void fill_yx(td::int32 y, td::int32 x, td::CSlice value, td::int32 size) {
    if (!is_real()) {
      return;
    }
    for (td::int32 i = 0; i < size; i++) {
      putstr_yx(y, x + i, value);
    }
  }
  void fill_vert_yx(td::int32 y, td::int32 x, char c, td::int32 size) {
    if (!is_real()) {
      return;
    }
    char s[2];
    s[0] = c;
    s[1] = 0;
    for (td::int32 i = 0; i < size; i++) {
      putstr_yx(y + i, x, s, 1);
    }
  }
  void fill_vert_yx(td::int32 y, td::int32 x, td::CSlice value, td::int32 size) {
    if (!is_real()) {
      return;
    }
    for (td::int32 i = 0; i < size; i++) {
      putstr_yx(y + i, x, value);
    }
  }
  virtual void fill_rect(td::int32 y, td::int32 x, td::int32 height, td::int32 width, td::CSlice value) {
    if (!is_real()) {
      return;
    }
    for (td::int32 i = 0; i < height; i++) {
      fill_yx(y + i, x, value, width);
    }
  }
  virtual void erase_rect(td::int32 y, td::int32 x, td::int32 height, td::int32 width) {
    if (!is_real()) {
      return;
    }
    for (td::int32 i = 0; i < height; i++) {
      erase_yx(y + i, x, width);
    }
  }
  virtual void transparent_rect(td::int32 y, td::int32 x, td::int32 height, td::int32 width) {
    if (!is_real()) {
      return;
    }
    erase_rect(y, x, height, width);
  }
  virtual void cursor_move_yx(td::int32 y, td::int32 x, CursorShape cursor_shape) = 0;
  virtual void set_fg_color(Color color) = 0;
  virtual void unset_fg_color() = 0;
  virtual void set_bg_color(Color color) = 0;
  virtual void unset_bg_color() = 0;
  virtual void set_bold(bool value) = 0;
  virtual void unset_bold() = 0;
  virtual void set_underline(bool value) = 0;
  virtual void unset_underline() = 0;
  virtual void set_italic(bool value) = 0;
  virtual void unset_italic() = 0;
  virtual void set_reverse(bool value) = 0;
  virtual void unset_reverse() = 0;
  virtual void set_strike(bool value) = 0;
  virtual void unset_strike() = 0;
  virtual void set_blink(bool value) = 0;
  virtual void unset_blink() = 0;
  virtual bool is_real() const = 0;
  virtual td::int32 local_cursor_y() const = 0;
  virtual td::int32 local_cursor_x() const = 0;
  virtual td::int32 global_cursor_y() const = 0;
  virtual td::int32 global_cursor_x() const = 0;
  virtual CursorShape cursor_shape() const = 0;
  virtual void translate(td::int32 delta_y, td::int32 delta_x) = 0;
  virtual void untranslate(td::int32 delta_y, td::int32 delta_x) {
    translate(-delta_y, -delta_x);
  }

  virtual std::unique_ptr<WindowOutputter> create_subwindow_outputter(td::int32 y_offset, td::int32 x_offset,
                                                                      td::int32 height, td::int32 width,
                                                                      bool is_active) = 0;
  virtual void update_cursor_position_from(WindowOutputter &from) = 0;
  virtual bool is_active() const = 0;
  virtual bool allow_render_image() {
    return false;
  }
  virtual td::int32 rendered_image_height(td::int32 max_height, td::int32 max_width, std::string path) {
    return 0;
  }
  virtual std::unique_ptr<RenderedImage> render_image(td::int32 max_height, td::int32 max_width, std::string path) {
    return nullptr;
  }
  virtual void draw_rendered_image(td::int32 y, td::int32 x, RenderedImage &image) {
  }
  virtual void hide_rendered_image(RenderedImage &image) {
  }
};

void set_empty_window_outputter(std::unique_ptr<WindowOutputter> out);
void create_empty_window_outputter_notcurses(void *notcurses, void *baseplane, void *renderplane);
void create_empty_window_outputter_libtickit();

WindowOutputter &empty_window_outputter();
std::unique_ptr<WindowOutputter> tickit_window_outputter(void *rb, td::int32 height, td::int32 width);
std::unique_ptr<WindowOutputter> notcurses_window_outputter(void *notcurses, void *baseplane, void *renderplane,
                                                            td::int32 height, td::int32 width);

using SavedRenderedImages = std::vector<std::pair<std::string, std::vector<std::unique_ptr<RenderedImage>>>>;
}  // namespace windows
