#pragma once

#include "windows/Output.hpp"

namespace tdcurses {

class ColorScheme {
 public:
  ColorScheme();
  ~ColorScheme();

  enum class Mode {
    DarkBackground,
    NormalBackground,
    BrightBackground,
    DarkForeground,
    NormalForeground,
    BrightForeground
  };

  windows::ColorRGB color_info(td::int32 id, Mode mode) {
    CHECK(id >= 0 && id <= 6);
    if (mode == Mode::DarkBackground) {
      static const std::vector<windows::ColorRGB> colors{windows::ColorRGB{0x440000}, windows::ColorRGB{0x662200},
                                                         windows::ColorRGB{0x220044}, windows::ColorRGB{0x004400},
                                                         windows::ColorRGB{0x004444}, windows::ColorRGB{0x000044},
                                                         windows::ColorRGB{0x44211f}};
      return colors[id];
    } else {
      static const std::vector<windows::ColorRGB> colors{windows::ColorRGB{0xff0000}, windows::ColorRGB{0xff6600},
                                                         windows::ColorRGB{0x7f00ff}, windows::ColorRGB{0x00ff00},
                                                         windows::ColorRGB{0x00ffff}, windows::ColorRGB{0x0000ff},
                                                         windows::ColorRGB{0xf88379}};
      return colors[id];
    }
  }
};

ColorScheme &color_scheme();

}  // namespace tdcurses
