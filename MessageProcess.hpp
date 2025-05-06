#pragma once

#include "auto/td/telegram/td_api.h"
#include "auto/td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/common.h"
#include <string>
#include <vector>

namespace tdcurses {

const td::td_api::formattedText *message_get_formatted_text(const td::td_api::message &message);
td::tl_object_ptr<td::td_api::file> *message_get_file_object_ref(td::td_api::message &message);
const td::td_api::file *message_get_file(const td::td_api::message &message);

#define DEFAULT_CLONE_TL_OBJECT(T)                         \
  inline td::tl_object_ptr<T> clone_tl_object(const T &) { \
    return td::make_tl_object<T>();                        \
  }

td::tl_object_ptr<td::td_api::formattedText> clone_tl_object(const td::td_api::formattedText &);
td::tl_object_ptr<td::td_api::textEntity> clone_tl_object(const td::td_api::textEntity &);

DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeMention);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeHashtag);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeCashtag);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeBotCommand);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeUrl);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeEmailAddress);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypePhoneNumber);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeBankCardNumber);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeBold);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeItalic);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeUnderline);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeStrikethrough);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeSpoiler);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeCode);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypePre);
td::tl_object_ptr<td::td_api::textEntityTypePreCode> clone_tl_object(const td::td_api::textEntityTypePreCode &);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeBlockQuote);
DEFAULT_CLONE_TL_OBJECT(td::td_api::textEntityTypeExpandableBlockQuote);
td::tl_object_ptr<td::td_api::textEntityTypeTextUrl> clone_tl_object(const td::td_api::textEntityTypeTextUrl &);
td::tl_object_ptr<td::td_api::textEntityTypeMentionName> clone_tl_object(const td::td_api::textEntityTypeMentionName &);
td::tl_object_ptr<td::td_api::textEntityTypeCustomEmoji> clone_tl_object(const td::td_api::textEntityTypeCustomEmoji &);
td::tl_object_ptr<td::td_api::textEntityTypeMediaTimestamp> clone_tl_object(
    const td::td_api::textEntityTypeMediaTimestamp &);

template <class T>
std::enable_if_t<!std::is_constructible<T>::value && std::is_base_of<td::TlObject, T>::value, td::tl_object_ptr<T>>
clone_tl_object(const T &X) {
  td::tl_object_ptr<T> res;
  td::td_api::downcast_call(const_cast<T &>(X), [&](const auto &obj) { res = clone_tl_object(obj); });
  return res;
}

template <class T>
std::enable_if_t<std::is_base_of<td::TlObject, T>::value, td::tl_object_ptr<T>> clone_tl_object(const T *X) {
  return clone_tl_object(*X);
}

template <class T>
std::enable_if_t<std::is_base_of<td::TlObject, T>::value, td::tl_object_ptr<T>> clone_tl_object(
    const td::tl_object_ptr<T> &X) {
  return clone_tl_object(*X);
}

template <class T>
std::enable_if_t<std::is_base_of<td::TlObject, T>::value, std::vector<td::tl_object_ptr<T>>> clone_tl_object(
    const std::vector<td::tl_object_ptr<T>> &X) {
  std::vector<td::tl_object_ptr<T>> res;
  for (const auto &e : X) {
    res.push_back(clone_tl_object(e));
  }
  return res;
}

}  // namespace tdcurses
