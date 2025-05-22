#include "HelpWindow.hpp"

namespace tdcurses {

void generate_help_info(Outputter &out) {
  out << "Key bindings:\n";
  out << "  Global:\n";
  out << "    C-r               : force refresh\n";
  out << "    C-a + [hjkl]      : change active window\n";
  out << "    q/C-q/Esc         : close the popup window and open the previous one\n";
  out << "    Q/C-Q             : close all popup windows\n";
  out << "    F1                : show this help\n";
  out << "    F7                : open settings\n";
  out << "    F8                : open current user profile\n";
  out << "    F9                : open config\n";
  out << "    F12               : open debug info window\n";
  out << "  Dialog list:\n";
  out << "    I                 : open chat info window\n";
  out << "    s                 : search chat to open\n";
  out << "    :                 : activate command line\n";
  out << "    /                 : search chats\n";
  out << "    f                 : choose folder\n";
  out << "  Chat window:\n";
  out << "    Enter/I           : open message actions window\n";
  out << "    Escape/C-q/C-Q/q/Q: clear multiselection / clear search\n";
  out << "    i                 : compose a message\n";
  out << "    :                 : activate command line\n";
  out << "    /                 : search chats\n";
  out << "    F                 : quick send file\n";
  out << "    Space             : select message\n";
  out << "    v                 : view attachment\n";
  out << "    r                 : compose a reply\n";
  out << "    e                 : edit text message\n";
  out << "    L                 : open reactions window\n";
  out << "    y                 : copy message contents to primary buffer\n";
  out << "    Y                 : copy message contents to clipboard\n";
  out << "    f                 : forward messages\n";
  out << "    d                 : delete messages for all\n";
  out << "    D                 : delete messages for only for yourself\n";
  out << "  Compose window:\n";
  out << "    Alt-m             : toggle markdown parsing\n";
  out << "    Alt-s             : toggle no sound send\n";
  out << "    Alt-a             : add attachement\n";
}

}  // namespace tdcurses
