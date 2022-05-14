#include "compose-window.h"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/editorwindow.h"
#include <memory>
#include <vector>

namespace tdcurses {

void ComposeWindow::install_callback() {
  class Cb : public EditorWindow::Callback {
   public:
    void on_answer(EditorWindow *window, std::string text) override {
      static_cast<ComposeWindow *>(window)->send_message(std::move(text));
    }
    void on_abort(EditorWindow *window, std::string text) override {
      static_cast<ComposeWindow *>(window)->set_draft(std::move(text));
    }
  };

  windows::EditorWindow::install_callback(std::make_unique<Cb>());
}

void ComposeWindow::send_message(std::string message) {
  //formattedText text:string entities:vector<textEntity> = FormattedText;
  auto text =
      td::make_tl_object<td::td_api::formattedText>(message, std::vector<td::tl_object_ptr<td::td_api::textEntity>>());
  //inputMessageText text:formattedText disable_web_page_preview:Bool clear_draft:Bool = InputMessageContent;
  auto content = td::make_tl_object<td::td_api::inputMessageText>(std::move(text), false, false);
  //sendMessage chat_id:int53 message_thread_id:int53 reply_to_message_id:int53 options:messageSendOptions reply_markup:ReplyMarkup input_message_content:InputMessageContent = Message;
  auto req = td::make_tl_object<td::td_api::sendMessage>(chat_id_, 0, 0, nullptr, nullptr, std::move(content));

  send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::message>> R) {});
  clear();
}

void ComposeWindow::set_draft(std::string message) {
  //formattedText text:string entities:vector<textEntity> = FormattedText;
  auto text =
      td::make_tl_object<td::td_api::formattedText>(message, std::vector<td::tl_object_ptr<td::td_api::textEntity>>());
  //inputMessageText text:formattedText disable_web_page_preview:Bool clear_draft:Bool = InputMessageContent;
  auto content = td::make_tl_object<td::td_api::inputMessageText>(std::move(text), false, false);
  //draftMessage reply_to_message_id:int53 date:int32 input_message_text:InputMessageContent = DraftMessage;
  auto draft = td::make_tl_object<td::td_api::draftMessage>(0, 0, std::move(content));
  //setChatDraftMessage chat_id:int53 message_thread_id:int53 draft_message:draftMessage = Ok;
  auto req = td::make_tl_object<td::td_api::setChatDraftMessage>(chat_id_, 0, std::move(draft));

  send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {});
  clear();
}

}  // namespace tdcurses
