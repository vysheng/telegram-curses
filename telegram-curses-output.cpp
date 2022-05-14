#include "td/generate/auto/td/telegram/td_api.h"
#include "td/generate/auto/td/telegram/td_api.hpp"

#include "td/utils/format.h"

#include "telegram-curses-output.h"

#include <ctime>
#include <memory>

namespace tdcurses {

Outputter &operator<<(Outputter &out, const td::td_api::message &message) {
  auto C = ChatManager::instance->get_chat(message.chat_id_);
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
    out << "[fwd " << message.forward_info_ << "]\n";
  }

  out << Color::Revert;

  return out << message.content_;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForwardInfo &fwd_info) {
  return out << fwd_info.origin_ << " " << Outputter::Date{fwd_info.date_};
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForwardOriginUser &fwd_info) {
  auto U = ChatManager::instance->get_user(fwd_info.sender_user_id_);
  return out << U;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForwardOriginChannel &fwd_info) {
  auto C = ChatManager::instance->get_chat(fwd_info.chat_id_);
  return out << C;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForwardOriginChat &fwd_info) {
  auto C = ChatManager::instance->get_chat(fwd_info.sender_chat_id_);
  return out << C;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForwardOriginHiddenUser &fwd_info) {
  return out << Color::Red << fwd_info.sender_name_ << Color::Revert;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageForwardOriginMessageImport &fwd_info) {
  return out << Color::Red << fwd_info.sender_name_ << Color::Revert;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageText &content) {
  out << content.text_;
  if (content.web_page_) {
    out << " [" << content.web_page_ << "]";
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

Outputter &operator<<(Outputter &out, const td::td_api::messagePhoto &content) {
  out << "[" << content.photo_ << "]";
  if (content.caption_) {
    out << " " << content.caption_;
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageExpiredPhoto &content) {
  return out << "[photo expired]";
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

Outputter &operator<<(Outputter &out, const td::td_api::messageExpiredVideo &content) {
  return out << "[video expired]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageVideoNote &content) {
  return out << "[" << content.video_note_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageVoiceNote &content) {
  return out << "[" << content.voice_note_ << "]";
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
  return out << "[joined by request approval]";
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

Outputter &operator<<(Outputter &out, const td::td_api::messageChatSetTheme &content) {
  return out << "[set theme " << content.theme_name_ << "]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageChatSetTtl &content) {
  return out << "[ttl " << td::format::as_time(content.ttl_) << "]";
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

Outputter &operator<<(Outputter &out, const td::td_api::messageContactRegistered &content) {
  return out << "[registered]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageWebsiteConnected &content) {
  return out << "[website " << content.domain_name_ << " connected]";
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

Outputter &operator<<(Outputter &out, const td::td_api::messageWebAppDataSent &content) {
  return out << "[webapp data sent]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageWebAppDataReceived &content) {
  return out << "[webapp datar received]";
}

Outputter &operator<<(Outputter &out, const td::td_api::messageUnsupported &content) {
  return out << "[unsupported]";
}

Outputter &operator<<(Outputter &out, const td::td_api::formattedText &content) {
  return out << content.text_;
}

Outputter &operator<<(Outputter &out, const td::td_api::webPage &content) {
  out << content.embed_url_;
  if (content.description_) {
    out << " " << content.description_;
  }
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
    out << "\t" << opt->vote_percentage_ << "% " << opt->text_ << "\n";
  }
  return out;
}

Outputter &operator<<(Outputter &out, const td::td_api::file &file) {
  return out << file.id_ << " " << td::format::as_size(file.size_);
}

Outputter &operator<<(Outputter &out, const td::td_api::messageSenderChat &sender) {
  auto C = ChatManager::instance->get_chat(sender.chat_id_);
  return out << " " << C;
}

Outputter &operator<<(Outputter &out, const td::td_api::messageSenderUser &sender) {
  auto C = ChatManager::instance->get_user(sender.user_id_);
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

}  // namespace tdcurses
