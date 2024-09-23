#pragma once

#include "auto/td/telegram/td_api.h"
#include "td/utils/common.h"
#include <string>

namespace tdcurses {

const td::td_api::formattedText *message_get_formatted_text(const td::td_api::message &message);
const td::td_api::file *message_get_file(const td::td_api::message &message);

}  // namespace tdcurses
