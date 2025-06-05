#pragma once

#include "auto/td/telegram/td_api.h"
#include "td/utils/Slice.h"
#include "td/utils/common.h"
#include "td/utils/logging.h"

#include <map>

namespace tdcurses {

class StickerManager {
 public:
  void process_update(td::td_api::updateStickerSet &update) {
    auto id = update.sticker_set_->id_;
    sticker_sets_[id] = std::move(update.sticker_set_);

    process_sticker_set(*sticker_sets_[id]);
  }

  void process_update(td::td_api::updateInstalledStickerSets &update) {
  }

  void process_update(td::td_api::updateTrendingStickerSets &update) {
  }

  void process_update(td::td_api::updateRecentStickers &update) {
    recent_stickers_ = std::move(update.sticker_ids_);
  }

  void process_update(td::td_api::updateFavoriteStickers &update) {
    favorite_stickers_ = std::move(update.sticker_ids_);
  }
  void process_custom_emoji_stickers(const td::td_api::stickers &stickers) {
    for (const auto &sticker : stickers.stickers_) {
      extract_custom_emoji_from_sticker(*sticker);
    }
  }
  td::Slice get_custom_emoji(td::int64 id) const {
    auto it = custom_emojis_.find(id);
    if (it != custom_emojis_.end()) {
      return it->second;
    } else {
      return td::Slice();
    }
  }

 private:
  void process_sticker_set(const td::td_api::stickerSet &sticker_set) {
    extract_custom_emojis_from_sticker_set(sticker_set);
  }

  void extract_custom_emojis_from_sticker_set(const td::td_api::stickerSet &sticker_set) {
    if (!sticker_set.is_allowed_as_chat_emoji_status_) {
      return;
    }
    for (const auto &sticker : sticker_set.stickers_) {
      extract_custom_emoji_from_sticker(*sticker);
    }
  }

  void extract_custom_emoji_from_sticker(const td::td_api::sticker &sticker) {
    if (sticker.full_type_->get_id() == td::td_api::stickerFullTypeCustomEmoji::ID) {
      const auto &t = static_cast<const td::td_api::stickerFullTypeCustomEmoji &>(*sticker.full_type_);
      custom_emojis_[t.custom_emoji_id_] = sticker.emoji_;
    }
  }

 private:
  std::map<td::int64, td::tl_object_ptr<td::td_api::stickerSet>> sticker_sets_;
  std::vector<td::int32> recent_stickers_;
  std::vector<td::int32> favorite_stickers_;

  std::map<td::int64, std::string> custom_emojis_;
};

StickerManager &sticker_manager();

}  // namespace tdcurses
