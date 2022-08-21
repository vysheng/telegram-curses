#include "td/actor/impl/ActorId-decl.h"
#include "td/actor/impl/Scheduler-decl.h"
#include "td/tdactor/td/actor/actor.h"
#include "td/tdutils/td/utils/int_types.h"
#include "td/tl/TlObject.h"
#include "td/generate/auto/td/telegram/td_api.h"
#include "td/generate/auto/td/telegram/td_api.hpp"
#include "td/telegram/ClientActor.h"
#include "td/tdactor/td/actor/PromiseFuture.h"
#include "td/tdactor/td/actor/ConcurrentScheduler.h"
#include "td/tdutils/td/utils/type_traits.h"
#include "td/tdutils/td/utils/buffer.h"
#include "td/tdutils/td/utils/Variant.h"
#include "td/utils/Status.h"
#include "td/utils/format.h"
#include "td/utils/logging.h"
#include "td/utils/misc.h"
#include "td/utils/overloaded.h"
#include "td/utils/port/signals.h"
#include "td/utils/OptionParser.h"
#include "td/utils/filesystem.h"
#include "td/utils/port/path.h"
#include "td/utils/FileLog.h"
#include "td/telegram/Td.h"
#include "td/utils/port/StdStreams.h"

#include "windows/editorwindow.h"
#include "windows/screen.h"
#include "windows/unicode.h"

//#include "telegram-cli-output.h"
#include "telegram-curses.h"
#include "telegram-curses.hpp"
#include "telegram-curses-version.h"
#include "qrcodegen/qrcodegen.hpp"
#include "dialog-list-window.h"
#include "compose-window.h"
#include "chat-window.h"
#include "config-window.h"
#include "windows/window.h"
#include "windows/log-window.h"
#include "print.h"
#include "layout.h"

#include <atomic>
#include <cstdio>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <libconfig.h++>
#include <signal.h>
#include <openssl/opensslv.h>
#include <readline/readline.h>

#if HAVE_PWD_H
#include <pwd.h>
#endif

#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif

td::int32 total_updates;

namespace tdcurses {

std::string detect_home_dir() {
  auto env = ::getenv("HOME");
  if (env && *env) {
    return env;
  }

#if HAVE_PWD_H
  uid_t uid = getuid();
  struct passwd *pw = getpwuid(uid);

  if (pw == NULL) {
    LOG(WARNING) << "cannot find HOME dir";
    exit(EXIT_FAILURE);
  }

  return pw->pw_dir;
#else
  LOG(WARNING) << "cannot find HOME dir";
  exit(EXIT_FAILURE);
#endif
}

std::string detect_config_name() {
  auto env = ::getenv("XDG_CONFIG_HOME");
  if (env && *env) {
    std::string e = env;
    if (e[e.size() - 1] != '/') {
      e = e + "/";
    }
    td::mkdir(e + "telegram-curses").ensure();
    return e + "telegram-curses/config";
  } else {
    LOG(WARNING) << "XDG_CONFIG_HOME not defined";
    std::string e = detect_home_dir() + "/.config/";
    td::mkdir(e + "telegram-curses").ensure();
    return e + "telegram-curses/config";
  }
}

std::string detect_db_root_name() {
  auto env = ::getenv("XDG_DATA_HOME");
  if (env && *env) {
    std::string e = env;
    if (e[e.size() - 1] != '/') {
      e = e + "/";
    }
    td::mkdir(e + "telegram-curses").ensure();
    return e + "telegram-curses/";
  } else {
    LOG(WARNING) << "XDG_DATA_HOME not defined";
    std::string e = detect_home_dir() + "/.local/share/";
    td::mkdir(e + "telegram-curses").ensure();
    return e + "telegram-curses/";
  }
}

class TdcursesImpl : public Tdcurses {
 private:
  td::tl_object_ptr<td::td_api::tdlibParameters> tdlib_parameters_;
  td::ActorId<TdcursesImpl> self_;
  std::string root_directory_ = ".tdcurses";
  bool starting_ = false;
  td::ActorOwn<td::ClientActor> td_;

  td::unique_ptr<td::TdCallback> make_td_callback() {
    class TdCallbackImpl : public td::TdCallback {
     public:
      explicit TdCallbackImpl(td::ActorId<TdcursesImpl> client) : client_(client) {
      }
      void on_result(td::uint64 id, td::tl_object_ptr<td::td_api::Object> result) override {
        if (id == 0) {
          td::send_closure(client_, &TdcursesImpl::on_update, td::move_tl_object_as<td::td_api::Update>(result));
        } else {
          td::send_closure(client_, &TdcursesImpl::on_result, id, std::move(result));
        }
      }
      void on_error(td::uint64 id, td::tl_object_ptr<td::td_api::error> error) override {
        td::send_closure(client_, &TdcursesImpl::on_error, id, std::move(error));
      }

     private:
      td::ActorId<TdcursesImpl> client_;
    };
    return td::make_unique<TdCallbackImpl>(self_);
  }

 public:
  TdcursesImpl() {
  }

  void get_os_type() {
  }

  void change_system_version(std::string version) {
    CHECK(!starting_);
    tdlib_parameters_->system_version_ = version;
  }

  void start(td::tl_object_ptr<td::td_api::tdlibParameters> tdlib_parameters,
             std::unique_ptr<TdcursesParameters> tdcurses_params) {
    self_ = actor_id(this);
    tdlib_parameters_ = std::move(tdlib_parameters);
    starting_ = true;

    start_curses(*tdcurses_params);

    td_ = td::create_actor<td::ClientActor>("ClientActor", make_td_callback());
  }

  void notify() override {
    // NB: Interface will be changed
    td::send_closure_later(self_, &TdcursesImpl::on_net);
  }
  void on_net() {
    loop();
  }

  void on_result(td::uint64 id, td::tl_object_ptr<td::td_api::Object> result) {
    auto it = handlers_.find(id);
    it->second.set_result(std::move(result));
    handlers_.erase(it);
  }
  void on_error(td::uint64 id, td::tl_object_ptr<td::td_api::error> result) {
    auto it = handlers_.find(id);
    it->second.set_error(td::Status::Error(result->code_, result->message_));
    handlers_.erase(it);
  }

  void on_update(td::tl_object_ptr<td::td_api::Update> update) {
    total_updates++;
    td::td_api::downcast_call(*update.get(), [Self = this](auto &obj) { Self->process_update(obj); });
    refresh();
  }

  void process_auth_state(td::td_api::authorizationStateWaitTdlibParameters &state) {
    send_request(td::make_tl_object<td::td_api::setTdlibParameters>(std::move(tdlib_parameters_)),
                 td::PromiseCreator::lambda([](td::Result<td::tl_object_ptr<td::td_api::ok>> R) { R.ensure(); }));
  }

  void received_database_password(td::Result<td::BufferSlice> res) {
    if (res.is_error()) {
      request_database_password();
    } else {
      send_request(td::make_tl_object<td::td_api::checkDatabaseEncryptionKey>(res.move_as_ok().as_slice().str()),
                   td::PromiseCreator::lambda([SelfId = self_](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                     if (R.is_error()) {
                       td::send_closure(SelfId, &TdcursesImpl::request_database_password);
                     }
                   }));
    }
  }

  void request_database_password() {
    class Callback : public windows::OneLineInputWindow::Callback {
     public:
      Callback(TdcursesImpl *ptr) : ptr_(ptr) {
      }
      void on_answer(windows::OneLineInputWindow *self, std::string text) override {
        ptr_->received_database_password(td::BufferSlice(text));
        ptr_->del_popup_window(window_);
      }
      void on_abort(windows::OneLineInputWindow *self, std::string text) override {
        ptr_->received_database_password(td::Status::Error("cancelled"));
        ptr_->del_popup_window(window_);
      }

      void set_window(windows::Window *window) {
        window_ = window;
      }

     private:
      TdcursesImpl *ptr_;
      windows::Window *window_;
    };
    auto cb = std::make_unique<Callback>(this);
    auto w = std::make_shared<windows::OneLineInputWindow>("db password: ", true, nullptr);
    auto box = std::make_shared<windows::BorderedWindow>(w, windows::BorderedWindow::BorderType::Double);
    cb->set_window(box.get());
    w->set_callback(std::move(cb));
    add_popup_window(box, 0);
  }

  //authorizationStateWaitEncryptionKey is_encrypted:Bool = AuthorizationState;
  void process_auth_state(td::td_api::authorizationStateWaitEncryptionKey &state) {
    request_database_password();
  }

