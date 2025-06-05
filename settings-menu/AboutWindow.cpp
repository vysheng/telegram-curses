#include "AboutWindow.hpp"
#include "Outputter.hpp"
#include "managers/GlobalParameters.hpp"
#include "rlottie/inc/rlottie.h"

#include <notcurses/notcurses.h>
#include <libconfig.h++>
#if USE_LIBTICKIT
#include "tickit.h"
#endif

namespace tdcurses {

void AboutWindow::build() {
  Outputter out;
  out << Color::Red << "Tdcurses" << Color::Revert << " version " << global_parameters().version() << "\n\n";
  out << "Tdcurses is an " << Outputter::Bold{true} << "UNOFFICIAL" << Outputter::Bold{false} << " Telegram client\n";
  out << "Tdcurses is distributed under Boost Software License, which can be downloaded here:\n  "
      << Outputter::Underline{true} << "https://www.boost.org/LICENSE_1_0.txt" << Outputter::Underline{false} << "\n";
  out << "\n";
  out << "Tdcurses uses, among others, the following libraries:\n";
  out << "    Tdlib version " << global_parameters().tdlib_version() << " " << Outputter::Underline{true}
      << "https://github.com/tdlib/td" << Outputter::Underline{false} << "\n";
  out << "    Notcurses version " << notcurses_version() << " " << Outputter::Underline{true}
      << "https://github.com/dankamongmen/notcurses" << Outputter::Underline{false} << "\n";
#if USE_LIBTICKIT
  out << "    Libtickit version " << TICKIT_VERSION_MAJOR << "." << TICKIT_VERSION_MINOR << "." << TICKIT_VERSION_PATCH
      << " " << Outputter::Underline{true} << "https://github.com/leonerd/libtickit" << Outputter::Underline{false}
      << "\n";
#endif
  out << "    Libconfig version " << LIBCONFIGXX_VER_MAJOR << "." << LIBCONFIGXX_VER_MINOR << "."
      << LIBCONFIGXX_VER_REVISION << " " << Outputter::Underline{true} << "https://github.com/hyperrealm/libconfig"
      << Outputter::Underline{false} << "\n";
  out << "    QR-Code-generator " << Outputter::Underline{true} << "https://github.com/nayuki/QR-Code-generator"
      << Outputter::Underline{false} << "\n";
  out << "    Rlottie " << Outputter::Underline{true} << "https://github.com/TelegramMessenger/rlottie"
      << Outputter::Underline{false} << "\n";
  replace_text(out.as_str(), out.markup());
}

}  // namespace tdcurses
