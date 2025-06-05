#include "ChatManager.hpp"

namespace tdcurses {

ChatManager &chat_manager() {
  static ChatManager chat_manager;
  return chat_manager;
}

}  // namespace tdcurses
