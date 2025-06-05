#include "StickerManager.hpp"

namespace tdcurses {

StickerManager &sticker_manager() {
  static StickerManager sticker_manager;
  return sticker_manager;
}

}  // namespace tdcurses
