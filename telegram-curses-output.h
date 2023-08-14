#pragma once
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"

#include "td/utils/buffer.h"
#include "td/utils/StringBuilder.h"
#include "td/utils/int_types.h"

#include "print.h"
#include "chat-manager.h"
#include <memory>
//#include "types.h"

namespace tdcurses {

Outputter &operator<<(Outputter &, const td::td_api::messageSenderUser &message);
Outputter &operator<<(Outputter &, const td::td_api::messageSenderChat &message);

Outputter &operator<<(Outputter &, const td::td_api::message &message);
Outputter &operator<<(Outputter &, const td::td_api::messageForwardInfo &fwd_info);
Outputter &operator<<(Outputter &, const td::td_api::messageForwardOriginUser &fwd_info);
Outputter &operator<<(Outputter &, const td::td_api::messageForwardOriginChannel &fwd_info);
Outputter &operator<<(Outputter &, const td::td_api::messageForwardOriginChat &fwd_info);
Outputter &operator<<(Outputter &, const td::td_api::messageForwardOriginHiddenUser &fwd_info);
Outputter &operator<<(Outputter &, const td::td_api::messageForwardOriginMessageImport &fwd_info);

Outputter &operator<<(Outputter &, const td::td_api::messageText &content);
Outputter &operator<<(Outputter &, const td::td_api::messageAnimation &content);
Outputter &operator<<(Outputter &, const td::td_api::messageAudio &content);
Outputter &operator<<(Outputter &, const td::td_api::messageDocument &content);
Outputter &operator<<(Outputter &, const td::td_api::messagePhoto &content);
Outputter &operator<<(Outputter &, const td::td_api::messageExpiredPhoto &content);
Outputter &operator<<(Outputter &, const td::td_api::messageSticker &content);
Outputter &operator<<(Outputter &, const td::td_api::messageVideo &content);
Outputter &operator<<(Outputter &, const td::td_api::messageExpiredVideo &content);
Outputter &operator<<(Outputter &, const td::td_api::messageVideoNote &content);
Outputter &operator<<(Outputter &, const td::td_api::messageVoiceNote &content);
Outputter &operator<<(Outputter &, const td::td_api::messageLocation &content);
Outputter &operator<<(Outputter &, const td::td_api::messageVenue &content);
Outputter &operator<<(Outputter &, const td::td_api::messageContact &content);
Outputter &operator<<(Outputter &, const td::td_api::messageAnimatedEmoji &content);
Outputter &operator<<(Outputter &, const td::td_api::messageDice &content);
Outputter &operator<<(Outputter &, const td::td_api::messageGame &content);
Outputter &operator<<(Outputter &, const td::td_api::messagePoll &content);
Outputter &operator<<(Outputter &, const td::td_api::messageInvoice &content);
Outputter &operator<<(Outputter &, const td::td_api::messageCall &content);
Outputter &operator<<(Outputter &, const td::td_api::messageVideoChatScheduled &content);
Outputter &operator<<(Outputter &, const td::td_api::messageVideoChatStarted &content);
Outputter &operator<<(Outputter &, const td::td_api::messageVideoChatEnded &content);
Outputter &operator<<(Outputter &, const td::td_api::messageInviteVideoChatParticipants &content);
Outputter &operator<<(Outputter &, const td::td_api::messageBasicGroupChatCreate &content);
Outputter &operator<<(Outputter &, const td::td_api::messageSupergroupChatCreate &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatChangeTitle &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatChangePhoto &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatDeletePhoto &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatAddMembers &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatJoinByLink &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatJoinByRequest &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatDeleteMember &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatUpgradeTo &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatUpgradeFrom &content);
Outputter &operator<<(Outputter &, const td::td_api::messagePinMessage &content);
Outputter &operator<<(Outputter &, const td::td_api::messageScreenshotTaken &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatSetBackground &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatSetTheme &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatSetMessageAutoDeleteTime &content);
Outputter &operator<<(Outputter &, const td::td_api::messageForumTopicCreated &content);
Outputter &operator<<(Outputter &, const td::td_api::messageForumTopicEdited &content);
Outputter &operator<<(Outputter &, const td::td_api::messageForumTopicIsClosedToggled &content);
Outputter &operator<<(Outputter &, const td::td_api::messageForumTopicIsHiddenToggled &content);
Outputter &operator<<(Outputter &, const td::td_api::messageSuggestProfilePhoto &content);
Outputter &operator<<(Outputter &, const td::td_api::messageCustomServiceAction &content);
Outputter &operator<<(Outputter &, const td::td_api::messageGameScore &content);
Outputter &operator<<(Outputter &, const td::td_api::messagePaymentSuccessful &content);
Outputter &operator<<(Outputter &, const td::td_api::messagePaymentSuccessfulBot &content);
Outputter &operator<<(Outputter &, const td::td_api::messageContactRegistered &content);
Outputter &operator<<(Outputter &, const td::td_api::messageWebsiteConnected &content);
Outputter &operator<<(Outputter &, const td::td_api::messagePassportDataSent &content);
Outputter &operator<<(Outputter &, const td::td_api::messagePassportDataReceived &content);
Outputter &operator<<(Outputter &, const td::td_api::messageProximityAlertTriggered &content);
Outputter &operator<<(Outputter &, const td::td_api::messageGiftedPremium &content);
Outputter &operator<<(Outputter &, const td::td_api::messageUserShared &content);
Outputter &operator<<(Outputter &, const td::td_api::messageChatShared &content);
Outputter &operator<<(Outputter &, const td::td_api::messageBotWriteAccessAllowed &content);
Outputter &operator<<(Outputter &, const td::td_api::messageWebAppDataSent &content);
Outputter &operator<<(Outputter &, const td::td_api::messageWebAppDataReceived &content);
Outputter &operator<<(Outputter &, const td::td_api::messageStory &content);
Outputter &operator<<(Outputter &, const td::td_api::messageUnsupported &content);

Outputter &operator<<(Outputter &, const td::td_api::formattedText &content);
Outputter &operator<<(Outputter &, const td::td_api::webPage &content);
Outputter &operator<<(Outputter &, const td::td_api::animation &content);
Outputter &operator<<(Outputter &, const td::td_api::audio &content);
Outputter &operator<<(Outputter &, const td::td_api::document &content);
Outputter &operator<<(Outputter &, const td::td_api::photo &content);
Outputter &operator<<(Outputter &, const td::td_api::sticker &content);
Outputter &operator<<(Outputter &, const td::td_api::video &content);
Outputter &operator<<(Outputter &, const td::td_api::videoNote &content);
Outputter &operator<<(Outputter &, const td::td_api::voiceNote &content);
Outputter &operator<<(Outputter &, const td::td_api::location &content);
Outputter &operator<<(Outputter &, const td::td_api::venue &content);
Outputter &operator<<(Outputter &, const td::td_api::contact &content);
Outputter &operator<<(Outputter &, const td::td_api::game &content);
Outputter &operator<<(Outputter &, const td::td_api::poll &content);

Outputter &operator<<(Outputter &, const td::td_api::file &file);
Outputter &operator<<(Outputter &, const std::shared_ptr<Chat> &chat);
Outputter &operator<<(Outputter &, const std::shared_ptr<User> &chat);

Outputter &operator<<(Outputter &, const td::td_api::messageSenderChat &sender);
Outputter &operator<<(Outputter &, const td::td_api::messageSenderUser &sender);

Outputter &operator<<(Outputter &, const td::td_api::reactionTypeEmoji &file);
Outputter &operator<<(Outputter &, const td::td_api::reactionTypeCustomEmoji &file);
}  // namespace tdcurses
