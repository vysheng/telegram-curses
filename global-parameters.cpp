#include "global-parameters.h"

namespace tdcurses {

GlobalParameters &global_parameters() {
  static GlobalParameters ptr;
  return ptr;
}

}  // namespace tdcurses
