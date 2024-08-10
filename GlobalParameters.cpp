#include "GlobalParameters.hpp"

namespace tdcurses {

GlobalParameters &global_parameters() {
  static GlobalParameters ptr;
  return ptr;
}

}  // namespace tdcurses
