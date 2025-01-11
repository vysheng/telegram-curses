#include "ComposeWindow.hpp"
#include "ChatWindow.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/EditorWindow.hpp"
#include "Outputter.hpp"
#include "TdObjectsOutput.h"
#include "windows/Output.hpp"
#include "windows/TextEdit.hpp"
#include <memory>
#include <vector>

namespace tdcurses {

void ComposeWindow::install_callback() {
  class Cb : public windows::EditorWindow::Callback {
   public:
    Cb(ComposeWindow *self) : self_(self) {
    }
    void on_answer(windows::EditorWindow *window, std::string text) override {
      self_->send_message(std::move(text));
    }
    void on_abort(windows::EditorWindow *window, std::string text) override {
      self_->set_draft(std::move(text));
    }

   private:
    ComposeWindow *self_;
  };

  editor_window_->install_callback(std::make_unique<Cb>(this));
}

void ComposeWindow::send_message(std::string message) {
  //formattedText text:string entities:vector<textEntity> = FormattedText;
  auto text =
      td::make_tl_object<td::td_api::formattedText>(message, std::vector<td::tl_object_ptr<td::td_api::textEntity>>());
  //inputMessageText text:formattedText link_preview_options:linkPreviewOptions clear_draft:Bool = InputMessageContent;
  auto content = td::make_tl_object<td::td_api::inputMessageText>(std::move(text), nullptr, true);
  td::tl_object_ptr<td::td_api::inputMessageReplyToMessage> reply;
  if (reply_message_id_) {
    reply = td::make_tl_object<td::td_api::inputMessageReplyToMessage>(reply_message_id_, nullptr);
  }
  //sendMessage chat_id:int53 message_thread_id:int53 reply_to:MessageReplyTo options:messageSendOptions reply_markup:ReplyMarkup input_message_content:InputMessageContent = Message;
  auto req =
      td::make_tl_object<td::td_api::sendMessage>(chat_id_, 0, std::move(reply), nullptr, nullptr, std::move(content));

  chat_window_->send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::message>> R) {
    if (R.is_ok()) {
      chat_window_->process_update_sent_message(R.move_as_ok());
    }
  });
  editor_window_->clear();
  set_reply_message_id(0);
}

void ComposeWindow::set_draft(std::string message) {
  //formattedText text:string entities:vector<textEntity> = FormattedText;
  auto text =
      td::make_tl_object<td::td_api::formattedText>(message, std::vector<td::tl_object_ptr<td::td_api::textEntity>>());
  //inputMessageText text:formattedText link_preview_options:linkPreviewOptions clear_draft:Bool = InputMessageContent;
  auto content = td::make_tl_object<td::td_api::inputMessageText>(std::move(text), nullptr, false);
  //draftMessage reply_to:InputMessageReplyTo date:int32 input_message_text:InputMessageContent effect_id:int64 = DraftMessage;
  auto draft = td::make_tl_object<td::td_api::draftMessage>(nullptr, 0, std::move(content), 0);
  //setChatDraftMessage chat_id:int53 message_thread_id:int53 draft_message:draftMessage = Ok;
  auto req = td::make_tl_object<td::td_api::setChatDraftMessage>(chat_id_, 0, std::move(draft));

  send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {});
  editor_window_->clear();
  root()->close_compose_window();
}

void ComposeWindow::render(windows::WindowOutputter &rb, bool force) {
  if (reply_message_id_) {
    auto msg = chat_window_->get_message_as_message(chat_id_, reply_message_id_);
    if (!msg) {
      rb.erase_yx(0, 0, width());
      rb.putstr_yx(0, 0, "reply to: ", 10);
    } else {
      Outputter out;
      out << Outputter::NoLb(true) << "reply to: " << Color::Red << msg->sender_id_ << Color::Revert << " "
          << msg->content_;
      windows::TextEdit::render(rb, width(), out.as_cslice(), 0, out.markup(), false, false);
    }
  }
}

}  // namespace tdcurses
