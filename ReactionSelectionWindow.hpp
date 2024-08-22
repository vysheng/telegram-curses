#pragma once

#include "TdcursesWindowBase.hpp"
#include "ChatWindow.hpp"
#include "StickerManager.hpp"
#include "Outputter.hpp"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Random.h"
#include "td/utils/Status.h"
#include "td/utils/Variant.h"
#include "td/utils/common.h"
#include "td/utils/overloaded.h"
#include "windows/EditorWindow.hpp"
#include "windows/PadWindow.hpp"
#include <memory>
#include <vector>

namespace tdcurses {

class ReactionSelectionWindow
    : public windows::PadWindow
    , public TdcursesWindowBase {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_exit() = 0;
  };
  struct PaidReaction {
    bool operator==(const PaidReaction &other) const {
      return true;
    }
  };
  class Element : public windows::PadWindowElement {
   public:
    Element(std::string emoji, td::int32 count, bool selected)
        : emoji_(std::move(emoji)), count_(count), selected_(selected) {
    }
    Element(td::int64 emoji, td::int32 count, bool selected)
        : emoji_(std::move(emoji)), count_(count), selected_(selected) {
    }
    Element(PaidReaction, td::int32 count, bool selected) : emoji_(PaidReaction()), count_(count), selected_(selected) {
    }
    td::int32 render(PadWindow &root, TickitRenderBuffer *rb, bool is_selected) override {
      Outputter out;
      if (selected_) {
        out << Color::Yellow;
      }
      emoji_.visit(td::overloaded([&](const std::string &emoji) { out << emoji; },
                                  [&](td::int64 emoji_id) {
                                    auto e = sticker_manager().get_custom_emoji(emoji_id);
                                    if (e.size() > 0) {
                                      out << e;
                                    } else {
                                      out << "?";
                                    }
                                  },
                                  [&](const PaidReaction &) { out << "⭐"; }));
      out << " " << count_;
      if (selected_) {
        out << Color::Revert;
      }

      return render_plain_text(rb, out.as_cslice(), out.markup(), width(), 1, is_selected);
    }

    const auto &emoji() const {
      return emoji_;
    }

    bool is_less(const PadWindowElement &_other) const override {
      const auto &other = static_cast<const Element &>(_other);
      if (count_ != other.count_) {
        return count_ > other.count_;
      }
      if (emoji_.get_offset() == emoji_.offset<std::string>()) {
        if (other.emoji_.get_offset() == emoji_.offset<std::string>()) {
          return emoji_.get<std::string>() < other.emoji_.get<std::string>();
        } else {
          return false;
        }
      } else {
        if (other.emoji_.get_offset() == emoji_.offset<std::string>()) {
          return true;
        } else {
          return emoji_.get<td::int64>() < other.emoji_.get<td::int64>();
        }
      }
    }

    bool selected() const {
      return selected_;
    }

    auto set() {
      count_++;
      selected_ = true;
    }
    auto unset() {
      count_--;
      selected_ = false;
    }

   private:
    td::Variant<std::string, td::int64, PaidReaction> emoji_;
    td::int32 count_;
    bool selected_;
  };

  ReactionSelectionWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, const td::td_api::message &message)
      : TdcursesWindowBase(root, std::move(root_actor)), message_id_(ChatWindow::build_message_id(message)) {
    if (message.interaction_info_ && message.interaction_info_->reactions_) {
      for (const auto &reaction : message.interaction_info_->reactions_->reactions_) {
        td::td_api::downcast_call(const_cast<td::td_api::ReactionType &>(*reaction->type_),
                                  td::overloaded(
                                      [&](const td::td_api::reactionTypeEmoji &r) {
                                        reactions_.push_back(std::make_shared<Element>(r.emoji_, reaction->total_count_,
                                                                                       reaction->is_chosen_));
                                      },
                                      [&](const td::td_api::reactionTypeCustomEmoji &r) {
                                        reactions_.push_back(std::make_shared<Element>(
                                            r.custom_emoji_id_, reaction->total_count_, reaction->is_chosen_));
                                      },
                                      [&](const td::td_api::reactionTypePaid &r) {
                                        reactions_.push_back(std::make_shared<Element>(
                                            PaidReaction(), reaction->total_count_, reaction->is_chosen_));
                                      }));
      }
    }

    std::vector<std::string> extra_reactions{"👌", "🔥", "👍", "❤"};
    for (const auto &e : extra_reactions) {
      bool ok = false;
      for (const auto &r : reactions_) {
        if (r->emoji() == e) {
          ok = true;
          break;
        }
      }
      if (!ok) {
        reactions_.push_back(std::make_shared<Element>(e, 0, false));
      }
    }

    for (auto &element : reactions_) {
      add_element(element);
    }

    set_pad_to(PadWindow::PadTo::Top);
  }

  void handle_input(TickitKeyEventInfo *info) override {
    set_need_refresh();
    if (info->type == TICKIT_KEYEV_KEY) {
      if (!strcmp(info->str, "Enter")) {
        toggle();
        return;
      }
      if (!strcmp(info->str, "Escape")) {
        callback_->on_exit();
        return;
      }
    } else {
      if (!strcmp(info->str, " ")) {
        toggle();
        return;
      }
      if (!strcmp(info->str, "q") || !strcmp(info->str, "C-q") || !strcmp(info->str, "Q") ||
          !strcmp(info->str, "C-Q")) {
        callback_->on_exit();
        return;
      }
    }
    PadWindow::handle_input(info);
  }

  void toggle() {
    auto e = get_active_element();
    if (!e) {
      return;
    }
    auto element = std::static_pointer_cast<Element>(std::move(e));
    td::tl_object_ptr<td::td_api::ReactionType> emoji;
    element->emoji().visit(td::overloaded(
        [&](const std::string &emoji_str) { emoji = td::make_tl_object<td::td_api::reactionTypeEmoji>(emoji_str); },
        [&](td::int64 emoji_id) { emoji = td::make_tl_object<td::td_api::reactionTypeCustomEmoji>(emoji_id); },
        [&](const PaidReaction &) { emoji = td::make_tl_object<td::td_api::reactionTypePaid>(); }));
    if (!element->selected()) {
      auto req = td::make_tl_object<td::td_api::addMessageReaction>(message_id_.chat_id, message_id_.message_id,
                                                                    std::move(emoji), false, true);
      send_request(std::move(req), [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {});
      change_element(e, [&]() { element->set(); });
    } else {
      auto req = td::make_tl_object<td::td_api::removeMessageReaction>(message_id_.chat_id, message_id_.message_id,
                                                                       std::move(emoji));
      send_request(std::move(req), [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {});
      change_element(e, [&]() { element->unset(); });
    }
  }

  void set_callback(std::unique_ptr<Callback> callback) {
    callback_ = std::move(callback);
  }

 private:
  ChatWindow::MessageId message_id_;
  std::vector<std::shared_ptr<Element>> reactions_;
  std::unique_ptr<Callback> callback_;
};

}  // namespace tdcurses
