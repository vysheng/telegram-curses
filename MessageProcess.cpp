#include "MessageProcess.hpp"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/overloaded.h"

namespace tdcurses {

const td::td_api::formattedText *message_get_formatted_text(const td::td_api::message &message) {
  if (!message.content_) {
    return nullptr;
  }
  const td::td_api::formattedText *res = nullptr;
  td::td_api::downcast_call(
      const_cast<td::td_api::MessageContent &>(*message.content_),
      td::overloaded(
          [&](const td::td_api::messageText &content) { res = content.text_.get(); },
          [&](const td::td_api::messageAnimation &content) { res = content.caption_.get(); },
          [&](const td::td_api::messageAudio &content) { res = content.caption_.get(); },
          [&](const td::td_api::messageDocument &content) { res = content.caption_.get(); },
          [&](const td::td_api::messagePaidMedia &content) { res = content.caption_.get(); },
          [&](const td::td_api::messagePhoto &content) { res = content.caption_.get(); },
          [&](const td::td_api::messageSticker &content) {},
          [&](const td::td_api::messageVideo &content) { res = content.caption_.get(); },
          [&](const td::td_api::messageVideoNote &content) {}, [&](const td::td_api::messageVoiceNote &content) {},
          [&](const td::td_api::messageExpiredPhoto &content) {},
          [&](const td::td_api::messageExpiredVideo &content) {},
          [&](const td::td_api::messageExpiredVideoNote &content) {},
          [&](const td::td_api::messageExpiredVoiceNote &content) {},
          [&](const td::td_api::messageLocation &content) {}, [&](const td::td_api::messageVenue &content) {},
          [&](const td::td_api::messageContact &content) {}, [&](const td::td_api::messageAnimatedEmoji &content) {},
          [&](const td::td_api::messageDice &content) {}, [&](const td::td_api::messageGame &content) {},
          [&](const td::td_api::messagePoll &content) { res = content.poll_->question_.get(); },
          [&](const td::td_api::messageStory &content) {}, [&](const td::td_api::messageInvoice &content) {},
          [&](const td::td_api::messageCall &content) {}, [&](const td::td_api::messageGroupCall &content) {},
          [&](const td::td_api::messageVideoChatScheduled &content) {},
          [&](const td::td_api::messageVideoChatStarted &content) {},
          [&](const td::td_api::messageVideoChatEnded &content) {},
          [&](const td::td_api::messageInviteVideoChatParticipants &content) {},
          [&](const td::td_api::messageBasicGroupChatCreate &content) {},
          [&](const td::td_api::messageSupergroupChatCreate &content) {},
          [&](const td::td_api::messageChatChangeTitle &content) {},
          [&](const td::td_api::messageChatChangePhoto &content) {},
          [&](const td::td_api::messageChatDeletePhoto &content) {},
          [&](const td::td_api::messageChatAddMembers &content) {},
          [&](const td::td_api::messageChatJoinByLink &content) {},
          [&](const td::td_api::messageChatJoinByRequest &content) {},
          [&](const td::td_api::messageChatDeleteMember &content) {},
          [&](const td::td_api::messageChatUpgradeTo &content) {},
          [&](const td::td_api::messageChatUpgradeFrom &content) {},
          [&](const td::td_api::messagePinMessage &content) {},
          [&](const td::td_api::messageScreenshotTaken &content) {},
          [&](const td::td_api::messageChatSetBackground &content) {},
          [&](const td::td_api::messageChatSetTheme &content) {},
          [&](const td::td_api::messageChatSetMessageAutoDeleteTime &content) {},
          [&](const td::td_api::messageChatBoost &content) {},
          [&](const td::td_api::messageForumTopicCreated &content) {},
          [&](const td::td_api::messageForumTopicEdited &content) {},
          [&](const td::td_api::messageForumTopicIsClosedToggled &content) {},
          [&](const td::td_api::messageForumTopicIsHiddenToggled &content) {},
          [&](const td::td_api::messageSuggestProfilePhoto &content) {},
          [&](const td::td_api::messageCustomServiceAction &content) {},
          [&](const td::td_api::messageGameScore &content) {},
          [&](const td::td_api::messagePaymentSuccessful &content) {},
          [&](const td::td_api::messagePaymentSuccessfulBot &content) {},
          [&](const td::td_api::messagePaymentRefunded &content) {},
          [&](const td::td_api::messageGiftedPremium &content) {},
          [&](const td::td_api::messagePremiumGiftCode &content) {},
          [&](const td::td_api::messageGiveawayCreated &content) {}, [&](const td::td_api::messageGiveaway &content) {},
          [&](const td::td_api::messageGiveawayCompleted &content) {},
          [&](const td::td_api::messageGiveawayWinners &content) {},
          [&](const td::td_api::messageGiveawayPrizeStars &content) {}, [&](const td::td_api::messageGift &content) {},
          [&](const td::td_api::messageUpgradedGift &content) {},
          [&](const td::td_api::messageRefundedUpgradedGift &content) {},
          [&](const td::td_api::messagePaidMessagesRefunded &content) {},
          [&](const td::td_api::messagePaidMessagePriceChanged &content) {},
          [&](const td::td_api::messageGiftedStars &content) {},
          [&](const td::td_api::messageContactRegistered &content) {},
          [&](const td::td_api::messageUsersShared &content) {}, [&](const td::td_api::messageChatShared &content) {},
          [&](const td::td_api::messageBotWriteAccessAllowed &content) {},
          [&](const td::td_api::messageWebAppDataSent &content) {},
          [&](const td::td_api::messageWebAppDataReceived &content) {},
          [&](const td::td_api::messagePassportDataSent &content) {},
          [&](const td::td_api::messagePassportDataReceived &content) {},
          [&](const td::td_api::messageProximityAlertTriggered &content) {},
          [&](const td::td_api::messageUnsupported &content) {}));
  return res;
}

td::tl_object_ptr<td::td_api::file> *message_get_file_object_ref(td::td_api::message &message) {
  if (!message.content_) {
    return nullptr;
  }
  td::tl_object_ptr<td::td_api::file> *res = nullptr;
  td::td_api::downcast_call(
      *message.content_,
      td::overloaded(
          [&](td::td_api::messageText &content) {},
          [&](td::td_api::messageAnimation &content) { res = &content.animation_->animation_; },
          [&](td::td_api::messageAudio &content) { res = &content.audio_->audio_; },
          [&](td::td_api::messageDocument &content) { res = &content.document_->document_; },
          [&](td::td_api::messagePaidMedia &content) {},
          [&](td::td_api::messagePhoto &content) { res = &content.photo_->sizes_.back()->photo_; },
          [&](td::td_api::messageSticker &content) { res = &content.sticker_->sticker_; },
          [&](td::td_api::messageVideo &content) { res = &content.video_->video_; },
          [&](td::td_api::messageVideoNote &content) { res = &content.video_note_->video_; },
          [&](td::td_api::messageVoiceNote &content) { res = &content.voice_note_->voice_; },
          [&](td::td_api::messageExpiredPhoto &content) {}, [&](td::td_api::messageExpiredVideo &content) {},
          [&](td::td_api::messageExpiredVideoNote &content) {}, [&](td::td_api::messageExpiredVoiceNote &content) {},
          [&](td::td_api::messageLocation &content) {}, [&](td::td_api::messageVenue &content) {},
          [&](td::td_api::messageContact &content) {},
          [&](td::td_api::messageAnimatedEmoji &content) {
            if (content.animated_emoji_->sticker_) {
              res = &content.animated_emoji_->sticker_->sticker_;
            }
          },
          [&](td::td_api::messageDice &content) {}, [&](td::td_api::messageGame &content) {},
          [&](td::td_api::messagePoll &content) {}, [&](td::td_api::messageStory &content) {},
          [&](td::td_api::messageInvoice &content) {}, [&](td::td_api::messageCall &content) {},
          [&](td::td_api::messageGroupCall &content) {}, [&](td::td_api::messageVideoChatScheduled &content) {},
          [&](td::td_api::messageVideoChatStarted &content) {}, [&](td::td_api::messageVideoChatEnded &content) {},
          [&](td::td_api::messageInviteVideoChatParticipants &content) {},
          [&](td::td_api::messageBasicGroupChatCreate &content) {},
          [&](td::td_api::messageSupergroupChatCreate &content) {}, [&](td::td_api::messageChatChangeTitle &content) {},
          [&](td::td_api::messageChatChangePhoto &content) { res = &content.photo_->sizes_.back()->photo_; },
          [&](td::td_api::messageChatDeletePhoto &content) {}, [&](td::td_api::messageChatAddMembers &content) {},
          [&](td::td_api::messageChatJoinByLink &content) {}, [&](td::td_api::messageChatJoinByRequest &content) {},
          [&](td::td_api::messageChatDeleteMember &content) {}, [&](td::td_api::messageChatUpgradeTo &content) {},
          [&](td::td_api::messageChatUpgradeFrom &content) {}, [&](td::td_api::messagePinMessage &content) {},
          [&](td::td_api::messageScreenshotTaken &content) {}, [&](td::td_api::messageChatSetBackground &content) {},
          [&](td::td_api::messageChatSetTheme &content) {},
          [&](td::td_api::messageChatSetMessageAutoDeleteTime &content) {},
          [&](td::td_api::messageChatBoost &content) {}, [&](td::td_api::messageForumTopicCreated &content) {},
          [&](td::td_api::messageForumTopicEdited &content) {},
          [&](td::td_api::messageForumTopicIsClosedToggled &content) {},
          [&](td::td_api::messageForumTopicIsHiddenToggled &content) {},
          [&](td::td_api::messageSuggestProfilePhoto &content) {},
          [&](td::td_api::messageCustomServiceAction &content) {}, [&](td::td_api::messageGameScore &content) {},
          [&](td::td_api::messagePaymentSuccessful &content) {},
          [&](td::td_api::messagePaymentSuccessfulBot &content) {}, [&](td::td_api::messagePaymentRefunded &content) {},
          [&](td::td_api::messageGiftedPremium &content) {}, [&](td::td_api::messagePremiumGiftCode &content) {},
          [&](td::td_api::messageGiveawayCreated &content) {}, [&](td::td_api::messageGiveaway &content) {},
          [&](td::td_api::messageGiveawayCompleted &content) {}, [&](td::td_api::messageGiveawayWinners &content) {},
          [&](td::td_api::messageGiveawayPrizeStars &content) {}, [&](td::td_api::messageGift &content) {},
          [&](td::td_api::messageUpgradedGift &content) {}, [&](td::td_api::messageRefundedUpgradedGift &content) {},
          [&](td::td_api::messagePaidMessagesRefunded &content) {},
          [&](td::td_api::messagePaidMessagePriceChanged &content) {}, [&](td::td_api::messageGiftedStars &content) {},
          [&](td::td_api::messageContactRegistered &content) {}, [&](td::td_api::messageUsersShared &content) {},
          [&](td::td_api::messageChatShared &content) {}, [&](td::td_api::messageBotWriteAccessAllowed &content) {},
          [&](td::td_api::messageWebAppDataSent &content) {}, [&](td::td_api::messageWebAppDataReceived &content) {},
          [&](td::td_api::messagePassportDataSent &content) {},
          [&](td::td_api::messagePassportDataReceived &content) {},
          [&](td::td_api::messageProximityAlertTriggered &content) {},
          [&](td::td_api::messageUnsupported &content) {}));
  return res;
}

const td::td_api::file *message_get_file(const td::td_api::message &message) {
  if (!message.content_) {
    return nullptr;
  }
  auto ptr = message_get_file_object_ref(const_cast<td::td_api::message &>(message));
  if (ptr) {
    return (*ptr).get();
  } else {
    return nullptr;
  }
}

td::tl_object_ptr<td::td_api::textEntityTypePreCode> clone_tl_object(const td::td_api::textEntityTypePreCode &obj) {
  //textEntityTypePreCode language:string = TextEntityType;
  return td::make_tl_object<td::td_api::textEntityTypePreCode>(obj.language_);
}

td::tl_object_ptr<td::td_api::textEntityTypeTextUrl> clone_tl_object(const td::td_api::textEntityTypeTextUrl &obj) {
  //textEntityTypeTextUrl url:string = TextEntityType;
  return td::make_tl_object<td::td_api::textEntityTypeTextUrl>(obj.url_);
}
td::tl_object_ptr<td::td_api::textEntityTypeMentionName> clone_tl_object(
    const td::td_api::textEntityTypeMentionName &obj) {
  //textEntityTypeMentionName user_id:int53 = TextEntityType;
  return td::make_tl_object<td::td_api::textEntityTypeMentionName>(obj.user_id_);
}

td::tl_object_ptr<td::td_api::textEntityTypeCustomEmoji> clone_tl_object(
    const td::td_api::textEntityTypeCustomEmoji &obj) {
  //textEntityTypeCustomEmoji custom_emoji_id:int64 = TextEntityType;
  return td::make_tl_object<td::td_api::textEntityTypeCustomEmoji>(obj.custom_emoji_id_);
}

td::tl_object_ptr<td::td_api::textEntityTypeMediaTimestamp> clone_tl_object(
    const td::td_api::textEntityTypeMediaTimestamp &obj) {
  //textEntityTypeMediaTimestamp media_timestamp:int32 = TextEntityType;
  return td::make_tl_object<td::td_api::textEntityTypeMediaTimestamp>(obj.media_timestamp_);
}

td::tl_object_ptr<td::td_api::textEntity> clone_tl_object(const td::td_api::textEntity &obj) {
  //textEntity offset:int32 length:int32 type:TextEntityType = TextEntity;
  return td::make_tl_object<td::td_api::textEntity>(obj.offset_, obj.length_, clone_tl_object(*obj.type_));
}

td::tl_object_ptr<td::td_api::formattedText> clone_tl_object(const td::td_api::formattedText &obj) {
  //formattedText text:string entities:vector<textEntity> = FormattedText;
  return td::make_tl_object<td::td_api::formattedText>(obj.text_, clone_tl_object(obj.entities_));
}

}  // namespace tdcurses
