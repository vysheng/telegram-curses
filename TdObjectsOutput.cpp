#include "td/generate/auto/td/telegram/td_api.h"
#include "td/generate/auto/td/telegram/td_api.hpp"

#include "td/telegram/DownloadManager.h"
#include "td/utils/Slice-decl.h"
#include "td/utils/format.h"
#include "td/utils/logging.h"
#include "td/utils/overloaded.h"
#include "td/utils/port/Stat.h"
#include "td/utils/utf8.h"

#include "TdObjectsOutput.h"
#include "StickerManager.hpp"

#include "utf8proc.h"

#include <ctime>
#include <memory>
#include <vector>
#include <algorithm>

namespace tdcurses {

Outputter &operator<<(Outputter &out, const td::td_api::message &message) {
  auto start_pos = out.as_cslice().size();
  auto C = chat_manager().get_chat(message.chat_id_);
  if (message.is_outgoing_) {
    out << Color::Blue;
  } else {
    out << Color::Green;
  }
  out << Outputter::Date{message.date_} << " ";

  if (C->chat_type() == ChatType::SecretChat || C->chat_type() == ChatType::User) {
    // ???
  } else {
    out << message.sender_id_ << " ";
  }

  if (message.is_outgoing_) {
    if (C->last_read_outbox_message_id() >= message.id_) {
      out << "\xe2\x9c\x93\xe2\x9c\x93 ";
    } else {
      out << " \xe2\x9c\x94 ";
    }
  } else {
    out << "   ";
    /*if (C->last_read_outbox_message_id() >= message.id_) {
      out << ">>> ";
    } else {
      out << "--> ";
    }*/
  }

  if (message.forward_info_) {
    out << "fwd " << message.forward_info_ << "\n";
  }

  if (message.via_bot_user_id_) {
    auto user = chat_manager().get_user(message.via_bot_user_id_);
    if (user) {
      out << "via bot " << Outputter::FgColor{Color::Red} << user << Outputter::FgColor{Color::Revert} << "\n";
    }
  }

  if (message.reply_to_) {
    td::td_api::downcast_call(const_cast<td::td_api::MessageReplyTo &>(*message.reply_to_),
                              td::overloaded(
                                  [&](const td::td_api::messageReplyToMessage &r) {
                                    out << "reply " << Outputter::NoLb{true};
                                    auto reply_msg = out.get_message(r.chat_id_, r.message_id_);
                                    if (reply_msg) {
                                      out << Color::Red << reply_msg->sender_id_ << Color::Revert << " "
                                          << reply_msg->content_;
                                    }
                                    out << Outputter::NoLb{false} << "\n";
                                  },
                                  [&](const td::td_api::messageReplyToStory &r) {}));
  }

  out << Color::Revert;

  out << message.content_;

  if (message.interaction_info_ &&
      (message.interaction_info_->view_count_ || message.interaction_info_->forward_count_ ||
       message.interaction_info_->reactions_)) {
    if (out.as_cslice().size() - start_pos < 100 && out.as_cslice().find('\n') == td::CSlice::npos) {
      out << " ";
    } else {
      out << "\n";
    }
    if (message.interaction_info_->view_count_ > 0) {
      out << "[" << message.interaction_info_->view_count_ << "👁]";
    }
    if (message.interaction_info_->forward_count_ > 0) {
      out << "[" << message.interaction_info_->forward_count_ << "⏭]";
    }
    if (message.interaction_info_->reactions_) {
      for (auto &r : message.interaction_info_->reactions_->reactions_) {
        if (r->is_chosen_) {
          out << Outputter::FgColor(Color::Yellow);
        }
        out << "[" << r->total_count_ << r->type_ << "]";
        if (r->is_chosen_) {
          out << Outputter::FgColor(Color::Revert);
        }
      }
    }
    out << " ";
  }

  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForwardInfo &fwd_info) {
  return out << Outputter::FgColor{Color::Red} << fwd_info.origin_ << Outputter::FgColor{Color::Revert} << " "
             << Outputter::Date{fwd_info.date_};
}

Outputter &operator<<(Outputter &out, const td::td_api::messageOriginUser &fwd_info) {
  auto U = chat_manager().get_user(fwd_info.sender_user_id_);
  return out << U;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageOriginChannel &fwd_info) {
  auto C = chat_manager().get_chat(fwd_info.chat_id_);
  return out << C;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageOriginChat &fwd_info) {
  auto C = chat_manager().get_chat(fwd_info.sender_chat_id_);
  return out << C;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageOriginHiddenUser &fwd_info) {
  return out << Color::Red << fwd_info.sender_name_ << Color::Revert;
}

/*Outputter &operator<<(Outputter &out, const td::td_api::messageForwardOriginMessageImport &fwd_info) {
  return out << Color::Red << fwd_info.sender_name_ << Color::Revert;
}*/

Outputter &operator<<(Outputter &out, const td::td_api::messageText &content) {
  out << content.text_;
  if (content.link_preview_) {
    out << " [" << content.link_preview_ << "]";
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageAnimation &content) {
  out << "[" << content.animation_ << "]";
  if (content.caption_) {
    out << " " << content.caption_;
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageAudio &content) {
  out << "[" << content.audio_ << "]";
  if (content.caption_) {
    out << " " << content.caption_;
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageDocument &content) {
  out << "[" << content.document_ << "]";
  if (content.caption_) {
    out << " " << content.caption_;
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePaidMedia &content) {
  out << "[paid media]";
  if (content.caption_) {
    out << " " << content.caption_;
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePhoto &content) {
  out << "[" << content.photo_ << "]";
  if (content.caption_) {
    out << " " << content.caption_;
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageSticker &content) {
  return out << "[" << content.sticker_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageVideo &content) {
  out << "[" << content.video_ << "]";
  if (content.caption_) {
    out << " " << content.caption_;
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageVideoNote &content) {
  return out << "[" << content.video_note_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageVoiceNote &content) {
  return out << "[" << content.voice_note_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageExpiredPhoto &content) {
  return out << "[photo expired]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageExpiredVideo &content) {
  return out << "[video expired]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageExpiredVideoNote &content) {
  return out << "[video note expired]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageExpiredVoiceNote &content) {
  return out << "[voice note expired]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageLocation &content) {
  return out << "[" << content.location_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageVenue &content) {
  return out << "[" << content.venue_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageContact &content) {
  return out << "[" << content.contact_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageAnimatedEmoji &content) {
  return out << "[" << content.emoji_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageDice &content) {
  return out << "[dice " << content.value_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageGame &content) {
  return out << "[" << content.game_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePoll &content) {
  return out << "[" << content.poll_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageStory &content) {
  return out << "[story " << content.story_id_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageInvoice &content) {
  return out << "[invoice]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageCall &content) {
  return out << "[call " << td::format::as_time(content.duration_) << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageVideoChatScheduled &content) {
  return out << "[videochat " << content.group_call_id_ << " scheduled to " << content.start_date_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageVideoChatStarted &content) {
  return out << "[videochat " << content.group_call_id_ << " started]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageVideoChatEnded &content) {
  return out << "[videochat ended, duration " << td::format::as_time(content.duration_) << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageInviteVideoChatParticipants &content) {
  return out << "[videochat " << content.group_call_id_ << ": invited " << content.user_ids_.size() << " users]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageBasicGroupChatCreate &content) {
  return out << "[created]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageSupergroupChatCreate &content) {
  return out << "[created]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatChangeTitle &content) {
  return out << "[renamed]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatChangePhoto &content) {
  return out << "[changed photo]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatDeletePhoto &content) {
  return out << "[deleted photo]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatAddMembers &content) {
  return out << "[added members]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatJoinByLink &content) {
  return out << "[joined by link]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatJoinByRequest &content) {
  return out << "[joined by request]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatDeleteMember &content) {
  return out << "[kicked member]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatUpgradeTo &content) {
  return out << "[upgraded]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatUpgradeFrom &content) {
  return out << "[upgraded]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePinMessage &content) {
  return out << "[pinned]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageScreenshotTaken &content) {
  return out << "[screenshot]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatSetBackground &content) {
  return out << "[set background]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatSetTheme &content) {
  return out << "[set theme " << content.theme_name_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatSetMessageAutoDeleteTime &content) {
  return out << "[ttl " << td::format::as_time(content.message_auto_delete_time_) << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatBoost &content) {
  return out << "[boost " << content.boost_count_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForumTopicCreated &content) {
  return out << "[topic " << content.name_ << " created]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForumTopicEdited &content) {
  return out << "[topic " << content.name_ << " edited]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForumTopicIsClosedToggled &content) {
  return out << "[topic " << (content.is_closed_ ? "closed" : "opened") << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForumTopicIsHiddenToggled &content) {
  return out << "[topic " << (content.is_hidden_ ? "hidden" : "shown") << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageSuggestProfilePhoto &content) {
  return out << "[profile photo suggested " << content.photo_->animation_->file_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageCustomServiceAction &content) {
  return out << "[service " << content.text_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageGameScore &content) {
  return out << "[gamescore]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePaymentSuccessful &content) {
  return out << "[payment success]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePaymentSuccessfulBot &content) {
  return out << "[payment success]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePaymentRefunded &content) {
  return out << "[payment refunded]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageGiftedPremium &content) {
  return out << "[gifted premium]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePremiumGiftCode &content) {
  return out << "[gift code]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePremiumGiveawayCreated &content) {
  return out << "[givaway created]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePremiumGiveaway &content) {
  return out << "[givaway]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePremiumGiveawayCompleted &content) {
  return out << "[givaway completed]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePremiumGiveawayWinners &content) {
  return out << "[givaway winners]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageGiftedStars &content) {
  return out << "[gifted " << content.amount_ << " stars ]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageContactRegistered &content) {
  return out << "[registered]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageUsersShared &content) {
  return out << "[shared users]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatShared &content) {
  return out << "[shared chat]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageBotWriteAccessAllowed &content) {
  return out << "[bot write access allowed]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageWebAppDataSent &content) {
  return out << "[webapp data sent]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageWebAppDataReceived &content) {
  return out << "[webapp data received]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePassportDataSent &content) {
  return out << "[passportdatasent]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messagePassportDataReceived &content) {
  return out << "[passportdatareceived]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageProximityAlertTriggered &content) {
  return out << "[near " << content.traveler_id_ << ", distance " << content.distance_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageUnsupported &content) {
  return out << "[unsupported]";
}

/*Outputter &operator<<(Outputter &out, const td::td_api::messageUserShared &content) {
  return out << "[user shared]";
}*/

/*Outputter &operator<<(Outputter &out, const td::td_api::messageWebsiteConnected &content) {
  return out << "[website " << content.domain_name_ << " connected]";
}*/

Outputter &operator<<(Outputter &out, const td::td_api::formattedText &content) {
  auto enable_disable_markup = [&](const td::td_api::TextEntityType *ent, bool enable) {
    td::td_api::downcast_call(
        const_cast<td::td_api::TextEntityType &>(*ent),
        td::overloaded(
            [&](const td::td_api::textEntityTypeMention &) { out << (enable ? Color::Red : Color::Revert); },
            [&](const td::td_api::textEntityTypeHashtag &) {}, [&](const td::td_api::textEntityTypeCashtag &) {},
            [&](const td::td_api::textEntityTypeBotCommand &) {},
            [&](const td::td_api::textEntityTypeUrl &) { out << Outputter::Underline(enable); },
            [&](const td::td_api::textEntityTypeEmailAddress &) { out << Outputter::Underline(enable); },
            [&](const td::td_api::textEntityTypePhoneNumber &) { out << Outputter::Underline(enable); },
            [&](const td::td_api::textEntityTypeBankCardNumber &) { out << Outputter::Underline(enable); },
            [&](const td::td_api::textEntityTypeBold &) { out << Outputter::Bold(enable); },
            [&](const td::td_api::textEntityTypeItalic &) { out << Outputter::Italic(enable); },
            [&](const td::td_api::textEntityTypeUnderline &) { out << Outputter::Underline(enable); },
            [&](const td::td_api::textEntityTypeStrikethrough &) { out << Outputter::Strike(enable); },
            [&](const td::td_api::textEntityTypeSpoiler &) {}, [&](const td::td_api::textEntityTypeCode &) {},
            [&](const td::td_api::textEntityTypePre &) {}, [&](const td::td_api::textEntityTypePreCode &) {},
            [&](const td::td_api::textEntityTypeBlockQuote &e) {},
            [&](const td::td_api::textEntityTypeExpandableBlockQuote &) {},
            [&](const td::td_api::textEntityTypeTextUrl &e) {
              out << Outputter::Underline(enable);
              if (!enable) {
                out << "[" << e.url_ << "]";
              }
            },
            [&](const td::td_api::textEntityTypeMentionName &) { out << (enable ? Color::Red : Color::Revert); },
            [&](const td::td_api::textEntityTypeCustomEmoji &) {},
            [&](const td::td_api::textEntityTypeMediaTimestamp &) {}));
  };
  std::vector<std::pair<size_t, const td::td_api::TextEntityType *>> en;
  std::vector<std::pair<size_t, const td::td_api::TextEntityType *>> dis;
  for (auto &m : content.entities_) {
    if (m->length_ > 0) {
      en.emplace_back(m->offset_, m->type_.get());
      dis.emplace_back(m->offset_ + m->length_, m->type_.get());
    }
  }
  size_t enp = 0, disp = 0;
  std::sort(en.begin(), en.end());
  std::sort(dis.begin(), dis.end());
  const unsigned char *text = td::CSlice(content.text_).ubegin();
  size_t p = 0;
  while (*text) {
    while (enp < en.size() && en[enp].first <= p) {
      enable_disable_markup(en[enp].second, true);
      enp++;
    }
    while (disp < dis.size() && dis[disp].first <= p) {
      enable_disable_markup(dis[disp].second, false);
      disp++;
    }
    td::uint32 code;
    auto new_text = td::next_utf8_unsafe(text, &code);
    out << td::Slice(text, new_text);
    text = new_text;
    p += (code <= 0xffff) ? 1 : 2;  // UTF16 codepoints =((
  }
  while (enp < en.size()) {
    enable_disable_markup(en[enp].second, true);
    enp++;
  }
  while (disp < dis.size()) {
    enable_disable_markup(dis[disp].second, false);
    disp++;
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::linkPreview &content) {
  out << content.url_;
  if (content.description_) {
    out << " " << content.description_;
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::webPageInstantView &content) {
  out << "instant view";
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::animation &content) {
  return out << "animation " << content.animation_;
}

Outputter &operator<<(Outputter &out, const td::td_api::audio &content) {
  return out << "audio " << content.audio_;
}

Outputter &operator<<(Outputter &out, const td::td_api::document &content) {
  return out << "document " << content.document_;
}

Outputter &operator<<(Outputter &out, const td::td_api::photo &content) {
  if (content.sizes_.size() == 0) {
    return out << "photo {no file}";
  } else {
    return out << "photo " << (*content.sizes_.rbegin())->photo_;
  }
}

Outputter &operator<<(Outputter &out, const td::td_api::sticker &content) {
  return out << "sticker " << content.emoji_ << " " << content.sticker_;
}

Outputter &operator<<(Outputter &out, const td::td_api::video &content) {
  return out << "video " << content.video_;
}

Outputter &operator<<(Outputter &out, const td::td_api::videoNote &content) {
  return out << "videonote " << content.video_;
}

Outputter &operator<<(Outputter &out, const td::td_api::voiceNote &content) {
  return out << "voicenote " << content.voice_;
}

Outputter &operator<<(Outputter &out, const td::td_api::location &content) {
  return out << "location [" << content.longitude_ << "," << content.latitude_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::venue &content) {
  return out << "venue";
}

Outputter &operator<<(Outputter &out, const td::td_api::contact &content) {
  return out << "contact " << content.first_name_ << " " << content.last_name_ << " " << content.phone_number_;
}

Outputter &operator<<(Outputter &out, const td::td_api::game &content) {
  return out << "game";
}

Outputter &operator<<(Outputter &out, const td::td_api::poll &content) {
  out << "poll " << content.question_ << "\n";
  for (auto &opt : content.options_) {
    if (opt->is_being_chosen_ || opt->is_chosen_) {
      out << Color::Yellow;
    }
    out << "\t" << opt->vote_percentage_ << "% " << opt->text_ << "\n";
    if (opt->is_being_chosen_ || opt->is_chosen_) {
      out << Color::Revert;
    }
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::file &file) {
  out << file.id_ << " " << td::format::as_size(file.size_);
  if (file.local_) {
    if (file.local_->is_downloading_completed_) {
      out << " " << file.local_->path_;
    }
    if (file.local_->is_downloading_active_) {
      auto v = file.local_->downloaded_size_ * 100 / (file.size_ ?: 1);
      out << " " << v << "%";
    }
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageSenderChat &sender) {
  auto C = chat_manager().get_chat(sender.chat_id_);
  return out << " " << C;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageSenderUser &sender) {
  auto C = chat_manager().get_user(sender.user_id_);
  return out << " " << C;
}

Outputter &operator<<(Outputter &out, const std::shared_ptr<Chat> &chat) {
  if (chat) {
    return out << chat->title();
  } else {
    return out << "(unknown)";
  }
}

Outputter &operator<<(Outputter &out, const std::shared_ptr<User> &chat) {
  if (chat) {
    if (chat->first_name().size() > 0) {
      out << chat->first_name();
      if (chat->last_name().size() > 0) {
        return out << " " << chat->last_name();
      } else {
        return out;
      }
    } else if (chat->last_name().size() > 0) {
      return out << chat->last_name();
    } else {
      return out << "(empty)";
    }
  } else {
    return out << "(unknown)";
  }
}

Outputter &operator<<(Outputter &out, const td::td_api::reactionTypeEmoji &e) {
  LOG(ERROR) << "EMOJI!" << e.emoji_;
  return out << e.emoji_;
}

Outputter &operator<<(Outputter &out, const td::td_api::reactionTypeCustomEmoji &e) {
  auto x = sticker_manager().get_custom_emoji(e.custom_emoji_id_);
  if (x.size() > 0) {
    return out << x;
  } else {
    return out << "?";
  }
}

Outputter &operator<<(Outputter &out, const td::td_api::reactionTypePaid &e) {
  return out << "☆";
}

Outputter &operator<<(Outputter &out, const td::td_api::chatPhoto &file) {
  return out << "chat photo";
}

}  // namespace tdcurses
