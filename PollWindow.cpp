#include "PollWindow.hpp"
#include "common-windows/LoadingWindow.hpp"
#include "common-windows/ErrorWindow.hpp"
#include "TdObjectsOutput.h"
#include "auto/td/telegram/td_api.hpp"
#include "td/telegram/td_api.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "td/utils/overloaded.h"
#include <vector>

namespace tdcurses {

void PollWindow::select(td::int32 idx) {
  if (already_voted_) {
    return;
  }
  if (options_to_be_set_.count(idx) > 0) {
    options_to_be_set_.erase(idx);
    process_message();
  } else if (options_to_be_set_.size() == 0 || allow_multichoise_) {
    options_to_be_set_.insert(idx);
    process_message();
  }
}

void PollWindow::update_poll(td::tl_object_ptr<td::td_api::poll> poll) {
  poll_ = std::move(poll);
  clear();

  bool already_voted = false;
  for (td::int32 idx = 0; idx < (td::int32)poll_->options_.size(); idx++) {
    auto &el = poll_->options_[idx];
    if (el->is_chosen_ || el->is_being_chosen_) {
      already_voted = true;
      options_to_be_set_.insert(idx);
    }
  }
  already_voted_ = already_voted;

  td::td_api::downcast_call(*poll_->type_, td::overloaded(
                                               [&](const td::td_api::pollTypeRegular &o) {
                                                 is_quiz_ = false;
                                                 allow_multichoise_ = o.allow_multiple_answers_;
                                               },
                                               [&](const td::td_api::pollTypeQuiz &o) {
                                                 is_quiz_ = true;
                                                 allow_multichoise_ = false;
                                               }));
}

void PollWindow::process_message() {
  clear();

  /* question */ {
    Outputter out;
    out << *poll_->question_;
    add_element("question", out.as_str(), out.markup());
  }

  /* options */
  for (td::int32 idx = 0; idx < (td::int32)poll_->options_.size(); idx++) {
    auto &el = poll_->options_[idx];
    Outputter out;
    if (already_voted_) {
      if (options_to_be_set_.count(idx)) {
        out << Outputter::Reverse{true};
        if (!is_quiz_) {
          out << Color::Yellow;
        } else if (static_cast<const td::td_api::pollTypeQuiz &>(*poll_->type_).correct_option_id_ == idx) {
          out << Color::Green;
        } else {
          out << Color::Red;
        }
      }
    } else {
      if (options_to_be_set_.count(idx)) {
        out << Color::Green;
      }
    }
    out << *el->text_;

    if (already_voted_) {
      out << Outputter::RightPad{PSTRING() << el->vote_percentage_ << "% (" << el->voter_count_ << ")"};
    }
    add_element("option", out.as_str(), out.markup(), [self = this, idx]() {
      self->select(idx);
      return false;
    });
  }

  /* retract vote */
  if (already_voted_ && !is_quiz_) {
    add_element("retract", "retract my choices", {}, [self = this]() -> bool {
      auto req =
          td::make_tl_object<td::td_api::setPollAnswer>(self->chat_id_, self->message_id_, std::vector<td::int32>());

      loading_window_send_request(
          *self, "sending vote...", {}, std::move(req), [self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
            DROP_IF_DELETED(R);
            if (R.is_error()) {
              spawn_error_window(*self, PSTRING() << "failed to retract vote: " << R.move_as_error(), {});
            } else {
              self->options_to_be_set_.clear();
              self->already_voted_ = false;
              self->process_message();
            }
          });

      return false;
    });
  }

  /* send vote */
  if (!already_voted_ && options_to_be_set_.size() > 0) {
    add_element("submit", "send currently selected options", {}, [self = this]() -> bool {
      auto opts = std::vector<td::int32>(self->options_to_be_set_.begin(), self->options_to_be_set_.end());
      auto req = td::make_tl_object<td::td_api::setPollAnswer>(self->chat_id_, self->message_id_, std::move(opts));

      loading_window_send_request(
          *self, "sending vote...", {}, std::move(req), [self](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {
            DROP_IF_DELETED(R);
            if (R.is_error()) {
              spawn_error_window(*self, PSTRING() << "failed to send vote: " << R.move_as_error(), {});
            } else {
              self->already_voted_ = true;
              self->process_message();
            }
          });

      return false;
    });
  }

  /* explanation */
  if (already_voted_ && is_quiz_) {
    const auto &e = static_cast<const td::td_api::pollTypeQuiz &>(*poll_->type_).explanation_;
    if (e) {
      Outputter out;
      out << *e;
      add_element("explanation", out.as_str(), out.markup());
    }
  }
}

}  // namespace tdcurses
