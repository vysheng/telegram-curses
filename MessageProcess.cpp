#include "MessageProcess.hpp"
#include "td/telegram/td_api.hpp"
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
          [&](const td::td_api::messageCall &content) {}, [&](const td::td_api::messageVideoChatScheduled &content) {},
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

const td::td_api::file *message_get_file(const td::td_api::message &message) {
  if (!message.content_) {
    return nullptr;
  }
  const td::td_api::file *res = nullptr;
  td::td_api::downcast_call(
      const_cast<td::td_api::MessageContent &>(*message.content_),
      td::overloaded(
          [&](const td::td_api::messageText &content) {},
          [&](const td::td_api::messageAnimation &content) { res = content.animation_->animation_.get(); },
          [&](const td::td_api::messageAudio &content) { res = content.audio_->audio_.get(); },
          [&](const td::td_api::messageDocument &content) { res = content.document_->document_.get(); },
          [&](const td::td_api::messagePaidMedia &content) {},
          [&](const td::td_api::messagePhoto &content) { res = content.photo_->sizes_.back()->photo_.get(); },
          [&](const td::td_api::messageSticker &content) { res = content.sticker_->sticker_.get(); },
          [&](const td::td_api::messageVideo &content) { res = content.video_->video_.get(); },
          [&](const td::td_api::messageVideoNote &content) { res = content.video_note_->video_.get(); },
          [&](const td::td_api::messageVoiceNote &content) { res = content.voice_note_->voice_.get(); },
          [&](const td::td_api::messageExpiredPhoto &content) {},
          [&](const td::td_api::messageExpiredVideo &content) {},
          [&](const td::td_api::messageExpiredVideoNote &content) {},
          [&](const td::td_api::messageExpiredVoiceNote &content) {},
          [&](const td::td_api::messageLocation &content) {}, [&](const td::td_api::messageVenue &content) {},
          [&](const td::td_api::messageContact &content) {},
          [&](const td::td_api::messageAnimatedEmoji &content) {
            res = content.animated_emoji_->sticker_->sticker_.get();
          },
          [&](const td::td_api::messageDice &content) {}, [&](const td::td_api::messageGame &content) {},
          [&](const td::td_api::messagePoll &content) {}, [&](const td::td_api::messageStory &content) {},
          [&](const td::td_api::messageInvoice &content) {}, [&](const td::td_api::messageCall &content) {},
          [&](const td::td_api::messageVideoChatScheduled &content) {},
          [&](const td::td_api::messageVideoChatStarted &content) {},
          [&](const td::td_api::messageVideoChatEnded &content) {},
          [&](const td::td_api::messageInviteVideoChatParticipants &content) {},
          [&](const td::td_api::messageBasicGroupChatCreate &content) {},
          [&](const td::td_api::messageSupergroupChatCreate &content) {},
          [&](const td::td_api::messageChatChangeTitle &content) {},
          [&](const td::td_api::messageChatChangePhoto &content) {
            res = content.photo_->sizes_.front()->photo_.get();
          },
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

}  // namespace tdcurses