  void received_phone_number(td::Result<td::BufferSlice> res) {
    if (res.is_error()) {
      request_phone_number();
    } else {
      send_request(
          td::make_tl_object<td::td_api::setAuthenticationPhoneNumber>(
              res.move_as_ok().as_slice().str(), td::make_tl_object<td::td_api::phoneNumberAuthenticationSettings>(
                                                     false, false, false, false, std::vector<std::string>())),
          td::PromiseCreator::lambda([SelfId = self_](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
            if (R.is_error()) {
              td::send_closure(SelfId, &TdcursesImpl::request_phone_number);
            }
          }));
    }
  }

  void request_phone_number() {
    class Callback : public windows::OneLineInputWindow::Callback {
     public:
      Callback(TdcursesImpl *ptr) : ptr_(ptr) {
      }
      void on_answer(windows::OneLineInputWindow *self, std::string text) override {
        ptr_->received_phone_number(td::BufferSlice(text));
        ptr_->del_popup_window(window_);
      }
      void on_abort(windows::OneLineInputWindow *self, std::string text) override {
        ptr_->received_phone_number(td::Status::Error("cancelled"));
        ptr_->del_popup_window(window_);
      }

      void set_window(windows::Window *window) {
        window_ = window;
      }

     private:
      TdcursesImpl *ptr_;
      windows::Window *window_;
    };
    auto cb = std::make_unique<Callback>(this);
    auto w = std::make_shared<windows::OneLineInputWindow>("phone number: ", false, nullptr);
    auto box = std::make_shared<windows::BorderedWindow>(w, windows::BorderedWindow::BorderType::Double);
    cb->set_window(box.get());
    w->set_callback(std::move(cb));
    add_popup_window(box, 0);
  }

  void request_qr_code() {
    send_request(td::make_tl_object<td::td_api::requestQrCodeAuthentication>(std::vector<td::int64>()),
                 td::PromiseCreator::lambda([SelfId = self_](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                   if (R.is_error()) {
                     td::send_closure(SelfId, &TdcursesImpl::request_phone_number);
                   }
                 }));
  }

  void process_auth_state(td::td_api::authorizationStateWaitPhoneNumber &state) {
    request_qr_code();
    //request_phone_number();
  }

  void received_code(td::Result<td::BufferSlice> res) {
    if (res.is_error()) {
      request_code();
    } else {
      send_request(td::make_tl_object<td::td_api::checkAuthenticationCode>(res.move_as_ok().as_slice().str()),
                   td::PromiseCreator::lambda([SelfId = self_](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                     if (R.is_error()) {
                       td::send_closure(SelfId, &TdcursesImpl::request_code);
                     }
                   }));
    }
  }

  void request_code() {
    class Callback : public windows::OneLineInputWindow::Callback {
     public:
      Callback(TdcursesImpl *ptr) : ptr_(ptr) {
      }
      void on_answer(windows::OneLineInputWindow *self, std::string text) override {
        ptr_->received_code(td::BufferSlice(text));
        ptr_->del_popup_window(window_);
      }
      void on_abort(windows::OneLineInputWindow *self, std::string text) override {
        ptr_->received_code(td::Status::Error("cancelled"));
        ptr_->del_popup_window(window_);
      }

      void set_window(windows::Window *window) {
        window_ = window;
      }

     private:
      TdcursesImpl *ptr_;
      windows::Window *window_;
    };
    auto cb = std::make_unique<Callback>(this);
    auto w = std::make_shared<windows::OneLineInputWindow>("auth code: ", false, nullptr);
    auto box = std::make_shared<windows::BorderedWindow>(w, windows::BorderedWindow::BorderType::Double);
    cb->set_window(box.get());
    w->set_callback(std::move(cb));
    add_popup_window(box, 0);
  }

  //authorizationStateWaitCode is_registered:Bool terms_of_service:termsOfService code_info:authenticationCodeInfo = AuthorizationState;
  void process_auth_state(td::td_api::authorizationStateWaitCode &state) {
    request_code();
  }

  void process_auth_state(td::td_api::authorizationStateWaitOtherDeviceConfirmation &state) {
    auto code = qrcodegen::QrCode::encodeText(state.link_.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
    auto size = code.getSize();

    if ((size + 2) * 2 > width() - 2 || size + 2 > height() - 2) {
      return request_phone_number();
    }

    Outputter out;
    {
      for (td::int32 i = -1; i <= size; i++) {
        for (td::int32 j = -1; j <= size; j++) {
          auto col = code.getModule(i, j);
          if (col) {
            out << Outputter::FgColor{Color::Black};
          } else {
            out << Outputter::FgColor{Color::White};
          }
          out << "\xe2\x96\x88"
              << "\xe2\x96\x88";
          out << Outputter::FgColor{Color::Revert};
        }
        out << "\n";
      }
      out << "\n";
    }

    class QrWindow : public windows::ViewWindow {
     public:
      QrWindow(std::string text, std::vector<windows::MarkupElement> markup, td::int32 size)
          : windows::ViewWindow(std::move(text), std::move(markup), nullptr), size_(size){};

      td::int32 min_width() override {
        return 2 * size_ + 4;
      }
      td::int32 min_height() override {
        return size_ + 2;
      }
      td::int32 best_width() override {
        return 2 * size_ + 4;
      }
      td::int32 best_height() override {
        return size_ + 2;
      }

     private:
      td::int32 size_;
      //UNREACHABLE();
    };

    auto w = std::make_shared<QrWindow>(out.as_str(), out.markup(), size);
    auto box = std::make_shared<windows::BorderedWindow>(w, windows::BorderedWindow::BorderType::Double);
    add_qr_code_window(box);
  }
  void received_name(td::Result<td::BufferSlice> res) {
    if (res.is_error()) {
      request_name();
    } else {
      auto str = res.move_as_ok().as_slice().str();
      auto ch = str.find(' ');
      std::string first_name = str;
      std::string last_name = "";
      if (ch != std::string::npos) {
        first_name = str.substr(0, ch);
        last_name = str.substr(ch + 1);
      }
      send_request(td::make_tl_object<td::td_api::registerUser>(first_name, last_name),
                   td::PromiseCreator::lambda([SelfId = self_](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                     if (R.is_error()) {
                       td::send_closure(SelfId, &TdcursesImpl::request_name);
                     }
                   }));
    }
  }

  void request_name() {
    class Callback : public windows::OneLineInputWindow::Callback {
     public:
      Callback(TdcursesImpl *ptr) : ptr_(ptr) {
      }
      void on_answer(windows::OneLineInputWindow *self, std::string text) override {
        ptr_->received_name(td::BufferSlice(text));
        ptr_->del_popup_window(window_);
      }
      void on_abort(windows::OneLineInputWindow *self, std::string text) override {
        ptr_->received_name(td::Status::Error("cancelled"));
        ptr_->del_popup_window(window_);
      }

      void set_window(windows::Window *window) {
        window_ = window;
      }

     private:
      TdcursesImpl *ptr_;
      windows::Window *window_;
    };
    auto cb = std::make_unique<Callback>(this);
    auto w = std::make_shared<windows::OneLineInputWindow>("your name: ", false, nullptr);
    auto box = std::make_shared<windows::BorderedWindow>(w, windows::BorderedWindow::BorderType::Double);
    cb->set_window(box.get());
    w->set_callback(std::move(cb));
    add_popup_window(box, 0);
  }

  void process_auth_state(td::td_api::authorizationStateWaitRegistration &state) {
    request_name();
  }

  void received_password(td::Result<td::BufferSlice> R) {
    if (R.is_error()) {
      request_password();
      return;
    }
    send_request(td::make_tl_object<td::td_api::checkAuthenticationPassword>(R.move_as_ok().as_slice().str()),
                 td::PromiseCreator::lambda([SelfId = self_](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                   if (R.is_error()) {
                     td::send_closure(SelfId, &TdcursesImpl::request_password);
                   }
                 }));
  }

  void request_password() {
    class Callback : public windows::OneLineInputWindow::Callback {
     public:
      Callback(TdcursesImpl *ptr) : ptr_(ptr) {
      }
      void on_answer(windows::OneLineInputWindow *self, std::string text) override {
        ptr_->received_password(td::BufferSlice(text));
        ptr_->del_popup_window(window_);
      }
      void on_abort(windows::OneLineInputWindow *self, std::string text) override {
        ptr_->received_password(td::Status::Error("cancelled"));
        ptr_->del_popup_window(window_);
      }

      void set_window(windows::Window *window) {
        window_ = window;
      }

     private:
      TdcursesImpl *ptr_;
      windows::Window *window_;
    };
    auto cb = std::make_unique<Callback>(this);
    auto w = std::make_shared<windows::OneLineInputWindow>("password: ", true, nullptr);
    auto box = std::make_shared<windows::BorderedWindow>(w, windows::BorderedWindow::BorderType::Double);
    cb->set_window(box.get());
    w->set_callback(std::move(cb));
    add_popup_window(box, 0);
  }

  //authorizationStateWaitPassword password_hint:string has_recovery_email_address:Bool recovery_email_address_pattern:string = AuthorizationState;
  void process_auth_state(td::td_api::authorizationStateWaitPassword &state) {
    request_password();
  }

  void process_auth_state(td::td_api::authorizationStateReady &state) {
    LOG(INFO) << "ready";
    // do nothing
    dialog_list_window()->request_bottom_elements();
  }

  void process_auth_state(td::td_api::authorizationStateLoggingOut &state) {
  }

  void process_auth_state(td::td_api::authorizationStateClosing &state) {
  }

  void process_auth_state(td::td_api::authorizationStateClosed &state) {
  }

  //@class Update @description Contains notifications about data changes

  //@description The user authorization state has changed @authorization_state New authorization state
  //updateAuthorizationState authorization_state:AuthorizationState = Update;
  void process_update(td::td_api::updateAuthorizationState &update) {
    del_qr_code_window();
    td::td_api::downcast_call(*update.authorization_state_, [&](auto &obj) { process_auth_state(obj); });
  }

  //@description A new message was received; can also be an outgoing message @message The new message
  //updateNewMessage message:message = Update;
  void process_update(td::td_api::updateNewMessage &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.message_->chat_id_) {
      c->process_update(update);
    }
  }

