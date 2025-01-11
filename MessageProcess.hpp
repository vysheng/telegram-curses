#pragma once

#include "auto/td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/common.h"
#include <string>

namespace tdcurses {

const td::td_api::formattedText *message_get_formatted_text(const td::td_api::message &message);
td::tl_object_ptr<td::td_api::file> *message_get_file_object_ref(td::td_api::message &message);
const td::td_api::file *message_get_file(const td::td_api::message &message);

}  // namespace tdcurses
