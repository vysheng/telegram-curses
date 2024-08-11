#include "td/actor/impl/ActorId.h"
#include "td/actor/impl/Scheduler-decl.h"
#include "td/actor/impl/Scheduler.h"
#include "td/tdactor/td/actor/actor.h"
#include "td/tdutils/td/utils/int_types.h"
#include "td/telegram/StickerType.h"
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

#include "windows/EditorWindow.hpp"
#include "windows/Markup.hpp"
#include "windows/unicode.h"

//#include "telegram-cli-output.h"
#include "StickerManager.hpp"
#include "Tdcurses.hpp"
#include "Outputter.hpp"
#include "TdObjectsOutput.h"
#include "ChatSearchWindow.hpp"
#include "ChatWindow.hpp"
#include "CommandLineWindow.hpp"
#include "ComposeWindow.hpp"
#include "ConfigWindow.hpp"
#include "DialogListWindow.hpp"
#include "InfoWindow.hpp"
#include "StatusLineWindow.hpp"
#include "TdcursesLayout.hpp"
#include "windows/LogWindow.hpp"
#include "GlobalParameters.hpp"

#include "qrcodegen/qrcodegen.hpp"

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
  td::tl_object_ptr<td::td_api::setTdlibParameters> tdlib_parameters_;
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
        LOG(ERROR) << "received error: " << error->code_ << " " << error->message_;
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

  auto clone_tdlib_parameters() const {
    //setTdlibParameters use_test_dc:Bool database_directory:string files_directory:string database_encryption_key:bytes use_file_database:Bool use_chat_info_database:Bool use_message_database:Bool use_secret_chats:Bool api_id:int32 api_hash:string system_language_code:string device_model:string system_version:string application_version:string = Ok;
    return td::make_tl_object<td::td_api::setTdlibParameters>(
        tdlib_parameters_->use_test_dc_, tdlib_parameters_->database_directory_, tdlib_parameters_->files_directory_,
        tdlib_parameters_->database_encryption_key_, tdlib_parameters_->use_file_database_,
        tdlib_parameters_->use_chat_info_database_, tdlib_parameters_->use_message_database_,
        tdlib_parameters_->use_secret_chats_, tdlib_parameters_->api_id_, tdlib_parameters_->api_hash_,
        tdlib_parameters_->system_language_code_, tdlib_parameters_->device_model_, tdlib_parameters_->system_version_,
        tdlib_parameters_->application_version_);
  }

  void change_system_version(std::string version) {
    CHECK(!starting_);
    tdlib_parameters_->system_version_ = version;
  }

  void start(td::tl_object_ptr<td::td_api::setTdlibParameters> tdlib_parameters,
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
    send_request(clone_tdlib_parameters(),
                 td::PromiseCreator::lambda([self = self_](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
                   if (R.is_error()) {
                     CHECK(R.error().code() == 401);
                     td::send_closure(self, &TdcursesImpl::request_database_password);
                   }
                 }));
  }

  void received_database_password(td::Result<td::BufferSlice> res) {
    if (res.is_error()) {
      request_database_password();
    } else {
      auto params = clone_tdlib_parameters();
      auto key = res.move_as_ok();
      params->database_encryption_key_ = std::string(key.data(), key.size());
      send_request(std::move(params),
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

  //@description TDLib needs the user's email address to authorize. Call setAuthenticationEmailAddress to provide the email address, or directly call checkAuthenticationEmailCode with Apple ID/Google ID token if allowed
  //@allow_apple_id True, if authorization through Apple ID is allowed
  //@allow_google_id True, if authorization through Google ID is allowed
  void process_auth_state(td::td_api::authorizationStateWaitEmailAddress &state) {
    UNREACHABLE();
  }

  //@description TDLib needs the user's authentication code sent to an email address to authorize. Call checkAuthenticationEmailCode to provide the code
  //@allow_apple_id True, if authorization through Apple ID is allowed
  //@allow_google_id True, if authorization through Google ID is allowed
  //@code_info Information about the sent authentication code
  //@next_phone_number_authorization_date Point in time (Unix timestamp) when the user will be able to authorize with a code sent to the user's phone number; 0 if unknown
  //authorizationStateWaitEmailCode allow_apple_id:Bool allow_google_id:Bool code_info:emailAddressAuthenticationCodeInfo next_phone_number_authorization_date:int32 = AuthorizationState;
  void process_auth_state(td::td_api::authorizationStateWaitEmailCode &state) {
    UNREACHABLE();
  }

  void received_phone_number(td::Result<td::BufferSlice> res) {
    if (res.is_error()) {
      request_phone_number();
    } else {
      //phoneNumberAuthenticationSettings allow_flash_call:Bool allow_missed_call:Bool is_current_phone_number:Bool has_unknown_phone_number:Bool allow_sms_retriever_api:Bool firebase_authentication_settings:FirebaseAuthenticationSettings authentication_tokens:vector<string> = PhoneNumberAuthenticationSettings;
      send_request(td::make_tl_object<td::td_api::setAuthenticationPhoneNumber>(
                       res.move_as_ok().as_slice().str(),
                       td::make_tl_object<td::td_api::phoneNumberAuthenticationSettings>(
                           /* allow flash call */ true, /* allow missed call */ true,
                           /* is current phone number */ false, /* has unknown phone number */ false,
                           /* allow sms retriever */ false, /* firebase */ nullptr,
                           /* tokens */ std::vector<std::string>())),
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
      send_request(td::make_tl_object<td::td_api::registerUser>(first_name, last_name, false),
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

  //@description The user authorization state has changed
  //@authorization_state New authorization state
  //updateAuthorizationState authorization_state:AuthorizationState = Update;
  void process_update(td::td_api::updateAuthorizationState &update) {
    del_qr_code_window();
    td::td_api::downcast_call(*update.authorization_state_, [&](auto &obj) { process_auth_state(obj); });
  }

  //@description A new message was received; can also be an outgoing message
  //@message The new message
  //updateNewMessage message:message = Update;
  void process_update(td::td_api::updateNewMessage &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.message_->chat_id_) {
      c->process_update(update);
    }
  }

  //@description A request to send a message has reached the Telegram server. This doesn't mean that the message will be sent successfully.
  //-This update is sent only if the option "use_quick_ack" is set to true. This update may be sent multiple times for the same message
  //@chat_id The chat identifier of the sent message
  //@message_id A temporary message identifier
  //updateMessageSendAcknowledged chat_id:int53 message_id:int53 = Update;
  void process_update(td::td_api::updateMessageSendAcknowledged &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message has been successfully sent
  //@message The sent message. Usually only the message identifier, date, and content are changed, but almost all other fields can also change
  //@old_message_id The previous temporary message identifier
  //updateMessageSendSucceeded message:message old_message_id:int53 = Update;
  void process_update(td::td_api::updateMessageSendSucceeded &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.message_->chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message failed to send. Be aware that some messages being sent can be irrecoverably deleted, in which case updateDeleteMessages will be received instead of this update
  //@message The failed to send message
  //@old_message_id The previous temporary message identifier
  //@error The cause of the message sending failure
  //updateMessageSendFailed message:message old_message_id:int53 error:error = Update;
  void process_update(td::td_api::updateMessageSendFailed &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.message_->chat_id_) {
      c->process_update(update);
    }
  }

  //@description The message content has changed
  //@chat_id Chat identifier
  //@message_id Message identifier
  //@new_content New message content
  //updateMessageContent chat_id:int53 message_id:int53 new_content:MessageContent = Update;
  void process_update(td::td_api::updateMessageContent &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message was edited. Changes in the message content will come in a separate updateMessageContent
  //@chat_id Chat identifier
  //@message_id Message identifier
  //@edit_date Point in time (Unix timestamp) when the message was edited
  //@reply_markup New message reply markup; may be null
  //updateMessageEdited chat_id:int53 message_id:int53 edit_date:int32 reply_markup:ReplyMarkup = Update;
  void process_update(td::td_api::updateMessageEdited &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description The message pinned state was changed
  //@chat_id Chat identifier @message_id The message identifier
  //@is_pinned True, if the message is pinned
  //updateMessageIsPinned chat_id:int53 message_id:int53 is_pinned:Bool = Update;
  void process_update(td::td_api::updateMessageIsPinned &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description The information about interactions with a message has changed
  //@chat_id Chat identifier
  //@message_id Message identifier
  //@interaction_info New information about interactions with the message; may be null
  //updateMessageInteractionInfo chat_id:int53 message_id:int53 interaction_info:messageInteractionInfo = Update;
  void process_update(td::td_api::updateMessageInteractionInfo &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description The message content was opened. Updates voice note messages to "listened", video note messages to "viewed" and starts the self-destruct timer
  //@chat_id Chat identifier
  //@message_id Message identifier
  //updateMessageContentOpened chat_id:int53 message_id:int53 = Update;
  void process_update(td::td_api::updateMessageContentOpened &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message with an unread mention was read
  //@chat_id Chat identifier
  //@message_id Message identifier
  //@unread_mention_count The new number of unread mention messages left in the chat
  //updateMessageMentionRead chat_id:int53 message_id:int53 unread_mention_count:int32 = Update;
  void process_update(td::td_api::updateMessageMentionRead &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description The list of unread reactions added to a message was changed
  //@chat_id Chat identifier
  //@message_id Message identifier
  //@unread_reactions The new list of unread reactions
  //@unread_reaction_count The new number of messages with unread reactions left in the chat
  //updateMessageUnreadReactions chat_id:int53 message_id:int53 unread_reactions:vector<unreadReaction> unread_reaction_count:int32 = Update;
  void process_update(td::td_api::updateMessageUnreadReactions &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A fact-check added to a message was changed
  //@chat_id Chat identifier
  //@message_id Message identifier
  //@fact_check The new fact-check
  //updateMessageFactCheck chat_id:int53 message_id:int53 fact_check:factCheck = Update;
  void process_update(td::td_api::updateMessageFactCheck &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message with a live location was viewed. When the update is received, the application is supposed to update the live location
  //@chat_id Identifier of the chat with the live location message
  //@message_id Identifier of the message with live location
  //updateMessageLiveLocationViewed chat_id:int53 message_id:int53 = Update;
  void process_update(td::td_api::updateMessageLiveLocationViewed &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A new chat has been loaded/created. This update is guaranteed to come before the chat identifier is returned to the application. The chat field changes will be reported through separate updates
  //@chat The chat
  //updateNewChat chat:chat = Update;
  void process_update(td::td_api::updateNewChat &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The title of a chat was changed
  //@chat_id Chat identifier
  //@title The new chat title
  //updateChatTitle chat_id:int53 title:string = Update;
  void process_update(td::td_api::updateChatTitle &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat photo was changed
  //@chat_id Chat identifier
  //@photo The new chat photo; may be null
  //updateChatPhoto chat_id:int53 photo:chatPhotoInfo = Update;
  void process_update(td::td_api::updateChatPhoto &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Chat accent colors have changed
  //@chat_id Chat identifier
  //@accent_color_id The new chat accent color identifier
  //@background_custom_emoji_id The new identifier of a custom emoji to be shown on the reply header and link preview background; 0 if none
  //@profile_accent_color_id The new chat profile accent color identifier; -1 if none
  //@profile_background_custom_emoji_id The new identifier of a custom emoji to be shown on the profile background; 0 if none
  //updateChatAccentColors chat_id:int53 accent_color_id:int32 background_custom_emoji_id:int64 profile_accent_color_id:int32 profile_background_custom_emoji_id:int64 = Update;
  void process_update(td::td_api::updateChatAccentColors &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Chat permissions were changed
  //@chat_id Chat identifier
  //@permissions The new chat permissions
  //updateChatPermissions chat_id:int53 permissions:chatPermissions = Update;
  void process_update(td::td_api::updateChatPermissions &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The last message of a chat was changed
  //@chat_id Chat identifier
  //@last_message The new last message in the chat; may be null if the last message became unknown. While the last message is unknown, new messages can be added to the chat without corresponding updateNewMessage update
  //@positions The new chat positions in the chat lists
  //updateChatLastMessage chat_id:int53 last_message:message positions:vector<chatPosition> = Update;
  void process_update(td::td_api::updateChatLastMessage &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The position of a chat in a chat list has changed. An updateChatLastMessage or updateChatDraftMessage update might be sent instead of the update
  //@chat_id Chat identifier
  //@position New chat position. If new order is 0, then the chat needs to be removed from the list
  //updateChatPosition chat_id:int53 position:chatPosition = Update;
  void process_update(td::td_api::updateChatPosition &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat was added to a chat list
  //@chat_id Chat identifier
  //@chat_list The chat list to which the chat was added
  //updateChatAddedToList chat_id:int53 chat_list:ChatList = Update;
  void process_update(td::td_api::updateChatAddedToList &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat was removed from a chat list
  //@chat_id Chat identifier
  //@chat_list The chat list from which the chat was removed
  //updateChatRemovedFromList chat_id:int53 chat_list:ChatList = Update;
  void process_update(td::td_api::updateChatRemovedFromList &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Incoming messages were read or the number of unread messages has been changed
  //@chat_id Chat identifier
  //@last_read_inbox_message_id Identifier of the last read incoming message
  //@unread_count The number of unread messages left in the chat
  //updateChatReadInbox chat_id:int53 last_read_inbox_message_id:int53 unread_count:int32 = Update;
  void process_update(td::td_api::updateChatReadInbox &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Outgoing messages were read
  //@chat_id Chat identifier
  //@last_read_outbox_message_id Identifier of last read outgoing message
  //updateChatReadOutbox chat_id:int53 last_read_outbox_message_id:int53 = Update;
  void process_update(td::td_api::updateChatReadOutbox &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat action bar was changed
  //@chat_id Chat identifier
  //@action_bar The new value of the action bar; may be null
  //updateChatActionBar chat_id:int53 action_bar:ChatActionBar = Update;
  void process_update(td::td_api::updateChatActionBar &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The bar for managing business bot was changed in a chat
  //@chat_id Chat identifier
  //@business_bot_manage_bar The new value of the business bot manage bar; may be null
  //updateChatBusinessBotManageBar chat_id:int53 business_bot_manage_bar:businessBotManageBar = Update;
  void process_update(td::td_api::updateChatBusinessBotManageBar &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat available reactions were changed
  //@chat_id Chat identifier @available_reactions The new reactions, available in the chat
  //updateChatAvailableReactions chat_id:int53 available_reactions:ChatAvailableReactions = Update;
  void process_update(td::td_api::updateChatAvailableReactions &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat draft has changed. Be aware that the update may come in the currently opened chat but with old content of the draft. If the user has changed the content of the draft, this update mustn't be applied
  //@chat_id Chat identifier
  //@draft_message The new draft message; may be null if none
  //@positions The new chat positions in the chat lists
  //updateChatDraftMessage chat_id:int53 draft_message:draftMessage positions:vector<chatPosition> = Update;
  void process_update(td::td_api::updateChatDraftMessage &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Chat emoji status has changed
  //@chat_id Chat identifier
  //@emoji_status The new chat emoji status; may be null
  //updateChatEmojiStatus chat_id:int53 emoji_status:emojiStatus = Update;
  void process_update(td::td_api::updateChatEmojiStatus &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The message sender that is selected to send messages in a chat has changed
  //@chat_id Chat identifier
  //@message_sender_id New value of message_sender_id; may be null if the user can't change message sender
  //updateChatMessageSender chat_id:int53 message_sender_id:MessageSender = Update;
  void process_update(td::td_api::updateChatMessageSender &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The message auto-delete or self-destruct timer setting for a chat was changed
  //@chat_id Chat identifier
  //@message_auto_delete_time New value of message_auto_delete_time
  //updateChatMessageAutoDeleteTime chat_id:int53 message_auto_delete_time:int32 = Update;
  void process_update(td::td_api::updateChatMessageAutoDeleteTime &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Notification settings for a chat were changed
  //@chat_id Chat identifier
  //@notification_settings The new notification settings
  //updateChatNotificationSettings chat_id:int53 notification_settings:chatNotificationSettings = Update;
  void process_update(td::td_api::updateChatNotificationSettings &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat pending join requests were changed
  //@chat_id Chat identifier
  //@pending_join_requests The new data about pending join requests; may be null
  //updateChatPendingJoinRequests chat_id:int53 pending_join_requests:chatJoinRequestsInfo = Update;
  void process_update(td::td_api::updateChatPendingJoinRequests &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The default chat reply markup was changed. Can occur because new messages with reply markup were received or because an old reply markup was hidden by the user
  //@chat_id Chat identifier
  //@reply_markup_message_id Identifier of the message from which reply markup needs to be used; 0 if there is no default custom reply markup in the chat
  //updateChatReplyMarkup chat_id:int53 reply_markup_message_id:int53 = Update;
  void process_update(td::td_api::updateChatReplyMarkup &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat background was changed
  //@chat_id Chat identifier
  //@background The new chat background; may be null if background was reset to default
  //updateChatBackground chat_id:int53 background:chatBackground = Update;
  void process_update(td::td_api::updateChatBackground &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat theme was changed
  //@chat_id Chat identifier
  //@theme_name The new name of the chat theme; may be empty if theme was reset to default
  //updateChatTheme chat_id:int53 theme_name:string = Update;
  void process_update(td::td_api::updateChatTheme &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat unread_mention_count has changed
  //@chat_id Chat identifier
  //@unread_mention_count The number of unread mention messages left in the chat
  //updateChatUnreadMentionCount chat_id:int53 unread_mention_count:int32 = Update;
  void process_update(td::td_api::updateChatUnreadMentionCount &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The chat unread_reaction_count has changed
  //@chat_id Chat identifier
  //@unread_reaction_count The number of messages with unread reactions left in the chat
  //updateChatUnreadReactionCount chat_id:int53 unread_reaction_count:int32 = Update;
  void process_update(td::td_api::updateChatUnreadReactionCount &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat video chat state has changed
  //@chat_id Chat identifier
  //@video_chat New value of video_chat
  //updateChatVideoChat chat_id:int53 video_chat:videoChat = Update;
  void process_update(td::td_api::updateChatVideoChat &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The value of the default disable_notification parameter, used when a message is sent to the chat, was changed
  //@chat_id Chat identifier
  //@default_disable_notification The new default_disable_notification value
  //updateChatDefaultDisableNotification chat_id:int53 default_disable_notification:Bool = Update;
  void process_update(td::td_api::updateChatDefaultDisableNotification &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat content was allowed or restricted for saving
  //@chat_id Chat identifier
  //@has_protected_content New value of has_protected_content
  //updateChatHasProtectedContent chat_id:int53 has_protected_content:Bool = Update;
  void process_update(td::td_api::updateChatHasProtectedContent &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Translation of chat messages was enabled or disabled
  //@chat_id Chat identifier
  //@is_translatable New value of is_translatable
  //updateChatIsTranslatable chat_id:int53 is_translatable:Bool = Update;
  void process_update(td::td_api::updateChatIsTranslatable &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat was marked as unread or was read
  //@chat_id Chat identifier
  //@is_marked_as_unread New value of is_marked_as_unread
  //updateChatIsMarkedAsUnread chat_id:int53 is_marked_as_unread:Bool = Update;
  void process_update(td::td_api::updateChatIsMarkedAsUnread &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat default appearance has changed
  //@chat_id Chat identifier
  //@view_as_topics New value of view_as_topics
  //updateChatViewAsTopics chat_id:int53 view_as_topics:Bool = Update;
  void process_update(td::td_api::updateChatViewAsTopics &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat was blocked or unblocked
  //@chat_id Chat identifier
  //@block_list Block list to which the chat is added; may be null if none
  //updateChatBlockList chat_id:int53 block_list:BlockList = Update;
  void process_update(td::td_api::updateChatBlockList &update) {
    dialog_list_window()->process_update(update);
  }

  //@description A chat's has_scheduled_messages field has changed
  //@chat_id Chat identifier
  //@has_scheduled_messages New value of has_scheduled_messages
  //updateChatHasScheduledMessages chat_id:int53 has_scheduled_messages:Bool = Update;
  void process_update(td::td_api::updateChatHasScheduledMessages &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The list of chat folders or a chat folder has changed
  //@chat_folders The new list of chat folders
  //@main_chat_list_position Position of the main chat list among chat folders, 0-based
  //@are_tags_enabled True, if folder tags are enabled
  //updateChatFolders chat_folders:vector<chatFolderInfo> main_chat_list_position:int32 are_tags_enabled:Bool = Update;
  void process_update(td::td_api::updateChatFolders &update) {
    global_parameters().process_update(update);
  }

  //@description The number of online group members has changed. This update with non-zero number of online group members is sent only for currently opened chats.
  //-There is no guarantee that it is sent just after the number of online users has changed
  //@chat_id Identifier of the chat
  //@online_member_count New number of online members in the chat, or 0 if unknown
  //updateChatOnlineMemberCount chat_id:int53 online_member_count:int32 = Update;
  void process_update(td::td_api::updateChatOnlineMemberCount &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Basic information about a Saved Messages topic has changed. This update is guaranteed to come before the topic identifier is returned to the application
  //@topic New data about the topic
  //updateSavedMessagesTopic topic:savedMessagesTopic = Update;
  void process_update(td::td_api::updateSavedMessagesTopic &update) {
  }

  //@description Number of Saved Messages topics has changed
  //@topic_count Approximate total number of Saved Messages topics
  //updateSavedMessagesTopicCount topic_count:int32 = Update;
  void process_update(td::td_api::updateSavedMessagesTopicCount &update) {
  }

  //@description Basic information about a quick reply shortcut has changed. This update is guaranteed to come before the quick shortcut name is returned to the application
  //@shortcut New data about the shortcut
  //updateQuickReplyShortcut shortcut:quickReplyShortcut = Update;
  void process_update(td::td_api::updateQuickReplyShortcut &update) {
  }

  //@description A quick reply shortcut and all its messages were deleted @shortcut_id The identifier of the deleted shortcut
  //updateQuickReplyShortcutDeleted shortcut_id:int32 = Update;
  void process_update(td::td_api::updateQuickReplyShortcutDeleted &update) {
  }

  //@description The list of quick reply shortcuts has changed @shortcut_ids The new list of identifiers of quick reply shortcuts
  //updateQuickReplyShortcuts shortcut_ids:vector<int32> = Update;
  void process_update(td::td_api::updateQuickReplyShortcuts &update) {
  }

  //@description The list of quick reply shortcut messages has changed
  //@shortcut_id The identifier of the shortcut
  //@messages The new list of quick reply messages for the shortcut in order from the first to the last sent
  //updateQuickReplyShortcutMessages shortcut_id:int32 messages:vector<quickReplyMessage> = Update;
  void process_update(td::td_api::updateQuickReplyShortcutMessages &update) {
  }

  //@description Basic information about a topic in a forum chat was changed
  //@chat_id Chat identifier
  //@info New information about the topic
  //updateForumTopicInfo chat_id:int53 info:forumTopicInfo = Update;
  void process_update(td::td_api::updateForumTopicInfo &update) {
    dialog_list_window()->process_update(update);
  }

  //@description Notification settings for some type of chats were updated
  //@scope Types of chats for which notification settings were updated
  //@notification_settings The new notification settings
  //updateScopeNotificationSettings scope:NotificationSettingsScope notification_settings:scopeNotificationSettings = Update;
  void process_update(td::td_api::updateScopeNotificationSettings &update) {
    global_parameters().process_update(update);
  }

  //@description Notification settings for reactions were updated
  //@notification_settings The new notification settings
  //updateReactionNotificationSettings notification_settings:reactionNotificationSettings = Update;
  void process_update(td::td_api::updateReactionNotificationSettings &update) {
    global_parameters().process_update(update);
  }

  //@description A notification was changed
  //@notification_group_id Unique notification group identifier
  //@notification Changed notification
  //updateNotification notification_group_id:int32 notification:notification = Update;
  void process_update(td::td_api::updateNotification &update) {
  }

  //@description A list of active notifications in a notification group has changed
  //@notification_group_id Unique notification group identifier
  //@type New type of the notification group
  //@chat_id Identifier of a chat to which all notifications in the group belong
  //@notification_settings_chat_id Chat identifier, which notification settings must be applied to the added notifications
  //@notification_sound_id Identifier of the notification sound to be played; 0 if sound is disabled
  //@total_count Total number of unread notifications in the group, can be bigger than number of active notifications
  //@added_notifications List of added group notifications, sorted by notification identifier
  //@removed_notification_ids Identifiers of removed group notifications, sorted by notification identifier
  //updateNotificationGroup notification_group_id:int32 type:NotificationGroupType chat_id:int53 notification_settings_chat_id:int53 notification_sound_id:int64 total_count:int32 added_notifications:vector<notification> removed_notification_ids:vector<int32> = Update;
  void process_update(td::td_api::updateNotificationGroup &update) {
  }

  //@description Contains active notifications that were shown on previous application launches. This update is sent only if the message database is used. In that case it comes once before any updateNotification and updateNotificationGroup update
  //@groups Lists of active notification groups
  //updateActiveNotifications groups:vector<notificationGroup> = Update;
  void process_update(td::td_api::updateActiveNotifications &update) {
  }

  //@description Describes whether there are some pending notification updates. Can be used to prevent application from killing, while there are some pending notifications
  //@have_delayed_notifications True, if there are some delayed notification updates, which will be sent soon
  //@have_unreceived_notifications True, if there can be some yet unreceived notifications, which are being fetched from the server
  //updateHavePendingNotifications have_delayed_notifications:Bool have_unreceived_notifications:Bool = Update;
  void process_update(td::td_api::updateHavePendingNotifications &update) {
  }

  //@description Some messages were deleted
  //@chat_id Chat identifier
  //@message_ids Identifiers of the deleted messages
  //@is_permanent True, if the messages are permanently deleted by a user (as opposed to just becoming inaccessible)
  //@from_cache True, if the messages are deleted only from the cache and can possibly be retrieved again in the future
  //updateDeleteMessages chat_id:int53 message_ids:vector<int53> is_permanent:Bool from_cache:Bool = Update;
  void process_update(td::td_api::updateDeleteMessages &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description A message sender activity in the chat has changed
  //@chat_id Chat identifier
  //@message_thread_id If not 0, the message thread identifier in which the action was performed
  //@sender_id Identifier of a message sender performing the action
  //@action The action
  //updateChatAction chat_id:int53 message_thread_id:int53 sender_id:MessageSender action:ChatAction = Update;
  void process_update(td::td_api::updateChatAction &update) {
    auto c = chat_window();
    if (c && c->main_chat_id() == update.chat_id_) {
      c->process_update(update);
    }
  }

  //@description The user went online or offline
  //@user_id User identifier
  //@status New status of the user
  //updateUserStatus user_id:int53 status:UserStatus = Update;
  void process_update(td::td_api::updateUserStatus &update) {
    dialog_list_window()->process_user_update(update);
  }

  //@description Some data of a user has changed. This update is guaranteed to come before the user identifier is returned to the application
  //@user New data about the user
  //updateUser user:user = Update;
  void process_update(td::td_api::updateUser &update) {
    dialog_list_window()->process_user_update(update);
  }

  //@description Some data of a basic group has changed. This update is guaranteed to come before the basic group identifier is returned to the application
  //@basic_group New data about the group
  //updateBasicGroup basic_group:basicGroup = Update;
  void process_update(td::td_api::updateBasicGroup &update) {
    dialog_list_window()->process_user_update(update);
  }

  //@description Some data of a supergroup or a channel has changed. This update is guaranteed to come before the supergroup identifier is returned to the application
  //@supergroup New data about the supergroup
  //updateSupergroup supergroup:supergroup = Update;
  void process_update(td::td_api::updateSupergroup &update) {
    dialog_list_window()->process_user_update(update);
  }

  //@description Some data of a secret chat has changed. This update is guaranteed to come before the secret chat identifier is returned to the application
  //@secret_chat New data about the secret chat
  //updateSecretChat secret_chat:secretChat = Update;
  void process_update(td::td_api::updateSecretChat &update) {
    dialog_list_window()->process_user_update(update);
  }

  //@description Some data in userFullInfo has been changed
  //@user_id User identifier
  //@user_full_info New full information about the user
  //updateUserFullInfo user_id:int53 user_full_info:userFullInfo = Update;
  void process_update(td::td_api::updateUserFullInfo &update) {
    dialog_list_window()->process_user_update(update);
  }

  //@description Some data in basicGroupFullInfo has been changed
  //@basic_group_id Identifier of a basic group
  //@basic_group_full_info New full information about the group
  //updateBasicGroupFullInfo basic_group_id:int53 basic_group_full_info:basicGroupFullInfo = Update;
  void process_update(td::td_api::updateBasicGroupFullInfo &update) {
    dialog_list_window()->process_user_update(update);
  }

  //@description Some data in supergroupFullInfo has been changed @supergroup_id Identifier of the supergroup or channel
  //@supergroup_full_info New full information about the supergroup
  //updateSupergroupFullInfo supergroup_id:int53 supergroup_full_info:supergroupFullInfo = Update;
  void process_update(td::td_api::updateSupergroupFullInfo &update) {
    dialog_list_window()->process_user_update(update);
  }

  //@description A service notification from the server was received. Upon receiving this the application must show a popup with the content of the notification
  //@type Notification type. If type begins with "AUTH_KEY_DROP_", then two buttons "Cancel" and "Log out" must be shown under notification; if user presses the second, all local data must be destroyed using Destroy method
  //@content Notification content
  //updateServiceNotification type:string content:MessageContent = Update;
  void process_update(td::td_api::updateServiceNotification &update) {
    Outputter out;
    out << "SERVICE NOTIFICATION\n" << *update.content_;

    spawn_popup_view_window(out.as_str(), out.markup(), 3);
  }

  //@description Information about a file was updated
  //@file New data about the file
  //updateFile file:file = Update;
  void process_update(td::td_api::updateFile &update) {
    auto c = chat_window();
    if (c) {
      c->process_update(update);
    }
  }

  //@description The file generation process needs to be started by the application
  //@generation_id Unique identifier for the generation process
  //@original_path The path to a file from which a new file is generated; may be empty
  //@destination_path The path to a file that must be created and where the new file is generated
  //@conversion String specifying the conversion applied to the original file. If conversion is "#url#" than original_path contains an HTTP/HTTPS URL of a file, which must be downloaded by the application
  //updateFileGenerationStart generation_id:int64 original_path:string destination_path:string conversion:string = Update;
  void process_update(td::td_api::updateFileGenerationStart &update) {
  }

  //@description File generation is no longer needed @generation_id Unique identifier for the generation process
  //updateFileGenerationStop generation_id:int64 = Update;
  void process_update(td::td_api::updateFileGenerationStop &update) {
  }

  //@description The state of the file download list has changed
  //@total_size Total size of files in the file download list, in bytes
  //@total_count Total number of files in the file download list
  //@downloaded_size Total downloaded size of files in the file download list, in bytes
  //updateFileDownloads total_size:int53 total_count:int32 downloaded_size:int53 = Update;
  void process_update(td::td_api::updateFileDownloads &update) {
    global_parameters().process_update(update);
  }

  //@description A file was added to the file download list. This update is sent only after file download list is loaded for the first time
  //@file_download The added file download
  //@counts New number of being downloaded and recently downloaded files found
  //updateFileAddedToDownloads file_download:fileDownload counts:downloadedFileCounts = Update;
  void process_update(td::td_api::updateFileAddedToDownloads &update) {
  }

  //@description A file download was changed. This update is sent only after file download list is loaded for the first time
  //@file_id File identifier
  //@complete_date Point in time (Unix timestamp) when the file downloading was completed; 0 if the file downloading isn't completed
  //@is_paused True, if downloading of the file is paused
  //@counts New number of being downloaded and recently downloaded files found
  //updateFileDownload file_id:int32 complete_date:int32 is_paused:Bool counts:downloadedFileCounts = Update;
  void process_update(td::td_api::updateFileDownload &update) {
  }

  //@description A file was removed from the file download list. This update is sent only after file download list is loaded for the first time
  //@file_id File identifier
  //@counts New number of being downloaded and recently downloaded files found
  //updateFileRemovedFromDownloads file_id:int32 counts:downloadedFileCounts = Update;
  void process_update(td::td_api::updateFileRemovedFromDownloads &update) {
  }

  //@description A request can't be completed unless application verification is performed; for official mobile applications only.
  //-The method setApplicationVerificationToken must be called once the verification is completed or failed
  //@verification_id Unique identifier for the verification process
  //@nonce Unique base64url-encoded nonce for the classic Play Integrity verification (https://developer.android.com/google/play/integrity/classic) for Android,
  //-or a unique string to compare with verify_nonce field from a push notification for iOS
  //@cloud_project_number Cloud project number to pass to the Play Integrity API on Android
  //updateApplicationVerificationRequired verification_id:int53 nonce:string cloud_project_number:int64 = Update;
  void process_update(td::td_api::updateApplicationVerificationRequired &update) {
    spawn_popup_view_window("APP VERIFICATION REQUIRED (IMPOSSIBLE TO CONTINUE)", 3);
  }

  //@description New call was created or information about a call was updated
  //@call New data about a call
  //updateCall call:call = Update;
  void process_update(td::td_api::updateCall &update) {
  }

  //@description Information about a group call was updated
  //@group_call New data about a group call
  //updateGroupCall group_call:groupCall = Update;
  void process_update(td::td_api::updateGroupCall &update) {
  }

  //@description Information about a group call participant was changed. The updates are sent only after the group call is received through getGroupCall and only if the call is joined or being joined
  //@group_call_id Identifier of group call
  //@participant New data about a participant
  //updateGroupCallParticipant group_call_id:int32 participant:groupCallParticipant = Update;
  void process_update(td::td_api::updateGroupCallParticipant &update) {
  }

  //@description New call signaling data arrived
  //@call_id The call identifier
  //@data The data
  //updateNewCallSignalingData call_id:int32 data:bytes = Update;
  void process_update(td::td_api::updateNewCallSignalingData &update) {
  }

  //@description Some privacy setting rules have been changed
  //@setting The privacy setting
  //@rules New privacy rules
  //updateUserPrivacySettingRules setting:UserPrivacySetting rules:userPrivacySettingRules = Update;
  void process_update(td::td_api::updateUserPrivacySettingRules &update) {
    global_parameters().process_update(update);
  }

  //@description Number of unread messages in a chat list has changed. This update is sent only if the message database is used
  //@chat_list The chat list with changed number of unread messages
  //@unread_count Total number of unread messages
  //@unread_unmuted_count Total number of unread messages in unmuted chats
  //updateUnreadMessageCount chat_list:ChatList unread_count:int32 unread_unmuted_count:int32 = Update;
  void process_update(td::td_api::updateUnreadMessageCount &update) {
  }

  //@description Number of unread chats, i.e. with unread messages or marked as unread, has changed. This update is sent only if the message database is used
  //@chat_list The chat list with changed number of unread messages
  //@total_count Approximate total number of chats in the chat list
  //@unread_count Total number of unread chats
  //@unread_unmuted_count Total number of unread unmuted chats
  //@marked_as_unread_count Total number of chats marked as unread
  //@marked_as_unread_unmuted_count Total number of unmuted chats marked as unread
  //updateUnreadChatCount chat_list:ChatList total_count:int32 unread_count:int32 unread_unmuted_count:int32 marked_as_unread_count:int32 marked_as_unread_unmuted_count:int32 = Update;
  void process_update(td::td_api::updateUnreadChatCount &update) {
    unread_chats_ = update.unread_unmuted_count_ + update.marked_as_unread_unmuted_count_;
    update_status_line();
  }

  //@description A story was changed
  //@story The new information about the story
  //updateStory story:story = Update;
  void process_update(td::td_api::updateStory &update) {
  }

  //@description A story became inaccessible
  //@story_sender_chat_id Identifier of the chat that posted the story
  //@story_id Story identifier
  //updateStoryDeleted story_sender_chat_id:int53 story_id:int32 = Update;
  void process_update(td::td_api::updateStoryDeleted &update) {
  }

  //@description A story has been successfully sent
  //@story The sent story
  //@old_story_id The previous temporary story identifier
  //updateStorySendSucceeded story:story old_story_id:int32 = Update;
  void process_update(td::td_api::updateStorySendSucceeded &update) {
  }

  //@description A story failed to send. If the story sending is canceled, then updateStoryDeleted will be received instead of this update
  //@story The failed to send story
  //@error The cause of the story sending failure
  //@error_type Type of the error; may be null if unknown
  //updateStorySendFailed story:story error:error error_type:CanSendStoryResult = Update;
  void process_update(td::td_api::updateStorySendFailed &update) {
  }

  //@description The list of active stories posted by a specific chat has changed
  //@active_stories The new list of active stories
  //updateChatActiveStories active_stories:chatActiveStories = Update;
  void process_update(td::td_api::updateChatActiveStories &update) {
  }

  //@description Number of chats in a story list has changed
  //@story_list The story list @chat_count Approximate total number of chats with active stories in the list
  //updateStoryListChatCount story_list:StoryList chat_count:int32 = Update;
  void process_update(td::td_api::updateStoryListChatCount &update) {
  }

  //@description Story stealth mode settings have changed
  //@active_until_date Point in time (Unix timestamp) until stealth mode is active; 0 if it is disabled
  //@cooldown_until_date Point in time (Unix timestamp) when stealth mode can be enabled again; 0 if there is no active cooldown
  //updateStoryStealthMode active_until_date:int32 cooldown_until_date:int32 = Update;
  void process_update(td::td_api::updateStoryStealthMode &update) {
  }

  //@description An option changed its value
  //@name The option name
  //@value The new option value
  //updateOption name:string value:OptionValue = Update;
  void process_update(td::td_api::updateOption &update) {
  }

  //@description A sticker set has changed
  //@sticker_set The sticker set
  //updateStickerSet sticker_set:stickerSet = Update;
  void process_update(td::td_api::updateStickerSet &update) {
    sticker_manager().process_update(update);
  }

  //@description The list of installed sticker sets was updated @sticker_type Type of the affected stickers
  //@sticker_set_ids The new list of installed ordinary sticker sets
  //updateInstalledStickerSets sticker_type:StickerType sticker_set_ids:vector<int64> = Update;
  void process_update(td::td_api::updateInstalledStickerSets &update) {
    sticker_manager().process_update(update);
  }

  //@description The list of trending sticker sets was updated or some of them were viewed @sticker_type Type of the affected stickers @sticker_sets The prefix of the list of trending sticker sets with the newest trending sticker sets
  //updateTrendingStickerSets sticker_type:StickerType sticker_sets:trendingStickerSets = Update;
  void process_update(td::td_api::updateTrendingStickerSets &update) {
    sticker_manager().process_update(update);
  }

  //@description The list of recently used stickers was updated
  //@is_attached True, if the list of stickers attached to photo or video files was updated; otherwise, the list of sent stickers is updated
  //@sticker_ids The new list of file identifiers of recently used stickers
  //updateRecentStickers is_attached:Bool sticker_ids:vector<int32> = Update;
  void process_update(td::td_api::updateRecentStickers &update) {
    sticker_manager().process_update(update);
  }

  //@description The list of favorite stickers was updated
  //@sticker_ids The new list of file identifiers of favorite stickers
  //updateFavoriteStickers sticker_ids:vector<int32> = Update;
  void process_update(td::td_api::updateFavoriteStickers &update) {
    sticker_manager().process_update(update);
  }

  //@description The list of saved animations was updated
  //@animation_ids The new list of file identifiers of saved animations
  //updateSavedAnimations animation_ids:vector<int32> = Update;
  void process_update(td::td_api::updateSavedAnimations &update) {
    global_parameters().process_update(update);
  }

  //@description The list of saved notification sounds was updated. This update may not be sent until information about a notification sound was requested for the first time
  //@notification_sound_ids The new list of identifiers of saved notification sounds
  //updateSavedNotificationSounds notification_sound_ids:vector<int64> = Update;
  void process_update(td::td_api::updateSavedNotificationSounds &update) {
    global_parameters().process_update(update);
  }

  //@description The default background has changed
  //@for_dark_theme True, if default background for dark theme has changed
  //@background The new default background; may be null
  //updateDefaultBackground for_dark_theme:Bool background:background = Update;
  void process_update(td::td_api::updateDefaultBackground &update) {
    global_parameters().process_update(update);
  }

  //@description The list of available chat themes has changed
  //@chat_themes The new list of chat themes
  //updateChatThemes chat_themes:vector<chatTheme> = Update;
  void process_update(td::td_api::updateChatThemes &update) {
    global_parameters().process_update(update);
  }

  //@description The list of supported accent colors has changed
  //@colors Information about supported colors; colors with identifiers 0 (red), 1 (orange), 2 (purple/violet), 3 (green), 4 (cyan), 5 (blue), 6 (pink) must always be supported
  //-and aren't included in the list. The exact colors for the accent colors with identifiers 0-6 must be taken from the app theme
  //@available_accent_color_ids The list of accent color identifiers, which can be set through setAccentColor and setChatAccentColor. The colors must be shown in the specififed order
  //updateAccentColors colors:vector<accentColor> available_accent_color_ids:vector<int32> = Update;
  void process_update(td::td_api::updateAccentColors &update) {
    global_parameters().process_update(update);
  }

  //@description The list of supported accent colors for user profiles has changed
  //@colors Information about supported colors
  //@available_accent_color_ids The list of accent color identifiers, which can be set through setProfileAccentColor and setChatProfileAccentColor. The colors must be shown in the specififed order
  //updateProfileAccentColors colors:vector<profileAccentColor> available_accent_color_ids:vector<int32> = Update;
  void process_update(td::td_api::updateProfileAccentColors &update) {
    global_parameters().process_update(update);
  }

  //@description Some language pack strings have been updated
  //@localization_target Localization target to which the language pack belongs
  //@language_pack_id Identifier of the updated language pack
  //@strings List of changed language pack strings; empty if all strings have changed
  //updateLanguagePackStrings localization_target:string language_pack_id:string strings:vector<languagePackString> = Update;
  void process_update(td::td_api::updateLanguagePackStrings &update) {
    global_parameters().process_update(update);
  }

  //@description The connection state has changed. This update must be used only to show a human-readable description of the connection state
  //@state The new connection state
  //updateConnectionState state:ConnectionState = Update;
  void process_update(td::td_api::updateConnectionState &update) {
    global_parameters().process_update(update);
    update_status_line();
  }

  //@description New terms of service must be accepted by the user. If the terms of service are declined, then the deleteAccount method must be called with the reason "Decline ToS update" @terms_of_service_id Identifier of the terms of service
  //@terms_of_service The new terms of service
  //updateTermsOfService terms_of_service_id:string terms_of_service:termsOfService = Update;
  void process_update(td::td_api::updateTermsOfService &update) {
    Outputter out;
    out << "UPDATED TERMS OF SERVICE:\n" << update.terms_of_service_->text_;
    spawn_popup_view_window(out.as_str(), out.markup(), 3);
  }

  //@description The list of users nearby has changed. The update is guaranteed to be sent only 60 seconds after a successful searchChatsNearby request
  //@users_nearby The new list of users nearby
  //updateUsersNearby users_nearby:vector<chatNearby> = Update;
  void process_update(td::td_api::updateUsersNearby &update) {
  }

  //@description The first unconfirmed session has changed
  //@session The unconfirmed session; may be null if none
  //updateUnconfirmedSession session:unconfirmedSession = Update;
  void process_update(td::td_api::updateUnconfirmedSession &update) {
  }

  //@description The list of bots added to attachment or side menu has changed
  //@bots The new list of bots. The bots must not be shown on scheduled messages screen
  //updateAttachmentMenuBots bots:vector<attachmentMenuBot> = Update;
  void process_update(td::td_api::updateAttachmentMenuBots &update) {
    global_parameters().process_update(update);
  }

  //@description A message was sent by an opened Web App, so the Web App needs to be closed
  //@web_app_launch_id Identifier of Web App launch
  //updateWebAppMessageSent web_app_launch_id:int64 = Update;
  void process_update(td::td_api::updateWebAppMessageSent &update) {
  }

  //@description The list of active emoji reactions has changed
  //@emojis The new list of active emoji reactions
  //updateActiveEmojiReactions emojis:vector<string> = Update;
  void process_update(td::td_api::updateActiveEmojiReactions &update) {
    global_parameters().process_update(update);
  }

  //@description The list of available message effects has changed
  //@reaction_effect_ids The new list of available message effects from emoji reactions
  //@sticker_effect_ids The new list of available message effects from Premium stickers
  //updateAvailableMessageEffects reaction_effect_ids:vector<int64> sticker_effect_ids:vector<int64> = Update;
  void process_update(td::td_api::updateAvailableMessageEffects &update) {
    global_parameters().process_update(update);
  }

  //@description The type of default reaction has changed
  //@reaction_type The new type of the default reaction
  //updateDefaultReactionType reaction_type:ReactionType = Update;
  void process_update(td::td_api::updateDefaultReactionType &update) {
    global_parameters().process_update(update);
  }

  //@description Tags used in Saved Messages or a Saved Messages topic have changed
  //@saved_messages_topic_id Identifier of Saved Messages topic which tags were changed; 0 if tags for the whole chat has changed
  //@tags The new tags
  //updateSavedMessagesTags saved_messages_topic_id:int53 tags:savedMessagesTags = Update;
  void process_update(td::td_api::updateSavedMessagesTags &update) {
  }

  //@description The number of Telegram Stars owned by the current user has changed
  //@star_count The new number of Telegram Stars owned
  //updateOwnedStarCount star_count:int53 = Update;
  void process_update(td::td_api::updateOwnedStarCount &update) {
    global_parameters().process_update(update);
  }

  //@description The revenue earned from sponsored messages in a chat has changed. If chat revenue screen is opened, then getChatRevenueTransactions may be called to fetch new transactions
  //@chat_id Identifier of the chat
  //@revenue_amount New amount of earned revenue
  //updateChatRevenueAmount chat_id:int53 revenue_amount:chatRevenueAmount = Update;
  void process_update(td::td_api::updateChatRevenueAmount &update) {
    dialog_list_window()->process_update(update);
  }

  //@description The Telegram Star revenue earned by a bot or a chat has changed. If Telegram Star transaction screen of the chat is opened, then getStarTransactions may be called to fetch new transactions
  //@owner_id Identifier of the owner of the Telegram Stars
  //@status New Telegram Star revenue status
  //updateStarRevenueStatus owner_id:MessageSender status:starRevenueStatus = Update;
  void process_update(td::td_api::updateStarRevenueStatus &update) {
  }

  //@description The parameters of speech recognition without Telegram Premium subscription has changed
  //@max_media_duration The maximum allowed duration of media for speech recognition without Telegram Premium subscription, in seconds
  //@weekly_count The total number of allowed speech recognitions per week; 0 if none
  //@left_count Number of left speech recognition attempts this week
  //@next_reset_date Point in time (Unix timestamp) when the weekly number of tries will reset; 0 if unknown
  //updateSpeechRecognitionTrial max_media_duration:int32 weekly_count:int32 left_count:int32 next_reset_date:int32 = Update;
  void process_update(td::td_api::updateSpeechRecognitionTrial &update) {
  }

  //@description The list of supported dice emojis has changed
  //@emojis The new list of supported dice emojis
  //updateDiceEmojis emojis:vector<string> = Update;
  void process_update(td::td_api::updateDiceEmojis &update) {
    global_parameters().process_update(update);
  }

  //@description Some animated emoji message was clicked and a big animated sticker must be played if the message is visible on the screen. chatActionWatchingAnimations with the text of the message needs to be sent if the sticker is played
  //@chat_id Chat identifier
  //@message_id Message identifier
  //@sticker The animated sticker to be played
  //updateAnimatedEmojiMessageClicked chat_id:int53 message_id:int53 sticker:sticker = Update;
  void process_update(td::td_api::updateAnimatedEmojiMessageClicked &update) {
  }

  //@description The parameters of animation search through getOption("animation_search_bot_username") bot has changed
  //@provider Name of the animation search provider
  //@emojis The new list of emojis suggested for searching
  //updateAnimationSearchParameters provider:string emojis:vector<string> = Update;
  void process_update(td::td_api::updateAnimationSearchParameters &update) {
    global_parameters().process_update(update);
  }

  //@description The list of suggested to the user actions has changed
  //@added_actions Added suggested actions
  //@removed_actions Removed suggested actions
  //updateSuggestedActions added_actions:vector<SuggestedAction> removed_actions:vector<SuggestedAction> = Update;
  void process_update(td::td_api::updateSuggestedActions &update) {
  }

  //@description Download or upload file speed for the user was limited, but it can be restored by subscription to Telegram Premium. The notification can be postponed until a being downloaded or uploaded file is visible to the user
  //-Use getOption("premium_download_speedup") or getOption("premium_upload_speedup") to get expected speedup after subscription to Telegram Premium
  //@is_upload True, if upload speed was limited; false, if download speed was limited
  //updateSpeedLimitNotification is_upload:Bool = Update;
  void process_update(td::td_api::updateSpeedLimitNotification &update) {
  }

  //@description The list of contacts that had birthdays recently or will have birthday soon has changed
  //@close_birthday_users List of contact users with close birthday
  //updateContactCloseBirthdays close_birthday_users:vector<closeBirthdayUser> = Update;
  void process_update(td::td_api::updateContactCloseBirthdays &update) {
  }

  //@description Autosave settings for some type of chats were updated
  //@scope Type of chats for which autosave settings were updated
  //@settings The new autosave settings; may be null if the settings are reset to default
  //updateAutosaveSettings scope:AutosaveSettingsScope settings:scopeAutosaveSettings = Update;
  void process_update(td::td_api::updateAutosaveSettings &update) {
    global_parameters().process_update(update);
  }

  //@description A business connection has changed; for bots only
  //@connection New data about the connection
  //updateBusinessConnection connection:businessConnection = Update;
  void process_update(td::td_api::updateBusinessConnection &update) {
    UNREACHABLE();
  }

  //@description A new message was added to a business account; for bots only
  //@connection_id Unique identifier of the business connection
  //@message The new message
  //updateNewBusinessMessage connection_id:string message:businessMessage = Update;
  void process_update(td::td_api::updateNewBusinessMessage &update) {
    UNREACHABLE();
  }

  //@description A message in a business account was edited; for bots only
  //@connection_id Unique identifier of the business connection
  //@message The edited message
  //updateBusinessMessageEdited connection_id:string message:businessMessage = Update;
  void process_update(td::td_api::updateBusinessMessageEdited &update) {
    UNREACHABLE();
  }

  //@description Messages in a business account were deleted; for bots only
  //@connection_id Unique identifier of the business connection
  //@chat_id Identifier of a chat in the business account in which messages were deleted
  //@message_ids Unique message identifiers of the deleted messages
  //updateBusinessMessagesDeleted connection_id:string chat_id:int53 message_ids:vector<int53> = Update;
  void process_update(td::td_api::updateBusinessMessagesDeleted &update) {
    UNREACHABLE();
  }

  //@description A new incoming inline query; for bots only
  //@id Unique query identifier
  //@sender_user_id Identifier of the user who sent the query
  //@user_location User location; may be null
  //@chat_type The type of the chat from which the query originated; may be null if unknown
  //@query Text of the query
  //@offset Offset of the first entry to return
  //updateNewInlineQuery id:int64 sender_user_id:int53 user_location:location chat_type:ChatType query:string offset:string = Update;
  void process_update(td::td_api::updateNewInlineQuery &update) {
    UNREACHABLE();
  }

  //@description The user has chosen a result of an inline query; for bots only
  //@sender_user_id Identifier of the user who sent the query
  //@user_location User location; may be null
  //@query Text of the query
  //@result_id Identifier of the chosen result
  //@inline_message_id Identifier of the sent inline message, if known
  //updateNewChosenInlineResult sender_user_id:int53 user_location:location query:string result_id:string inline_message_id:string = Update;
  void process_update(td::td_api::updateNewChosenInlineResult &update) {
    UNREACHABLE();
  }

  //@description A new incoming callback query; for bots only
  //@id Unique query identifier
  //@sender_user_id Identifier of the user who sent the query
  //@chat_id Identifier of the chat where the query was sent
  //@message_id Identifier of the message from which the query originated
  //@chat_instance Identifier that uniquely corresponds to the chat to which the message was sent
  //@payload Query payload
  //updateNewCallbackQuery id:int64 sender_user_id:int53 chat_id:int53 message_id:int53 chat_instance:int64 payload:CallbackQueryPayload = Update;
  void process_update(td::td_api::updateNewCallbackQuery &update) {
    UNREACHABLE();
  }

  //@description A new incoming callback query from a message sent via a bot; for bots only
  //@id Unique query identifier
  //@sender_user_id Identifier of the user who sent the query
  //@inline_message_id Identifier of the inline message from which the query originated
  //@chat_instance An identifier uniquely corresponding to the chat a message was sent to
  //@payload Query payload
  //updateNewInlineCallbackQuery id:int64 sender_user_id:int53 inline_message_id:string chat_instance:int64 payload:CallbackQueryPayload = Update;
  void process_update(td::td_api::updateNewInlineCallbackQuery &update) {
    UNREACHABLE();
  }

  //@description A new incoming callback query from a business message; for bots only
  //@id Unique query identifier
  //@sender_user_id Identifier of the user who sent the query
  //@connection_id Unique identifier of the business connection
  //@message The message from the business account from which the query originated
  //@chat_instance An identifier uniquely corresponding to the chat a message was sent to
  //@payload Query payload
  //updateNewBusinessCallbackQuery id:int64 sender_user_id:int53 connection_id:string message:businessMessage chat_instance:int64 payload:CallbackQueryPayload = Update;
  void process_update(td::td_api::updateNewBusinessCallbackQuery &update) {
    UNREACHABLE();
  }

  //@description A new incoming shipping query; for bots only. Only for invoices with flexible price
  //@id Unique query identifier
  //@sender_user_id Identifier of the user who sent the query
  //@invoice_payload Invoice payload
  //@shipping_address User shipping address
  //updateNewShippingQuery id:int64 sender_user_id:int53 invoice_payload:string shipping_address:address = Update;
  void process_update(td::td_api::updateNewShippingQuery &update) {
    UNREACHABLE();
  }

  //@description A new incoming pre-checkout query; for bots only. Contains full information about a checkout
  //@id Unique query identifier
  //@sender_user_id Identifier of the user who sent the query
  //@currency Currency for the product price
  //@total_amount Total price for the product, in the smallest units of the currency
  //@invoice_payload Invoice payload
  //@shipping_option_id Identifier of a shipping option chosen by the user; may be empty if not applicable
  //@order_info Information about the order; may be null
  //updateNewPreCheckoutQuery id:int64 sender_user_id:int53 currency:string total_amount:int53 invoice_payload:bytes shipping_option_id:string order_info:orderInfo = Update;
  void process_update(td::td_api::updateNewPreCheckoutQuery &update) {
    UNREACHABLE();
  }

  //@description A new incoming event; for bots only
  //@event A JSON-serialized event
  //updateNewCustomEvent event:string = Update;
  void process_update(td::td_api::updateNewCustomEvent &update) {
    UNREACHABLE();
  }

  //@description A new incoming query; for bots only
  //@id The query identifier
  //@data JSON-serialized query data
  //@timeout Query timeout
  //updateNewCustomQuery id:int64 data:string timeout:int32 = Update;
  void process_update(td::td_api::updateNewCustomQuery &update) {
    UNREACHABLE();
  }

  //@description A poll was updated; for bots only
  //@poll New data about the poll
  //updatePoll poll:poll = Update;
  void process_update(td::td_api::updatePoll &update) {
    UNREACHABLE();
  }

  //@description A user changed the answer to a poll; for bots only
  //@poll_id Unique poll identifier
  //@voter_id Identifier of the message sender that changed the answer to the poll
  //@option_ids 0-based identifiers of answer options, chosen by the user
  //updatePollAnswer poll_id:int64 voter_id:MessageSender option_ids:vector<int32> = Update;
  void process_update(td::td_api::updatePollAnswer &update) {
    UNREACHABLE();
  }

  //@description User rights changed in a chat; for bots only
  //@chat_id Chat identifier
  //@actor_user_id Identifier of the user, changing the rights
  //@date Point in time (Unix timestamp) when the user rights were changed
  //@invite_link If user has joined the chat using an invite link, the invite link; may be null
  //@via_join_request True, if the user has joined the chat after sending a join request and being approved by an administrator
  //@via_chat_folder_invite_link True, if the user has joined the chat using an invite link for a chat folder
  //@old_chat_member Previous chat member
  //@new_chat_member New chat member
  //updateChatMember chat_id:int53 actor_user_id:int53 date:int32 invite_link:chatInviteLink via_join_request:Bool via_chat_folder_invite_link:Bool old_chat_member:chatMember new_chat_member:chatMember = Update;
  void process_update(td::td_api::updateChatMember &update) {
    UNREACHABLE();
  }

  //@description A user sent a join request to a chat; for bots only
  //@chat_id Chat identifier
  //@request Join request
  //@user_chat_id Chat identifier of the private chat with the user
  //@invite_link The invite link, which was used to send join request; may be null
  //updateNewChatJoinRequest chat_id:int53 request:chatJoinRequest user_chat_id:int53 invite_link:chatInviteLink = Update;
  void process_update(td::td_api::updateNewChatJoinRequest &update) {
    UNREACHABLE();
  }

  //@description A chat boost has changed; for bots only
  //@chat_id Chat identifier
  //@boost New information about the boost
  //updateChatBoost chat_id:int53 boost:chatBoost = Update;
  void process_update(td::td_api::updateChatBoost &update) {
    UNREACHABLE();
  }

  //@description User changed its reactions on a message with public reactions; for bots only
  //@chat_id Chat identifier
  //@message_id Message identifier
  //@actor_id Identifier of the user or chat that changed reactions
  //@date Point in time (Unix timestamp) when the reactions were changed
  //@old_reaction_types Old list of chosen reactions
  //@new_reaction_types New list of chosen reactions
  //updateMessageReaction chat_id:int53 message_id:int53 actor_id:MessageSender date:int32 old_reaction_types:vector<ReactionType> new_reaction_types:vector<ReactionType> = Update;
  void process_update(td::td_api::updateMessageReaction &update) {
    UNREACHABLE();
  }

  //@description Reactions added to a message with anonymous reactions have changed; for bots only
  //@chat_id Chat identifier
  //@message_id Message identifier
  //@date Point in time (Unix timestamp) when the reactions were changed
  //@reactions The list of reactions added to the message
  //updateMessageReactions chat_id:int53 message_id:int53 date:int32 reactions:vector<messageReaction> = Update;
  void process_update(td::td_api::updateMessageReactions &update) {
    UNREACHABLE();
  }

  void do_send_request(td::tl_object_ptr<td::td_api::Function> func,
                       td::Promise<td::tl_object_ptr<td::td_api::Object>> cb) override {
    auto id = ++last_query_id_;
    handlers_.emplace(id, std::move(cb));
    td::send_closure(td_, &td::ClientActor::request, id, std::move(func));
  }

  void start_up() override {
  }

  void update_status_line() override;

 private:
  std::map<td::uint64, td::Promise<td::tl_object_ptr<td::td_api::Object>>> handlers_;
  td::uint64 last_query_id_;
  td::int32 unread_chats_{0};
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
  layout_ = std::make_shared<TdcursesLayout>(this, actor_id(this));
  log_window_ = std::make_shared<windows::LogWindow>();
  log_interface_ = std::make_unique<windows::WindowLogInterface<Tdcurses>>(log_window_, actor_id(this));
  dialog_list_window_ = std::make_shared<DialogListWindow>(this, actor_id(this));
  status_line_window_ = std::make_shared<StatusLineWindow>();
  class CommandLineCallback : public CommandLineWindow::Callback {
   public:
    CommandLineCallback(Tdcurses *ptr) : curses_(ptr) {
    }
    void on_command(std::string command) override {
      if (command.size() == 0) {
        return;
      }
      if (command == ":test_popup_window") {
        curses_->spawn_popup_view_window("TEST POPUP\n1\n2\n3\n4\n5\n6", 2);
        return;
      }
      if (command == ":test_chat_search") {
        curses_->spawn_chat_selection_window([curses = curses_](td::Result<std::shared_ptr<Chat>> R) {
          if (R.is_ok()) {
            auto chat = R.move_as_ok();
            curses->open_chat(chat->chat_id());
          }
        });
        return;
      }
      if (command[0] == '/') {
        auto c = curses_->chat_window();
        if (c && c->is_active()) {
          c->set_search_pattern(command.substr(1));
        }
        auto d = curses_->dialog_list_window();
        if (d && d->is_active()) {
          d->set_search_pattern(command.substr(1));
        }
      }
    }

   private:
    Tdcurses *curses_;
  };
  command_line_window_ = std::make_shared<CommandLineWindow>(std::make_unique<CommandLineCallback>(this));
  status_line_window_->replace_text("", {windows::MarkupElement::bg_color(0, 1000, (td::int32)Color::Grey)});
  //td::log_interface = log_interface_.get();
  screen_->change_layout(layout_);
  layout_->replace_log_window(log_window_);
  layout_->replace_dialog_list_window(dialog_list_window_);
  layout_->replace_status_line_window(status_line_window_);
  layout_->replace_command_line_window(command_line_window_);
  layout_->initialize_sizes(params);
  screen_->refresh(true);

  initialize_options(params);

  auto config = std::make_shared<ConfigWindow>(this, actor_id(this), options_);
  config_window_ = std::make_shared<windows::BorderedWindow>(config, windows::Window::BorderType::Double);

  chat_info_window_ = std::make_shared<ChatInfoWindow>(this, actor_id(this), nullptr);
  chat_info_window_bordered_ =
      std::make_shared<windows::BorderedWindow>(chat_info_window_, windows::Window::BorderType::Double);

  LOG(INFO) << "starting";
}

bool Tdcurses::window_exists(td::int64 id) {
  return all_active_windows_.count(id) > 0;
}

void Tdcurses::open_chat(td::int64 chat_id) {
  chat_window_ = std::make_shared<ChatWindow>(this, actor_id(this), chat_id);
  compose_window_ = nullptr;
  layout_->replace_chat_window(chat_window_);
  layout_->activate_window(chat_window_);
  layout_->replace_compose_window(nullptr);
  update_status_line();
}

void Tdcurses::seek_chat(td::int64 chat_id, td::int64 message_id) {
  if (chat_window_) {
    chat_window_->seek(chat_id, message_id);
    return;
  }
}

void Tdcurses::open_compose_window() {
  if (!chat_window_) {
    return;
  }
  if (compose_window_) {
    return;
  }

  //ComposeWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id, std::string text)
  compose_window_ = std::make_shared<ComposeWindow>(this, actor_id(this), chat_window_->main_chat_id(),
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

void Tdcurses::hide_chat_info_window() {
  if (screen_->has_popup_window(chat_info_window_bordered_.get())) {
    screen_->del_popup_window(chat_info_window_bordered_.get());
  }
}

void Tdcurses::show_chat_info_window(td::int64 chat_id) {
  chat_info_window_->set_chat(chat_manager().get_chat(chat_id));
  if (!screen_->has_popup_window(chat_info_window_bordered_.get())) {
    screen_->add_popup_window(chat_info_window_bordered_, 3);
  } else {
    chat_info_window_bordered_->set_need_refresh();
  }
}

void Tdcurses::show_user_info_window(td::int64 user_id) {
  chat_info_window_->set_chat(chat_manager().get_user(user_id));
  if (!screen_->has_popup_window(chat_info_window_bordered_.get())) {
    screen_->add_popup_window(chat_info_window_bordered_, 3);
  } else {
    chat_info_window_bordered_->set_need_refresh();
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

void TdcursesImpl::update_status_line() {
  auto w = status_line_window();
  if (w) {
    Outputter out;

    td::td_api::downcast_call(
        const_cast<td::td_api::ConnectionState &>(*global_parameters().connection_state()),
        td::overloaded(
            [&](const td::td_api::connectionStateReady &state) { out << "ready "; },
            [&](const td::td_api::connectionStateConnectingToProxy &state) { out << "connecting to proxy "; },
            [&](const td::td_api::connectionStateConnecting &state) { out << "connecting "; },
            [&](const td::td_api::connectionStateUpdating &state) { out << "updating "; },
            [&](const td::td_api::connectionStateWaitingForNetwork &state) { out << "waitnet "; }));
    out << Outputter::Reverse(Outputter::ChangeBool::Enable) << " ";

    if (unread_chats_) {
      out << Outputter::FgColor(Color::Red) << unread_chats_ << Outputter::FgColor(Color::Revert);
    } else {
      out << unread_chats_;
    }
    out << " unread " << Outputter::Reverse(Outputter::ChangeBool::Revert) << " ";
    auto ch = chat_window();
    if (ch) {
      auto info = chat_manager().get_chat(ch->main_chat_id());
      if (info) {
        out << info->title() << " ";
      }
    }
    out << Outputter::Reverse(Outputter::ChangeBool::Enable) << " ";
    if (ch) {
      out << ch->search_pattern() << " ";
    }
    out << Outputter::Reverse(Outputter::ChangeBool::Revert) << " ";
    auto markup = out.markup();
    auto str = out.as_str();
    markup.push_back(windows::MarkupElement::bg_color(0, 1000, (td::int32)Color::Grey));
    markup.push_back(windows::MarkupElement::fg_color(0, 1000, (td::int32)Color::Lime));
    w->replace_text(std::move(str), std::move(markup));
  }
}

void Tdcurses::spawn_chat_selection_window(td::Promise<std::shared_ptr<Chat>> promise) {
  auto window = std::make_shared<ChatSearchWindow>(this, actor_id(this));
  auto boxed_window = std::make_shared<windows::BorderedWindow>(window, windows::BorderedWindow::BorderType::Double);
  class Callback : public ChatSearchWindow::Callback {
   public:
    Callback(td::Promise<std::shared_ptr<Chat>> promise, std::shared_ptr<windows::Window> window, Tdcurses *curses)
        : promise_(std::move(promise)), window_(std::move(window)), curses_(curses) {
    }
    void on_answer(std::shared_ptr<Chat> chat) override {
      promise_.set_value(std::move(chat));
      curses_->del_popup_window(window_.get());
    }
    void on_abort() override {
      promise_.set_error(td::Status::Error("Cancelled"));
      curses_->del_popup_window(window_.get());
    }

   private:
    td::Promise<std::shared_ptr<Chat>> promise_;
    std::shared_ptr<windows::Window> window_;
    Tdcurses *curses_;
  };
  window->install_callback(std::make_unique<Callback>(std::move(promise), boxed_window, this));
  add_popup_window(boxed_window, 1);
}

void Tdcurses::register_alive_window(TdcursesWindowBase *window) {
  CHECK(all_active_windows_.emplace(window->window_unique_id(), window).second);
}
void Tdcurses::unregister_alive_window(TdcursesWindowBase *window) {
  CHECK(all_active_windows_.erase(window->window_unique_id()));
}

void Tdcurses::spawn_popup_view_window(std::string text, std::vector<windows::MarkupElement> markup,
                                       td::int32 priority) {
  class Callback : public windows::ViewWindow::Callback {
   public:
    Callback(Tdcurses *ptr, std::shared_ptr<windows::Window> to_close) : ptr_(ptr), to_close_(std::move(to_close)) {
    }
    void on_answer(windows::ViewWindow *window) override {
      ptr_->del_popup_window(to_close_.get());
    }
    void on_abort(windows::ViewWindow *window) override {
      ptr_->del_popup_window(to_close_.get());
    }

   private:
    Tdcurses *ptr_;
    std::shared_ptr<windows::Window> to_close_;
  };
  auto window = std::make_shared<windows::ViewWindow>(std::move(text), std::move(markup), nullptr);
  auto box = std::make_shared<windows::BorderedWindow>(window, windows::BorderedWindow::BorderType::Double);
  window->set_callback(std::make_unique<Callback>(this, box));
  add_popup_window(box, priority);
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

  td::ConcurrentScheduler scheduler(4);

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

  std::string copy_command = "wl-copy";
  std::string link_open_command = "xdg-open";

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
    auto &os = root.add("os", libconfig::Setting::Type::TypeGroup);
    os.add("copy", libconfig::Setting::TypeString) = copy_command;
    os.add("open_link", libconfig::Setting::TypeString) = link_open_command;

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

  config.lookupValue("os.copy_command", copy_command);
  config.lookupValue("os.link_open_command", link_open_command);

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

  LOG(ERROR) << "starting";

  auto tdlib_parameters = td::td_api::make_object<td::td_api::setTdlibParameters>();
  tdlib_parameters->api_hash_ = api_hash;
  tdlib_parameters->api_id_ = api_id;
  tdlib_parameters->application_version_ = tdcurses::global_parameters().version();
  tdlib_parameters->database_directory_ = db_root + "db/";
  tdlib_parameters->device_model_ = "Console";
  tdlib_parameters->files_directory_ = db_root + "files/";
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

  tdcurses::global_parameters().set_copy_command(copy_command);
  tdcurses::global_parameters().set_link_open_command(link_open_command);

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
