#include "ComposeWindow.hpp"
#include "managers/GlobalParameters.hpp"
#include "ChatWindow.hpp"
#include "AttachMenu.hpp"
#include "common-windows/ErrorWindow.hpp"
#include "common-windows/MenuWindowEdit.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/EditorWindow.hpp"
#include "Outputter.hpp"
#include "TdObjectsOutput.h"
#include "windows/Output.hpp"
#include "windows/TextEdit.hpp"
#include "td/telegram/SynchronousRequests.h"
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
  td::tl_object_ptr<td::td_api::formattedText> text;
  if (enabled_markdown_) {
    auto R =
        run_request_sync(td::make_tl_object<td::td_api::parseMarkdown>(td::make_tl_object<td::td_api::formattedText>(
            message, std::vector<td::tl_object_ptr<td::td_api::textEntity>>{})));
    if (R.is_error()) {
      root()->spawn_popup_view_window(PSTRING() << "failed to parse markdown: " << R.move_as_error(), 3);
      return;
    } else {
      text = R.move_as_ok();
    }
  } else {
    //formattedText text:string entities:vector<textEntity> = FormattedText;
    text = td::make_tl_object<td::td_api::formattedText>(message,
                                                         std::vector<td::tl_object_ptr<td::td_api::textEntity>>());
  }

  if (edit_message_id_) {
    //inputMessageText text:formattedText link_preview_options:linkPreviewOptions clear_draft:Bool = InputMessageContent;
    auto content = td::make_tl_object<td::td_api::inputMessageText>(std::move(text), nullptr, true);
    auto req = td::make_tl_object<td::td_api::editMessageText>(chat_id_, edit_message_id_, nullptr, std::move(content));
    chat_window_->send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::message>> R) {
      if (R.is_ok()) {
        chat_window_->process_update_sent_message(R.move_as_ok());
      }
    });
    editor_window_->clear();
    edit_message_id_ = 0;
    return;
  }

  //messageSendOptions disable_notification:Bool from_background:Bool protect_content:Bool allow_paid_broadcast:Bool paid_message_star_count:int53 update_order_of_installed_sticker_sets:Bool scheduling_state:MessageSchedulingState effect_id:int64 sending_id:int32 only_preview:Bool = MessageSendOptions;
  auto send_options = td::make_tl_object<td::td_api::messageSendOptions>(!no_sound_, false, false, false, 0, false,
                                                                         nullptr, 0, 0, false);

  std::vector<td::tl_object_ptr<td::td_api::InputMessageContent>> content;
  switch (attach_type_) {
    case AttachType::None:
      //inputMessageText text:formattedText link_preview_options:linkPreviewOptions clear_draft:Bool = InputMessageContent;
      content.push_back(td::make_tl_object<td::td_api::inputMessageText>(std::move(text), nullptr, true));
      break;
    case AttachType::Photo:
      for (auto &f : attach_files_) {
        //inputMessagePhoto photo:InputFile thumbnail:inputThumbnail added_sticker_file_ids:vector<int32> width:int32 height:int32 caption:formattedText show_caption_above_media:Bool self_destruct_type:MessageSelfDestructType has_spoiler:Bool = InputMessageContent;
        content.push_back(td::make_tl_object<td::td_api::inputMessagePhoto>(
            td::make_tl_object<td::td_api::inputFileLocal>(f), nullptr, std::vector<td::int32>{}, 0, 0, std::move(text),
            false, nullptr, false));
      }
      break;
    case AttachType::Audio:
      for (auto &f : attach_files_) {
        //inputMessageAudio audio:InputFile album_cover_thumbnail:inputThumbnail duration:int32 title:string performer:string caption:formattedText = InputMessageContent;
        content.push_back(td::make_tl_object<td::td_api::inputMessageAudio>(
            td::make_tl_object<td::td_api::inputFileLocal>(f), nullptr, 0, "", "", std::move(text)));
      }
      break;
    case AttachType::Video:
      for (auto &f : attach_files_) {
        //inputMessageVideo video:InputFile thumbnail:inputThumbnail cover:InputFile start_timestamp:int32 added_sticker_file_ids:vector<int32> duration:int32 width:int32 height:int32 supports_streaming:Bool caption:formattedText show_caption_above_media:Bool self_destruct_type:MessageSelfDestructType has_spoiler:Bool = InputMessageContent;
        content.push_back(td::make_tl_object<td::td_api::inputMessageVideo>(
            td::make_tl_object<td::td_api::inputFileLocal>(f), nullptr, nullptr, 0, std::vector<td::int32>{}, 0, 0, 0,
            true, std::move(text), false, nullptr, false));
      }
      break;
    case AttachType::Doc:
      for (auto &f : attach_files_) {
        //inputMessageDocument document:InputFile thumbnail:inputThumbnail disable_content_type_detection:Bool caption:formattedText = InputMessageContent;
        content.push_back(td::make_tl_object<td::td_api::inputMessageDocument>(
            td::make_tl_object<td::td_api::inputFileLocal>(f), nullptr, false, std::move(text)));
      }
      break;
    case AttachType::Geo:
      break;
    case AttachType::Contact:
      break;
    case AttachType::Poll:
      break;
  }
  td::tl_object_ptr<td::td_api::inputMessageReplyToMessage> reply;
  if (reply_message_id_) {
    reply = td::make_tl_object<td::td_api::inputMessageReplyToMessage>(reply_message_id_, nullptr);
    if (quote_.size() > 0) {
      if (enabled_markdown_) {
        auto R = run_request_sync(
            td::make_tl_object<td::td_api::parseMarkdown>(td::make_tl_object<td::td_api::formattedText>(
                quote_, std::vector<td::tl_object_ptr<td::td_api::textEntity>>{})));
        if (R.is_error()) {
          root()->spawn_popup_view_window(PSTRING() << "failed to parse markdown: " << R.move_as_error(), 3);
          return;
        } else {
          //@description Describes manually chosen quote from another message
          //@text Text of the quote; 0-getOption("message_reply_quote_length_max") characters. Only Bold, Italic, Underline, Strikethrough, Spoiler, and CustomEmoji entities are allowed to be kept and must be kept in the quote
          //@position Quote position in the original message in UTF-16 code units
          //inputTextQuote text:formattedText position:int32 = InputTextQuote;
          auto quote_text = R.move_as_ok();
          reply->quote_ = td::make_tl_object<td::td_api::inputTextQuote>(std::move(quote_text), 0);
        }
      }
    }
  }

  if (content.size() == 1) {
    //sendMessage chat_id:int53 message_thread_id:int53 reply_to:MessageReplyTo options:messageSendOptions reply_markup:ReplyMarkup input_message_content:InputMessageContent = Message;
    auto req = td::make_tl_object<td::td_api::sendMessage>(chat_id_, thread_id_, std::move(reply),
                                                           std::move(send_options), nullptr, std::move(content[0]));

    chat_window_->send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::message>> R) {
      DROP_IF_DELETED(R);
      if (R.is_ok()) {
        chat_window_->process_update_sent_message(R.move_as_ok());
      } else {
        spawn_error_window(*this, R.move_as_error().to_string(), {});
      }
    });
  } else {
    //sendMessageAlbum chat_id:int53 message_thread_id:int53 reply_to:InputMessageReplyTo options:messageSendOptions input_message_contents:vector<InputMessageContent> = Messages;
    auto req = td::make_tl_object<td::td_api::sendMessageAlbum>(chat_id_, thread_id_, std::move(reply),
                                                                std::move(send_options), std::move(content));

    chat_window_->send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::messages>> R) {
      DROP_IF_DELETED(R);
      if (R.is_ok()) {
        auto res = R.move_as_ok();
        for (auto &m : res->messages_) {
          chat_window_->process_update_sent_message(std::move(m));
        }
      } else {
        spawn_error_window(*this, R.move_as_error().to_string(), {});
      }
    });
  }
  editor_window_->clear();
  set_reply_message_id(0, "");
  attach_type_ = AttachType::None;
  attach_files_.clear();
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
  auto req = td::make_tl_object<td::td_api::setChatDraftMessage>(chat_id_, thread_id_, std::move(draft));

  send_request(std::move(req), [&](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {});
  editor_window_->clear();
  root()->close_compose_window();
}