  //@description A request to send a message has reached the Telegram server. This doesn't mean that the message will be sent successfully or even that the send message request will be processed. This update will be sent only if the option "use_quick_ack" is set to true. This update may be sent multiple times for the same message
  //@chat_id The chat identifier of the sent message @message_id A temporary message identifier
  //updateMessageSendAcknowledged chat_id:int53 message_id:int53 = Update;
  void process_update(td::td_api::updateMessageSendAcknowledged &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message has been successfully sent @message Information about the sent message. Usually only the message identifier, date, and content are changed, but almost all other fields can also change @old_message_id The previous temporary message identifier
  //updateMessageSendSucceeded message:message old_message_id:int53 = Update;
  void process_update(td::td_api::updateMessageSendSucceeded &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.message_->chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message failed to send. Be aware that some messages being sent can be irrecoverably deleted, in which case updateDeleteMessages will be received instead of this update
  //@message Contains information about the message that failed to send @old_message_id The previous temporary message identifier @error_code An error code @error_message Error message
  //updateMessageSendFailed message:message old_message_id:int53 error_code:int32 error_message:string = Update;
  void process_update(td::td_api::updateMessageSendFailed &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.message_->chat_id_) {
      c->process_update(update);
    }
  }

  //@description The message content has changed @chat_id Chat identifier @message_id Message identifier @new_content New message content
  //updateMessageContent chat_id:int53 message_id:int53 new_content:MessageContent = Update;
  void process_update(td::td_api::updateMessageContent &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message was edited. Changes in the message content will come in a separate updateMessageContent @chat_id Chat identifier @message_id Message identifier @edit_date Point in time (Unix timestamp) when the message was edited @reply_markup New message reply markup; may be null
  //updateMessageEdited chat_id : int53 message_id : int53 edit_date : int32 reply_markup : ReplyMarkup = Update;
  void process_update(td::td_api::updateMessageEdited &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description The message pinned state was changed @chat_id Chat identifier @message_id The message identifier @is_pinned True, if the message is pinned
  //updateMessageIsPinned chat_id:int53 message_id:int53 is_pinned:Bool = Update;
  void process_update(td::td_api::updateMessageIsPinned &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description The information about interactions with a message has changed @chat_id Chat identifier @message_id Message identifier @interaction_info New information about interactions with the message; may be null
  //updateMessageInteractionInfo chat_id:int53 message_id:int53 interaction_info:messageInteractionInfo = Update;
  void process_update(td::td_api::updateMessageInteractionInfo &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description The message content was opened. Updates voice note messages to "listened", video note messages to "viewed" and starts the TTL timer for self-destructing messages @chat_id Chat identifier @message_id Message identifier
  //updateMessageContentOpened chat_id : int53 message_id : int53 = Update;
  void process_update(td::td_api::updateMessageContentOpened &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message with an unread mention was read @chat_id Chat identifier @message_id Message identifier @unread_mention_count The new number of unread mention messages left in the chat
  //updateMessageMentionRead chat_id : int53 message_id : int53 unread_mention_count : int32 = Update;
  void process_update(td::td_api::updateMessageMentionRead &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description The list of unread reactions added to a message was changed @chat_id Chat identifier @message_id Message identifier @unread_reactions The new list of unread reactions @unread_reaction_count The new number of messages with unread reactions left in the chat
  //updateMessageUnreadReactions chat_id:int53 message_id:int53 unread_reactions:vector<unreadReaction> unread_reaction_count:int32 = Update;
  void process_update(td::td_api::updateMessageUnreadReactions &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message with a live location was viewed. When the update is received, the client is supposed to update the live location
  //@chat_id Identifier of the chat with the live location message @message_id Identifier of the message with live location
  void process_update(td::td_api::updateMessageLiveLocationViewed &update) {
    auto c = chat_window();
    if (c && c->chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A new chat has been loaded/created. This update is guaranteed to come before the chat identifier is returned to the client. The chat field changes will be reported through separate updates @chat The chat
  //updateNewChat chat : chat = Update;
  void process_update(td::td_api::updateNewChat &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The title of a chat was changed @chat_id Chat identifier @title The new chat title
  //updateChatTitle chat_id : int53 title : string = Update;
  void process_update(td::td_api::updateChatTitle &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat photo was changed @chat_id Chat identifier @photo The new chat photo; may be null
  //updateChatPhoto chat_id : int53 photo : chatPhoto = Update;
  void process_update(td::td_api::updateChatPhoto &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Chat permissions was changed @chat_id Chat identifier @permissions The new chat permissions
  //updateChatPermissions chat_id:int53 permissions:chatPermissions = Update;
  void process_update(td::td_api::updateChatPermissions &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The last message of a chat was changed. If last_message is null then the last message in the chat became unknown. Some new unknown messages might be added to the chat in this case @chat_id Chat identifier @last_message The new last message in the chat; may be null @order New value of the chat order
  //updateChatLastMessage chat_id : int53 last_message : message order : int64 = Update;
  void process_update(td::td_api::updateChatLastMessage &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The position of a chat in a chat list has changed. Instead of this update updateChatLastMessage or updateChatDraftMessage might be sent @chat_id Chat identifier @position New chat position. If new order is 0, then the chat needs to be removed from the list
  //updateChatPosition chat_id:int53 position:chatPosition = Update;
  void process_update(td::td_api::updateChatPosition &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Incoming messages were read or number of unread messages has been changed @chat_id Chat identifier @last_read_inbox_message_id Identifier of the last read incoming message @unread_count The number of unread messages left in the chat
  //updateChatReadInbox chat_id : int53 last_read_inbox_message_id : int53 unread_count : int32 = Update;
  void process_update(td::td_api::updateChatReadInbox &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Outgoing messages were read @chat_id Chat identifier @last_read_outbox_message_id Identifier of last read outgoing message
  //updateChatReadOutbox chat_id : int53 last_read_outbox_message_id : int53 = Update;
  void process_update(td::td_api::updateChatReadOutbox &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat action bar was changed @chat_id Chat identifier @action_bar The new value of the action bar; may be null
  void process_update(td::td_api::updateChatActionBar &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat available reactions were changed @chat_id Chat identifier @available_reactions The new list of reactions, available in the chat
  //updateChatAvailableReactions chat_id:int53 available_reactions:vector<string> = Update;
  void process_update(td::td_api::updateChatAvailableReactions &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat draft has changed. Be aware that the update may come in the currently opened chat but with old content of the draft. If the user has changed the content of the draft, this update shouldn't be applied @chat_id Chat identifier @draft_message The new draft message; may be null @order New value of the chat order
  //updateChatDraftMessage chat_id : int53 draft_message : draftMessage order : int64 = Update;
  void process_update(td::td_api::updateChatDraftMessage &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The message sender that is selected to send messages in a chat has changed @chat_id Chat identifier @message_sender_id New value of message_sender_id; may be null if the user can't change message sender
  //updateChatMessageSender chat_id:int53 message_sender_id:MessageSender = Update;
  void process_update(td::td_api::updateChatMessageSender &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The message Time To Live setting for a chat was changed @chat_id Chat identifier @message_ttl New value of message_ttl
  //updateChatMessageTtl chat_id:int53 message_ttl:int32 = Update;
  void process_update(td::td_api::updateChatMessageTtl &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Notification settings for a chat were changed @chat_id Chat identifier @notification_settings The new notification settings
  //updateChatNotificationSettings chat_id : int53 notification_settings : chatNotificationSettings = Update;
  void process_update(td::td_api::updateChatNotificationSettings &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat pending join requests were changed @chat_id Chat identifier @pending_join_requests The new data about pending join requests; may be null
  //updateChatPendingJoinRequests chat_id:int53 pending_join_requests:chatJoinRequestsInfo = Update;
  void process_update(td::td_api::updateChatPendingJoinRequests &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The default chat reply markup was changed. Can occur because new messages with reply markup were received or because an old reply markup was hidden by the user
  //@chat_id Chat identifier @reply_markup_message_id Identifier of the message from which reply markup needs to be used; 0 if there is no default custom reply markup in the chat
  //updateChatReplyMarkup chat_id : int53 reply_markup_message_id : int53 = Update;
  void process_update(td::td_api::updateChatReplyMarkup &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat theme was changed @chat_id Chat identifier @theme_name The new name of the chat theme; may be empty if theme was reset to default
  //updateChatTheme chat_id:int53 theme_name:string = Update;
  void process_update(td::td_api::updateChatTheme &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat unread_mention_count has changed @chat_id Chat identifier @unread_mention_count The number of unread mention messages left in the chat
  //updateChatUnreadMentionCount chat_id : int53 unread_mention_count : int32 = Update;
  void process_update(td::td_api::updateChatUnreadMentionCount &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Number of unread messages has changed. This update is sent only if a message database is used @unread_count Total number of unread messages @unread_unmuted_count Total number of unread messages in unmuted chats
  //updateUnreadMessageCount unread_count : int32 unread_unmuted_count : int32 = Update;
  void process_update(td::td_api::updateChatUnreadReactionCount &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat video chat state has changed @chat_id Chat identifier @video_chat New value of video_chat
  //updateChatVideoChat chat_id:int53 video_chat:videoChat = Update;
  void process_update(td::td_api::updateChatVideoChat &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The value of the default disable_notification parameter, used when a message is sent to the chat, was changed @chat_id Chat identifier @default_disable_notification The new default_disable_notification value
  //updateChatDefaultDisableNotification chat_id : int53 default_disable_notification : Bool = Update;
  void process_update(td::td_api::updateChatDefaultDisableNotification &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat content was allowed or restricted for saving @chat_id Chat identifier @has_protected_content New value of has_protected_content
  //updateChatHasProtectedContent chat_id:int53 has_protected_content:Bool = Update;
  void process_update(td::td_api::updateChatHasProtectedContent &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat's has_scheduled_messages field has changed @chat_id Chat identifier @has_scheduled_messages New value of has_scheduled_messages
  void process_update(td::td_api::updateChatHasScheduledMessages &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat was blocked or unblocked @chat_id Chat identifier @is_blocked New value of is_blocked
  //updateChatIsBlocked chat_id:int53 is_blocked:Bool = Update;
  void process_update(td::td_api::updateChatIsBlocked &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat was marked as unread or was read @chat_id Chat identifier @is_marked_as_unread New value of is_marked_as_unread
  //updateChatIsMarkedAsUnread chat_id : int53 is_marked_as_unread : Bool = Update;
  void process_update(td::td_api::updateChatIsMarkedAsUnread &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The list of chat filters or a chat filter has changed @chat_filters The new list of chat filters
  //updateChatFilters chat_filters:vector<chatFilterInfo> = Update;
  void process_update(td::td_api::updateChatFilters &update) {
    //ChatManager::instance->process_update(update);
  }

  //@description The number of online group members has changed. This update with non-zero number of online group members is sent only for currently opened chats. There is no guarantee that it will be sent just after the number of online users has changed @chat_id Identifier of the chat @online_member_count New number of online members in the chat, or 0 if unknown
  //updateChatOnlineMemberCount chat_id:int53 online_member_count:int32 = Update;
  void process_update(td::td_api::updateChatOnlineMemberCount &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Notification settings for some type of chats were updated @scope Types of chats for which notification settings were updated @notification_settings The new notification settings
  //updateScopeNotificationSettings scope : NotificationSettingsScope notification_settings : scopeNotificationSettings =
  //  Update;
  void process_update(td::td_api::updateScopeNotificationSettings &update) {
  }

  //@description A notification was changed @notification_group_id Unique notification group identifier @notification Changed notification
  //updateNotification notification_group_id : int32 notification : notification = Update;
  void process_update(td::td_api::updateNotification &update) {
  }

  //@description A list of active notifications in a notification group has changed
  //@notification_group_id Unique notification group identifier
  //@type New type of the notification group
  //@chat_id Identifier of a chat to which all notifications in the group belong
  //@notification_settings_chat_id Chat identifier, which notification settings must be applied to the added notifications
  //@is_silent True, if the notifications should be shown without sound
  //@total_count Total number of unread notifications in the group, can be bigger than number of active notifications
  //@added_notifications List of added group notifications, sorted by notification ID @removed_notification_ids Identifiers of removed group notifications, sorted by notification ID
  //updateNotificationGroup notification_group_id : int32 type : NotificationGroupType chat_id
  //    : int53 notification_settings_chat_id : int53 is_silent : Bool total_count : int32 added_notifications
  //    : vector<notification>
  //          removed_notification_ids : vector<int32> = Update;
  void process_update(td::td_api::updateNotificationGroup &update) {
  }

  //@description Contains active notifications that was shown on previous application launches. This update is sent only if a message database is used. In that case it comes once before any updateNotification and updateNotificationGroup update @groups Lists of active notification groups
  //updateActiveNotifications groups : vector<notificationGroup> = Update;
  void process_update(td::td_api::updateActiveNotifications &update) {
  }

  //@description Describes, whether there are some pending notification updates. Can be used to prevent application from killing, while there are some pending notifications
  //@have_delayed_notifications True, if there are some delayed notification updates, which will be sent soon
  //@have_unreceived_notifications True, if there can be some yet unreceived notifications, which are being fetched from the server
  //updateHavePendingNotifications have_delayed_notifications : Bool have_unreceived_notifications : Bool = Update;
  void process_update(td::td_api::updateHavePendingNotifications &update) {
  }

  //@description Some messages were deleted @chat_id Chat identifier @message_ids Identifiers of the deleted messages
  //@is_permanent True, if the messages are permanently deleted by a user (as opposed to just becoming inaccessible)
  //@from_cache True, if the messages are deleted only from the cache and can possibly be retrieved again in the future
  //updateDeleteMessages chat_id : int53 message_ids : vector<int53> is_permanent : Bool from_cache : Bool = Update;
  void process_update(td::td_api::updateDeleteMessages &update) {
  }

  //@description A message sender activity in the chat has changed @chat_id Chat identifier @message_thread_id If not 0, a message thread identifier in which the action was performed @sender_id Identifier of a message sender performing the action @action The action
  //updateChatAction chat_id:int53 message_thread_id:int53 sender_id:MessageSender action:ChatAction = Update;
  void process_update(td::td_api::updateChatAction &update) {
  }

  //@description The user went online or offline @user_id User identifier @status New status of the user
  //updateUserStatus user_id : int32 status : UserStatus = Update;
  void process_update(td::td_api::updateUserStatus &update) {
    dialog_list_window()->process_user_update(update);
  }

  //@description Some data of a user has changed. This update is guaranteed to come before the user identifier is returned to the client @user New data about the user
  //updateUser user : user = Update;
  void process_update(td::td_api::updateUser &update) {
    dialog_list_window()->process_user_update(update);
  }

  //@description Some data of a basic group has changed. This update is guaranteed to come before the basic group identifier is returned to the client @basic_group New data about the group
  //updateBasicGroup basic_group : basicGroup = Update;
  void process_update(td::td_api::updateBasicGroup &update) {
    //ChatManager::instance->process_update(update);
  }

  //@description Some data of a supergroup or a channel has changed. This update is guaranteed to come before the supergroup identifier is returned to the client @supergroup New data about the supergroup
  //updateSupergroup supergroup : supergroup = Update;
  void process_update(td::td_api::updateSupergroup &update) {
    //ChatManager::instance->process_update(update);
  }

  //@description Some data of a secret chat has changed. This update is guaranteed to come before the secret chat identifier is returned to the client @secret_chat New data about the secret chat
  //updateSecretChat secret_chat : secretChat = Update;
  void process_update(td::td_api::updateSecretChat &update) {
    //ChatManager::instance->process_update(update);
  }

  //@description Some data from userFullInfo has been changed @user_id User identifier @user_full_info New full information about the user
  //updateUserFullInfo user_id : int32 user_full_info : userFullInfo = Update;
  void process_update(td::td_api::updateUserFullInfo &update) {
    //ChatManager::instance->process_update(update);
  }

  //@description Some data from basicGroupFullInfo has been changed @basic_group_id Identifier of a basic group @basic_group_full_info New full information about the group
  //updateBasicGroupFullInfo basic_group_id : int32 basic_group_full_info : basicGroupFullInfo = Update;
  void process_update(td::td_api::updateBasicGroupFullInfo &update) {
    //ChatManager::instance->process_update(update);
  }

  //@description Some data from supergroupFullInfo has been changed @supergroup_id Identifier of the supergroup or channel @supergroup_full_info New full information about the supergroup
  //updateSupergroupFullInfo supergroup_id : int32 supergroup_full_info : supergroupFullInfo = Update;
  void process_update(td::td_api::updateSupergroupFullInfo &update) {
    //ChatManager::instance->process_update(update);
  }

  //@description Service notification from the server. Upon receiving this the client must show a popup with the content of the notification
  //@type Notification type. If type begins with "AUTH_KEY_DROP_", then two buttons "Cancel" and "Log out" should be shown under notification; if user presses the second, all local data should be destroyed using Destroy method
  //@content Notification content
  //updateServiceNotification type : string content : MessageContent = Update;
  void process_update(td::td_api::updateServiceNotification &update) {
    /*Outputter out;
    out << color_scheme.get_color(ObjectType::ServiceNotification) << update.content_ << td::TerminalColors::empty()
        << "\n";
    td::TerminalIO::out() << td::CSlice{out};*/
  }

  //@description Information about a file was updated @file New data about the file
  //updateFile file : file = Update;
  void process_update(td::td_api::updateFile &update) {
    auto c = chat_window();
    if (c) {
      c->process_update(update);
    }
  }

  //@description The file generation process needs to be started by the client
  //@generation_id Unique identifier for the generation process
  //@original_path The path to a file from which a new file is generated; may be empty
  //@destination_path The path to a file that should be created and where the new file should be generated
  //@conversion String specifying the conversion applied to the original file. If conversion is "#url#" than original_path contains an HTTP/HTTPS URL of a file, which should be downloaded by the client
  //updateFileGenerationStart generation_id : int64 original_path : string destination_path : string conversion
  //    : string = Update;
  void process_update(td::td_api::updateFileGenerationStart &update) {
  }

  //@description File generation is no longer needed @generation_id Unique identifier for the generation process
  //updateFileGenerationStop generation_id : int64 = Update;
  void process_update(td::td_api::updateFileGenerationStop &update) {
  }

  //@description The state of the file download list has changed
  //@total_size Total size of files in the file download list, in bytes
  //@total_count Total number of files in the file download list
  //@downloaded_size Total downloaded size of files in the file download list, in bytes
  //updateFileDownloads total_size:int53 total_count:int32 downloaded_size:int53 = Update;
  void process_update(td::td_api::updateFileDownloads &update) {
  }

  //@description A file was added to the file download list. This update is sent only after file download list is loaded for the first time @file_download The added file download @counts New number of being downloaded and recently downloaded files found
  //updateFileAddedToDownloads file_download:fileDownload counts:downloadedFileCounts = Update;
  void process_update(td::td_api::updateFileAddedToDownloads &update) {
  }

  //@description A file download was changed. This update is sent only after file download list is loaded for the first time @file_id File identifier
  //@complete_date Point in time (Unix timestamp) when the file downloading was completed; 0 if the file downloading isn't completed
  //@is_paused True, if downloading of the file is paused
  //@counts New number of being downloaded and recently downloaded files found
  //updateFileDownload file_id:int32 complete_date:int32 is_paused:Bool counts:downloadedFileCounts = Update;
  void process_update(td::td_api::updateFileDownload &update) {
  }

  //@description A file was removed from the file download list. This update is sent only after file download list is loaded for the first time @file_id File identifier @counts New number of being downloaded and recently downloaded files found
  //updateFileRemovedFromDownloads file_id:int32 counts:downloadedFileCounts = Update;
  void process_update(td::td_api::updateFileRemovedFromDownloads &update) {
  }

  //@description New call was created or information about a call was updated @call New data about a call
  //updateCall call : call = Update;
  void process_update(td::td_api::updateCall &update) {
  }

  //@description Information about a group call was updated @group_call New data about a group call
  //updateGroupCall group_call:groupCall = Update;
  void process_update(td::td_api::updateGroupCall &update) {
  }

  //@description Information about a group call participant was changed. The updates are sent only after the group call is received through getGroupCall and only if the call is joined or being joined
  //@group_call_id Identifier of group call @participant New data about a participant
  //updateGroupCallParticipant group_call_id:int32 participant:groupCallParticipant = Update;
  void process_update(td::td_api::updateGroupCallParticipant &update) {
  }

  //@description New call signaling data arrived @call_id The call identifier @data The data
  //updateNewCallSignalingData call_id:int32 data:bytes = Update;
  void process_update(td::td_api::updateNewCallSignalingData &update) {
  }

  //@description Some privacy setting rules have been changed @setting The privacy setting @rules New privacy rules
  //updateUserPrivacySettingRules setting : UserPrivacySetting rules : userPrivacySettingRules = Update;
  void process_update(td::td_api::updateUserPrivacySettingRules &update) {
  }

  //@description Number of unread messages has changed. This update is sent only if a message database is used @unread_count Total number of unread messages @unread_unmuted_count Total number of unread messages in unmuted chats
  //updateUnreadMessageCount unread_count : int32 unread_unmuted_count : int32 = Update;
  void process_update(td::td_api::updateUnreadMessageCount &update) {
  }

  //@description Number of unread chats, i.e. with unread messages or marked as unread, has changed. This update is sent only if a message database is used
  //@unread_count Total number of unread chats
  //@unread_unmuted_count Total number of unread unmuted chats
  //@marked_as_unread_count Total number of chats marked as unread
  //@marked_as_unread_unmuted_count Total number of unmuted chats marked as unread
  //updateUnreadChatCount unread_count : int32 unread_unmuted_count : int32 marked_as_unread_count
  //    : int32 marked_as_unread_unmuted_count : int32 = Update;
  void process_update(td::td_api::updateUnreadChatCount &update) {
    /*unread_chats_ = update.unread_unmuted_count_ + update.marked_as_unread_unmuted_count_;
    update_prompt();*/
  }

  //@description An option changed its value @name The option name @value The new option value
  //updateOption name : string value : OptionValue = Update;
  void process_update(td::td_api::updateOption &update) {
  }

  //@description A sticker set has changed @sticker_set The sticker set
  //updateStickerSet sticker_set:stickerSet = Update;
  void process_update(td::td_api::updateStickerSet &update) {
  }

  //@description The list of installed sticker sets was updated @is_masks True, if the list of installed mask sticker sets was updated @sticker_set_ids The new list of installed ordinary sticker sets
  //updateInstalledStickerSets is_masks : Bool sticker_set_ids : vector<int64> = Update;
  void process_update(td::td_api::updateInstalledStickerSets &update) {
  }

  //@description The list of trending sticker sets was updated or some of them were viewed @sticker_sets The new list of trending sticker sets
  //updateTrendingStickerSets sticker_sets : stickerSets = Update;
  void process_update(td::td_api::updateTrendingStickerSets &update) {
  }

  //@description The list of recently used stickers was updated @is_attached True, if the list of stickers attached to photo or video files was updated, otherwise the list of sent stickers is updated @sticker_ids The new list of file identifiers of recently used stickers
  //updateRecentStickers is_attached : Bool sticker_ids : vector<int32> = Update;
  void process_update(td::td_api::updateRecentStickers &update) {
  }

  //@description The list of favorite stickers was updated @sticker_ids The new list of file identifiers of favorite stickers
  //updateFavoriteStickers sticker_ids : vector<int32> = Update;
  void process_update(td::td_api::updateFavoriteStickers &update) {
  }

  //@description The list of saved animations was updated @animation_ids The new list of file identifiers of saved animations
  //updateSavedAnimations animation_ids : vector<int32> = Update;
  void process_update(td::td_api::updateSavedAnimations &update) {
  }

  //@description The list of saved notifications sounds was updated. This update may not be sent until information about a notification sound was requested for the first time @notification_sound_ids The new list of identifiers of saved notification sounds
  //updateSavedNotificationSounds notification_sound_ids:vector<int64> = Update;
  void process_update(td::td_api::updateSavedNotificationSounds &update) {
  }

  //@description The selected background has changed @for_dark_theme True, if background for dark theme has changed @background The new selected background; may be null
  //updateSelectedBackground for_dark_theme:Bool background:background = Update;
  void process_update(td::td_api::updateSelectedBackground &update) {
  }

  //@description The list of available chat themes has changed @chat_themes The new list of chat themes
  //updateChatThemes chat_themes:vector<chatTheme> = Update;
  void process_update(td::td_api::updateChatThemes &update) {
  }

  //@description Some language pack strings have been updated @localization_target Localization target to which the language pack belongs @language_pack_id Identifier of the updated language pack @strings List of changed language pack strings
  //updateLanguagePackStrings localization_target : string language_pack_id : string strings
  //    : vector<languagePackString> = Update;
  void process_update(td::td_api::updateLanguagePackStrings &update) {
  }

  //@description The connection state has changed @state The new connection state
  //updateConnectionState state : ConnectionState = Update;
  void process_update(td::td_api::updateConnectionState &update) {
    td::td_api::downcast_call(
        *update.state_,
        td::overloaded(
            [&](td::td_api::connectionStateReady &state) { conn_state_ = ""; },
            [&](td::td_api::connectionStateConnectingToProxy &state) { conn_state_ = "connecting to proxy"; },
            [&](td::td_api::connectionStateConnecting &state) { conn_state_ = "connecting"; },
            [&](td::td_api::connectionStateUpdating &state) { conn_state_ = "updating"; },
            [&](td::td_api::connectionStateWaitingForNetwork &state) { conn_state_ = "waitnet"; }));
  }

  //@description New terms of service must be accepted by the user. If the terms of service are declined, then the deleteAccount method should be called with the reason "Decline ToS update" @terms_of_service_id Identifier of the terms of service @terms_of_service The new terms of service
  //updateTermsOfService terms_of_service_id : string terms_of_service : termsOfService = Update;
  void process_update(td::td_api::updateTermsOfService &update) {
    /*Outputter out;
    out << "UPDATED TERMS OF SERVICE:\n"
        << color_scheme.get_color(ObjectType::UrgentServiceNotification) << update.terms_of_service_->text_->text_
        << td::TerminalColors::empty() << "\n";
    td::TerminalIO::out() << td::CSlice{out};*/
  }

  //@description List of users nearby has changed. The update is sent only 60 seconds after a successful searchChatsNearby request @users_nearby The new list of users nearby
  void process_update(td::td_api::updateUsersNearby &update) {
  }

  //@description The list of bots added to attachment menu has changed @bots The new list of bots added to attachment menu. The bots must be shown in attachment menu only in private chats. The bots must not be shown on scheduled messages screen
  //updateAttachmentMenuBots bots:vector<attachmentMenuBot> = Update;
  void process_update(td::td_api::updateAttachmentMenuBots &update) {
  }

  //@description A message was sent by an opened web app, so the web app needs to be closed @web_app_launch_id Identifier of web app launch
  //updateWebAppMessageSent web_app_launch_id:int64 = Update;
  void process_update(td::td_api::updateWebAppMessageSent &update) {
  }

  //@description The list of supported reactions has changed @reactions The new list of supported reactions
  //updateReactions reactions:vector<reaction> = Update;
  void process_update(td::td_api::updateReactions &update) {
  }

  //@description The list of supported dice emojis has changed @emojis The new list of supported dice emojis
  //updateDiceEmojis emojis:vector<string> = Update;
  void process_update(td::td_api::updateDiceEmojis &update) {
  }

  //@description Some animated emoji message was clicked and a big animated sticker must be played if the message is visible on the screen. chatActionWatchingAnimations with the text of the message needs to be sent if the sticker is played
  //@chat_id Chat identifier @message_id Message identifier @sticker The animated sticker to be played
  //updateAnimatedEmojiMessageClicked chat_id:int53 message_id:int53 sticker:sticker = Update;
  void process_update(td::td_api::updateAnimatedEmojiMessageClicked &update) {
  }

  //@description The parameters of animation search through GetOption("animation_search_bot_username") bot has changed @provider Name of the animation search provider @emojis The new list of emojis suggested for searching
  //updateAnimationSearchParameters provider:string emojis:vector<string> = Update;
  void process_update(td::td_api::updateAnimationSearchParameters &update) {
  }

  //@description The list of suggested to the user actions has changed @added_actions Added suggested actions @removed_actions Removed suggested actions
  //updateSuggestedActions added_actions:vector<SuggestedAction> removed_actions:vector<SuggestedAction> = Update;
  void process_update(td::td_api::updateSuggestedActions &update) {
  }

  //@description A new incoming inline query; for bots only @id Unique query identifier @sender_user_id Identifier of the user who sent the query @user_location User location, provided by the client; may be null @query Text of the query @offset Offset of the first entry to return
  //updateNewInlineQuery id : int64 sender_user_id : int32 user_location : location query : string offset : string =
  //   Update;
  void process_update(td::td_api::updateNewInlineQuery &update) {
  }

  //@description The user has chosen a result of an inline query; for bots only @sender_user_id Identifier of the user who sent the query @user_location User location, provided by the client; may be null @query Text of the query @result_id Identifier of the chosen result @inline_message_id Identifier of the sent inline message, if known
  //updateNewChosenInlineResult sender_user_id : int32 user_location : location query : string result_id
  //    : string inline_message_id : string = Update;
  void process_update(td::td_api::updateNewChosenInlineResult &update) {
  }

  //@description A new incoming callback query; for bots only @id Unique query identifier @sender_user_id Identifier of the user who sent the query @chat_id Identifier of the chat, in which the query was sent
  //@message_id Identifier of the message, from which the query originated @chat_instance Identifier that uniquely corresponds to the chat to which the message was sent @payload Query payload
  //updateNewCallbackQuery id : int64 sender_user_id : int32 chat_id : int53 message_id : int53 chat_instance
  //    : int64 payload : CallbackQueryPayload = Update;
  void process_update(td::td_api::updateNewCallbackQuery &update) {
  }

  //@description A new incoming callback query from a message sent via a bot; for bots only @id Unique query identifier @sender_user_id Identifier of the user who sent the query @inline_message_id Identifier of the inline message, from which the query originated
  //@chat_instance An identifier uniquely corresponding to the chat a message was sent to @payload Query payload
  //updateNewInlineCallbackQuery id : int64 sender_user_id : int32 inline_message_id : string chat_instance
  //    : int64 payload : CallbackQueryPayload = Update;
  void process_update(td::td_api::updateNewInlineCallbackQuery &update) {
  }

  //@description A new incoming shipping query; for bots only. Only for invoices with flexible price @id Unique query identifier @sender_user_id Identifier of the user who sent the query @invoice_payload Invoice payload @shipping_address User shipping address
  //updateNewShippingQuery id : int64 sender_user_id : int32 invoice_payload : string shipping_address : address = Update;
  void process_update(td::td_api::updateNewShippingQuery &update) {
  }

  //@description A new incoming pre-checkout query; for bots only. Contains full information about a checkout @id Unique query identifier @sender_user_id Identifier of the user who sent the query @currency Currency for the product price @total_amount Total price for the product, in the minimal quantity of the currency
  //@invoice_payload Invoice payload @shipping_option_id Identifier of a shipping option chosen by the user; may be empty if not applicable @order_info Information about the order; may be null
  //updateNewPreCheckoutQuery id : int64 sender_user_id : int32 currency : string total_amount : int53 invoice_payload
  //    : bytes shipping_option_id : string order_info : orderInfo = Update;
  void process_update(td::td_api::updateNewPreCheckoutQuery &update) {
  }

  //@description A new incoming event; for bots only @event A JSON-serialized event
  //updateNewCustomEvent event : string = Update;
  void process_update(td::td_api::updateNewCustomEvent &update) {
  }

  //@description A new incoming query; for bots only @id The query identifier @data JSON-serialized query data @timeout Query timeout
  //updateNewCustomQuery id : int64 data : string timeout : int32 = Update;
  void process_update(td::td_api::updateNewCustomQuery &update) {
  }

  //@description Information about a poll was updated; for bots only @poll New data about the poll
  //updatePoll poll : poll = Update;
  void process_update(td::td_api::updatePoll &update) {
  }

  //@description A user changed the answer to a poll; for bots only @poll_id Unique poll identifier @user_id The user, who changed the answer to the poll @option_ids 0-based identifiers of answer options, chosen by the user
  void process_update(td::td_api::updatePollAnswer &update) {
  }

  //@description User rights changed in a chat; for bots only @chat_id Chat identifier @actor_user_id Identifier of the user, changing the rights
  //@date Point in time (Unix timestamp) when the user rights was changed @invite_link If user has joined the chat using an invite link, the invite link; may be null
  //@old_chat_member Previous chat member @new_chat_member New chat member
  //updateChatMember chat_id:int53 actor_user_id:int53 date:int32 invite_link:chatInviteLink old_chat_member:chatMember new_chat_member:chatMember = Update;
  void process_update(td::td_api::updateChatMember &update) {
  }

  //@description A user sent a join request to a chat; for bots only @chat_id Chat identifier @request Join request @invite_link The invite link, which was used to send join request; may be null
  //updateNewChatJoinRequest chat_id:int53 request:chatJoinRequest invite_link:chatInviteLink = Update;
  void process_update(td::td_api::updateNewChatJoinRequest &update) {
  }

  void do_send_request(td::tl_object_ptr<td::td_api::Function> func,
                       td::Promise<td::tl_object_ptr<td::td_api::Object>> cb) override {
    auto id = ++last_query_id_;
    handlers_.emplace(id, std::move(cb));
    td::send_closure(td_, &td::ClientActor::request, id, std::move(func));
  }

  void start_up() override {
  }

 private:
  std::map<td::uint64, td::Promise<td::tl_object_ptr<td::td_api::Object>>> handlers_;
  td::uint64 last_query_id_;
  std::string conn_state_ = "none ";
  //td::int32 unread_chats_{0};
};

void Tdcurses::start_curses(TdcursesParameters &params) {
  class Cb : public windows::Screen::Callback {
   public:
    Cb(td::ActorId<Tdcurses> self) : self_(self) {
    }

    void on_close() override {
      td::send_closure(self_, &Tdcurses::on_close);
    }

   private:
    td::ActorId<Tdcurses> self_;
  };
  auto cb = std::make_unique<Cb>(self_);
  screen_ = std::make_unique<windows::Screen>(std::move(cb));
  screen_->init();
  td::Scheduler::subscribe(td::Stdin().get_poll_info().extract_pollable_fd(this), td::PollFlags::Read());
  loop();
  layout_ = std::make_shared<Layout>(this, actor_id(this));
  log_window_ = std::make_shared<windows::LogWindow>();
  log_interface_ = std::make_unique<windows::WindowLogInterface<Tdcurses>>(log_window_, actor_id(this));
  dialog_list_window_ = std::make_shared<DialogListWindow>(this, actor_id(this));
  //td::log_interface = log_interface_.get();
  screen_->change_layout(layout_);
  layout_->replace_log_window(log_window_);
  layout_->replace_dialog_list_window(dialog_list_window_);
  layout_->initialize_sizes(params);
  screen_->refresh(true);

  initialize_options(params);

  auto config = std::make_shared<ConfigWindow>(this, actor_id(this), options_);
  config_window_ = std::make_shared<windows::BorderedWindow>(config, windows::Window::BorderType::Double);

  LOG(INFO) << "starting";
}

bool Tdcurses::window_exists(td::int64 id) {
  if (dialog_list_window_ && dialog_list_window_->window_unique_id() == id) {
    return true;
  }
  if (chat_window_ && chat_window_->window_unique_id() == id) {
    return true;
  }
  if (compose_window_ && compose_window_->window_unique_id() == id) {
    return true;
  }

  return false;
}

void Tdcurses::open_chat(td::int64 chat_id) {
  chat_window_ = std::make_shared<ChatWindow>(this, actor_id(this), chat_id);
  compose_window_ = nullptr;
  layout_->replace_chat_window(chat_window_);
  layout_->activate_window(chat_window_);
  layout_->replace_compose_window(nullptr);
}

void Tdcurses::open_compose_window() {
  if (!chat_window_) {
    return;
  }
  if (compose_window_) {
    return;
  }

  //ComposeWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id, std::string text)
  compose_window_ = std::make_shared<ComposeWindow>(this, actor_id(this), chat_window_->chat_id(),
                                                    chat_window_->draft_message_text());
  layout_->replace_compose_window(compose_window_);
  layout_->activate_window(compose_window_);
}

void Tdcurses::hide_config_window() {
  if (screen_->has_popup_window(config_window_.get())) {
    screen_->del_popup_window(config_window_.get());
  }
}

void Tdcurses::show_config_window() {
  if (!screen_->has_popup_window(config_window_.get())) {
    screen_->add_popup_window(config_window_, 3);
  }
}

void Tdcurses::close_compose_window() {
  if (compose_window_) {
    layout_->replace_compose_window(nullptr);
    compose_window_ = nullptr;
  }
}

void Tdcurses::loop() {
  td::sync_with_poll(td::Stdin());
  screen_->loop();
  auto t = screen_->need_refresh_at();
  if (t) {
    set_timeout_at(t.at());
  }
}

void Tdcurses::refresh() {
  screen_->refresh();
  auto t = screen_->need_refresh_at();
  if (t) {
    set_timeout_at(t.at());
  }
}

void Tdcurses::tear_down() {
  td::Scheduler::unsubscribe(td::Stdin().get_poll_info().get_pollable_fd_ref());
}

void Tdcurses::initialize_options(TdcursesParameters &params) {
  options_ = {{"log_window",
               {"enabled", "disabled"},
               [&](std::string val) {
                 if (val == "enabled") {
                   layout_->enable_log_window();
                 } else {
                   layout_->disable_log_window();
                 }
               },
               params.log_window_enabled ? 0U : 1U}};
}

}  // namespace tdcurses

void termination_signal_handler(int signum) {
  td::signal_safe_write_signal_number(signum);

#if HAVE_EXECINFO_H
  void *buffer[64];
  int nptrs = backtrace(buffer, 64);
  (void)write(2, "\n------- Stack Backtrace -------\n", 33);
  backtrace_symbols_fd(buffer, nptrs, 2);
  (void)write(2, "-------------------------------\n", 32);
#endif

  _exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  td::setup_signals_alt_stack().ensure();
  td::set_signal_handler(td::SignalType::Abort, termination_signal_handler).ensure();
  td::set_signal_handler(td::SignalType::Error, termination_signal_handler).ensure();
  ::signal(SIGCHLD, SIG_IGN);
  td::ignore_signal(td::SignalType::Pipe).ensure();

  SET_VERBOSITY_LEVEL(VERBOSITY_NAME(WARNING));

  td::ConcurrentScheduler scheduler;
  scheduler.init(4);

  /*auto gpl_notice = PSTRING() << "Telegram-cli " << TELEGRAM_CLI_VERSION << " Copyright (C) 2022\n"
                              << "This program comes with ABSOLUTELY NO WARRANTY; for details read GPL.\n"
                              << "This is free software, and you are welcome to redistribute it\n"
                              << "under certain conditions; read GPL for details.\n";

  auto version_notice = PSTRING() << "Telegram-cli version " << TELEGRAM_CLI_VERSION << " uses tdlib "
                                  << td::Td::TDLIB_VERSION << ", " << OPENSSL_VERSION_TEXT << ", libreadline "
                                  << RL_VERSION_MAJOR << "." << RL_VERSION_MINOR << ", libconfig++ "
                                  << LIBCONFIGXX_VER_MAJOR << "." << LIBCONFIGXX_VER_MINOR << "."
                                  << LIBCONFIGXX_VER_REVISION << "\n";

  std::cout << gpl_notice << std::flush;
  std::cout << version_notice << std::flush; */

  std::string config_file_name;

  td::OptionParser p;
  p.set_description("curses client for Telegram. Based on Tdlib");

  p.add_checked_option('v', "verbosity", "sets verbosity", [&](td::Slice arg) {
    auto verbosity = td::to_integer<int>(arg);
    SET_VERBOSITY_LEVEL(VERBOSITY_NAME(FATAL) + verbosity);
    return (verbosity >= 0 && verbosity <= 9) ? td::Status::OK() : td::Status::Error("verbosity must be 0..9");
  });

  p.add_checked_option('c', "config", "config file name", [&](td::Slice arg) {
    config_file_name = arg.str();
    return td::Status::OK();
  });

  auto S = p.run(argc, argv);
  if (S.is_error()) {
    std::cerr << "failed to parse options: " << S.move_as_error().to_string() << std::endl;
    return 1;
  }

  if (config_file_name.size() == 0) {
    config_file_name = tdcurses::detect_config_name();
  }

  std::string db_root;
  bool enable_storage_optimizer = true;
  td::int32 api_id = 2899;
  std::string api_hash = "36722c72256a24c1225de00eb6a1ca74";
  bool ignore_file_names = false;
  bool use_chat_info_database = true;
  bool use_file_database = true;
  bool use_message_database = true;
  bool use_secret_chats = true;
  bool use_test_dc = false;
  bool vs16_makes_wide = true;

  bool log_window_enabled = true;
  td::int32 dialog_list_window_width = 10;
  td::int32 log_window_height = 10;
  td::int32 compose_window_height = 10;

  struct CodepointInfo {
    CodepointInfo(td::uint32 begin, td::uint32 end, td::int32 width, std::string terms)
        : begin(begin), end(end), width(width), terms(terms) {
    }
    td::uint32 begin;
    td::uint32 end;
    td::int32 width;
    std::string terms;
  };
  std::vector<CodepointInfo> force_wide_codepoints{{0x1F100, 0x1F1FF, 2, "*"}};

  auto r = ::access(config_file_name.c_str(), F_OK);
  if (r < 0) {
    libconfig::Config conf;
    auto &root = conf.getRoot();
    auto &td = root.add("td", libconfig::Setting::Type::TypeGroup);
    db_root = tdcurses::detect_db_root_name();
    td.add("db_root", libconfig::Setting::TypeString) = db_root;
    td.add("enable_storage_optimizer", libconfig::Setting::TypeBoolean) = enable_storage_optimizer;
    td.add("api_id", libconfig::Setting::TypeInt) = api_id;
    td.add("api_hash", libconfig::Setting::TypeString) = api_hash;
    td.add("ignore_file_names", libconfig::Setting::TypeBoolean) = ignore_file_names;
    td.add("use_chat_info_database", libconfig::Setting::TypeBoolean) = use_chat_info_database;
    td.add("use_file_database", libconfig::Setting::TypeBoolean) = use_file_database;
    td.add("use_message_database", libconfig::Setting::TypeBoolean) = use_message_database;
    td.add("use_secret_chats", libconfig::Setting::TypeBoolean) = use_secret_chats;
    td.add("use_test_dc", libconfig::Setting::TypeBoolean) = use_test_dc;
    auto &iface = root.add("iface", libconfig::Setting::Type::TypeGroup);
    iface.add("log_window_enabled", libconfig::Setting::TypeBoolean) = log_window_enabled;
    iface.add("dialog_list_window_width", libconfig::Setting::TypeInt) = dialog_list_window_width;
    iface.add("log_window_height", libconfig::Setting::TypeInt) = log_window_height;
    iface.add("compose_window_height", libconfig::Setting::TypeInt) = compose_window_height;
    auto &utf8 = root.add("utf8", libconfig::Setting::Type::TypeGroup);
    utf8.add("vs16_makes_wide", libconfig::Setting::TypeBoolean) = vs16_makes_wide;
    auto &widecode = utf8.add("codepoints_override", libconfig::Setting::TypeList);
    for (auto &l : force_wide_codepoints) {
      auto &g = widecode.add(libconfig::Setting::TypeGroup);
      g.add("first", libconfig::Setting::TypeInt) = (td::int32)l.begin;
      g.add("last", libconfig::Setting::TypeInt) = (td::int32)l.end;
      g.add("width", libconfig::Setting::TypeInt) = (td::int32)l.width;
      g.add("terms", libconfig::Setting::TypeString) = l.terms;
    }

    conf.writeFile(config_file_name.c_str());
  }

  libconfig::Config config;

  try {
    config.readFile(config_file_name.c_str());
  } catch (libconfig::FileIOException &e) {
    LOG(FATAL) << "failed to read from config file '" << config_file_name << "'";
  } catch (libconfig::ParseException &pex) {
    LOG(FATAL) << "failed to parse config '" << config_file_name << "' at " << pex.getFile() << ":" << pex.getLine()
               << " - " << pex.getError();
  }

  config.lookupValue("db_root", db_root);
  if (db_root == "") {
    db_root = tdcurses::detect_db_root_name();
  }
  config.lookupValue("enable_storage_optimizer", enable_storage_optimizer);
  config.lookupValue("app_id", api_id);
  config.lookupValue("api_hash", api_hash);
  config.lookupValue("ignore_file_names", ignore_file_names);
  config.lookupValue("use_chat_info_database", use_chat_info_database);
  config.lookupValue("use_file_database", use_file_database);
  config.lookupValue("use_message_database", use_message_database);
  config.lookupValue("use_secret_chats", use_secret_chats);
  config.lookupValue("use_test_dc", use_test_dc);
  config.lookupValue("utf8.vs16_makes_wide", vs16_makes_wide);

  config.lookupValue("iface.log_window_enabled", log_window_enabled);
  config.lookupValue("iface.dialog_list_window_width", dialog_list_window_width);
  config.lookupValue("iface.log_window_height", log_window_height);
  config.lookupValue("iface.compose_window_height", compose_window_height);

  [&]() {
    auto &root = config.getRoot();
    if (!root.exists("utf8")) {
      return;
    }
    auto &utf8 = root["utf8"];
    if (utf8.getType() != libconfig::Setting::Type::TypeGroup) {
      return;
    }
    if (!utf8.exists("codepoints_override")) {
      return;
    }
    auto &wc = utf8["codepoints_override"];
    if (wc.getType() != libconfig::Setting::Type::TypeList) {
      return;
    }
    std::vector<CodepointInfo> r;
    for (auto &x : wc) {
      if (!x.exists("first") || !x.exists("last") || !x.exists("width") || !x.exists("terms")) {
        continue;
      }
      td::int32 begin, end, width;
      std::string term;
      if (!x.lookupValue("first", begin)) {
        continue;
      }
      if (!x.lookupValue("last", end)) {
        continue;
      }
      if (!x.lookupValue("width", width)) {
        continue;
      }
      if (!x.lookupValue("terms", term)) {
        continue;
      }
      if (begin < 0 || begin > end) {
        continue;
      }
      if (width < 0 || width > 2) {
        continue;
      }
      r.emplace_back(begin, end, width, term);
    }
    force_wide_codepoints = std::move(r);
  }();

  if (vs16_makes_wide) {
    enable_wide_emojis();
  }

  if (force_wide_codepoints.size() > 0) {
    std::vector<UnicodeWidthBlock> b;
    auto term = getenv("TERM");
    for (auto &x : force_wide_codepoints) {
      if (x.terms == "*" || x.terms.find(term) != std::string::npos) {
        b.emplace_back(x.begin, x.end, x.width);
      }
    }
    override_unicode_width(std::move(b));
  }

  auto log_dir = db_root + "logs/";
  td::mkdir(log_dir).ensure();
  auto R = td::FileLog::create(log_dir + "log.txt");
  auto f = R.move_as_ok();
  td::log_interface = f.get();

  auto tdlib_parameters = td::td_api::make_object<td::td_api::tdlibParameters>();
  tdlib_parameters->api_hash_ = api_hash;
  tdlib_parameters->api_id_ = api_id;
  tdlib_parameters->application_version_ = TELEGRAM_CURSES_VERSION;
  tdlib_parameters->database_directory_ = db_root + "db/";
  tdlib_parameters->device_model_ = "Console";
  tdlib_parameters->enable_storage_optimizer_ = enable_storage_optimizer;
  tdlib_parameters->files_directory_ = db_root + "files/";
  tdlib_parameters->ignore_file_names_ = ignore_file_names;
  tdlib_parameters->system_language_code_ = "EN";
  tdlib_parameters->system_version_ = "Unix";
  tdlib_parameters->use_chat_info_database_ = use_chat_info_database;
  tdlib_parameters->use_file_database_ = use_file_database;
  tdlib_parameters->use_message_database_ = use_message_database;
  tdlib_parameters->use_secret_chats_ = use_secret_chats;
  tdlib_parameters->use_test_dc_ = use_test_dc;

  auto tdcurses_params = std::make_unique<tdcurses::TdcursesParameters>();
  tdcurses_params->log_window_enabled = log_window_enabled;
  tdcurses_params->dialog_list_window_width = dialog_list_window_width;
  tdcurses_params->log_window_height = log_window_height;
  tdcurses_params->compose_window_height = compose_window_height;

  td::ActorOwn<tdcurses::TdcursesImpl> act;
  act = scheduler.create_actor_unsafe<tdcurses::TdcursesImpl>(1, "CliClient");
  scheduler.start();
  {
    auto guard = scheduler.get_main_guard();
    td::send_closure_later(act.get(), &tdcurses::TdcursesImpl::start, std::move(tdlib_parameters),
                           std::move(tdcurses_params));
  }
  while (scheduler.run_main(100)) {
  }
  scheduler.finish();

  return 0;
}
