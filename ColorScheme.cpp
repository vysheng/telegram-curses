#include "ColorScheme.hpp"

namespace tdcurses {

ColorScheme::ColorScheme() {
}
ColorScheme::~ColorScheme() {
}

ColorScheme &color_scheme() {
  static ColorScheme color_scheme;
  return color_scheme;
}

}  // namespace tdcurses