void ComposeWindow::render(windows::WindowOutputter &rb, bool force) {
  {
    Outputter out;
    out << Outputter::NoLb(true);
    if (enabled_markdown_) {
      out << Outputter::Bold{true} << "[+MARKDOWN] " << Outputter::Bold{Outputter::ChangeBool::Revert};
    } else {
      out << "[-MARKDOWN] ";
    }
    if (!no_sound_) {
      out << Outputter::Bold{true} << "[+SOUND] " << Outputter::Bold{Outputter::ChangeBool::Revert};
    } else {
      out << "[-SOUND] ";
    }

    switch (attach_type_) {
      case AttachType::None:
        out << "[-ATTACH] ";
        break;
      case AttachType::Photo:
        out << Outputter::Bold{true} << "[+ATTACH: " << attach_files_.size() << " PHOTOS] "
            << Outputter::Bold{Outputter::ChangeBool::Revert};
        break;
      case AttachType::Audio:
        out << Outputter::Bold{true} << "[+ATTACH: " << attach_files_.size() << " AUDIOS] "
            << Outputter::Bold{Outputter::ChangeBool::Revert};
        break;
      case AttachType::Video:
        out << Outputter::Bold{true} << "[+ATTACH: " << attach_files_.size() << " VIDEOS] "
            << Outputter::Bold{Outputter::ChangeBool::Revert};
        break;
      case AttachType::Doc:
        out << Outputter::Bold{true} << "[+ATTACH: " << attach_files_.size() << " DOCS] "
            << Outputter::Bold{Outputter::ChangeBool::Revert};
        break;
      case AttachType::Geo:
        out << Outputter::Bold{true} << "[+ATTACH: " << attach_files_.size() << " LOC] "
            << Outputter::Bold{Outputter::ChangeBool::Revert};
        break;
      case AttachType::Contact:
        out << Outputter::Bold{true} << "[+ATTACH: " << attach_files_.size() << " CONTACT] "
            << Outputter::Bold{Outputter::ChangeBool::Revert};
        break;
      case AttachType::Poll:
        out << Outputter::Bold{true} << "[+ATTACH: " << attach_files_.size() << " POLL] "
            << Outputter::Bold{Outputter::ChangeBool::Revert};
        break;
    }

    if (quote_.size() > 0) {
      out << "[+QUOTE]";
    } else if (reply_message_id_ != 0) {
      out << "[-QUOTE]";
    }

    windows::TextEdit::render(rb, width(), out.as_cslice(), 0, out.markup(), false, false);
  }
  if (reply_message_id_) {
    rb.translate(1, 0);
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
    rb.untranslate(1, 0);
  }
}

void ComposeWindow::handle_input(const windows::InputEvent &info) {
  if (info == "M-m") {
    enabled_markdown_ = !enabled_markdown_;
    set_need_refresh();
    return;
  }
  if (info == "M-s") {
    no_sound_ = !no_sound_;
    set_need_refresh();
    return;
  }
  if (info == "M-a") {
    create_menu_window<AttachMenu>(root(), root_actor_id(), chat_id_, editor_window_->export_data(), attach_type_,
                                   attach_files_, this, window_unique_id());
    set_need_refresh();
    return;
  }
  if (info == "M-q" && reply_message_id_) {
    spawn_text_edit_window(*this, "select text to quote", quote_,
                           [self = this, reply_message_id = reply_message_id_](td::Result<std::string> res) mutable {
                             if (res.is_error()) {
                               return;
                             }
                             self->set_reply_message_id(reply_message_id, res.move_as_ok());
                           });
    return;
  }
  editor_window_->handle_input(info);
  auto chat_window = root()->chat_window();
  if (chat_window) {
    chat_window->update_visible();
  }
}

}  // namespace tdcurses
