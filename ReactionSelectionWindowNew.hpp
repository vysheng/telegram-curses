#pragma once

#include "TdcursesWindowBase.hpp"
#include "ChatWindow.hpp"
#include "StickerManager.hpp"
#include "Outputter.hpp"
#include "MenuWindowCommon.hpp"
#include "MenuWindowView.hpp"
#include "td/telegram/td_api.h"
#include "td/telegram/td_api.hpp"
#include "td/tl/TlObject.h"
#include "td/utils/Random.h"
#include "td/utils/Status.h"
#include "td/utils/Variant.h"
#include "td/utils/common.h"
#include "td/utils/overloaded.h"
#include "windows/EditorWindow.hpp"
#include "windows/Output.hpp"
#include "windows/PadWindow.hpp"
#include <algorithm>
#include <memory>
#include <vector>

namespace tdcurses {

class ReactionType;

class ReactionTypePaid {
 public:
  ReactionTypePaid() {
  }
  ReactionTypePaid(const td::td_api::reactionTypePaid &) {
  }
  void output(Outputter &out) const {
    out << "⭐";
  }
  td::tl_object_ptr<td::td_api::ReactionType> get_tl() const {
    return td::make_tl_object<td::td_api::reactionTypePaid>();
  }
  bool operator<(const ReactionTypePaid &other) const {
    return false;
  }
  bool operator==(const ReactionTypePaid &other) const {
    return true;
  }
};

class ReactionTypeEmoji {
 public:
  ReactionTypeEmoji(std::string emoji) : emoji_(std::move(emoji)) {
  }
  ReactionTypeEmoji(const td::td_api::reactionTypeEmoji &emoji) : emoji_(emoji.emoji_) {
  }
  void output(Outputter &out) const {
    out << emoji_;
  }
  td::tl_object_ptr<td::td_api::ReactionType> get_tl() const {
    return td::make_tl_object<td::td_api::reactionTypeEmoji>(emoji_);
  }
  bool operator<(const ReactionTypeEmoji &other) const {
    return emoji_ < other.emoji_;
  }
  bool operator==(const ReactionTypeEmoji &other) const {
    return emoji_ == other.emoji_;
  }

 private:
  std::string emoji_;
};

class ReactionTypeCustomEmoji {
 public:
  ReactionTypeCustomEmoji(td::int64 emoji_id) : emoji_id_(emoji_id) {
  }
  ReactionTypeCustomEmoji(const td::td_api::reactionTypeCustomEmoji &emoji) : emoji_id_(emoji.custom_emoji_id_) {
    auto e = sticker_manager().get_custom_emoji(emoji_id_);
  }
  void output(Outputter &out) const {
    auto e = sticker_manager().get_custom_emoji(emoji_id_);
    if (e.size() > 0) {
      out << e;
    } else {
      out << "?";
    }
  }
  td::tl_object_ptr<td::td_api::ReactionType> get_tl() const {
    return td::make_tl_object<td::td_api::reactionTypeCustomEmoji>(emoji_id_);
  }
  bool operator<(const ReactionTypeCustomEmoji &other) const {
    return emoji_id_ < other.emoji_id_;
  }
  bool operator==(const ReactionTypeCustomEmoji &other) const {
    return emoji_id_ == other.emoji_id_;
  }

 private:
  td::int64 emoji_id_;
};

class ReactionType {
 public:
  struct Paid {};
  ReactionType(Paid &) : value_(ReactionTypePaid{}) {
  }
  ReactionType(std::string emoji) : value_(ReactionTypeEmoji{std::move(emoji)}) {
  }
  ReactionType(td::int64 emoji_id) : value_(ReactionTypeCustomEmoji{std::move(emoji_id)}) {
  }
  ReactionType(const td::td_api::ReactionType &r) {
    td::td_api::downcast_call(
        const_cast<td::td_api::ReactionType &>(r),
        td::overloaded([&](const td::td_api::reactionTypePaid &r) { value_ = ReactionTypePaid{r}; },
                       [&](const td::td_api::reactionTypeEmoji &r) { value_ = ReactionTypeEmoji{r}; },
                       [&](const td::td_api::reactionTypeCustomEmoji &r) { value_ = ReactionTypeCustomEmoji{r}; }));
  }

  void output(Outputter &out) const {
    value_.visit([&](const auto &f) { f.output(out); });
  }
  td::tl_object_ptr<td::td_api::ReactionType> get_tl() const {
    td::tl_object_ptr<td::td_api::ReactionType> res;
    value_.visit([&](const auto &f) { res = f.get_tl(); });
    return res;
  }

  bool operator<(const ReactionType &other) const {
    auto of1 = value_.get_offset();
    auto of2 = other.value_.get_offset();
    if (of1 != of2) {
      return of1 < of2;
    }
    bool res = false;

    value_.visit([&](const auto &f) { res = f < other.value_.get<decltype(f)>(); });
    return res;
  }

  bool operator==(const ReactionType &other) const {
    auto of1 = value_.get_offset();
    auto of2 = other.value_.get_offset();
    if (of1 != of2) {
      return false;
    }
    bool res = false;

    value_.visit([&](const auto &f) { res = f == other.value_.get<decltype(f)>(); });
    return res;
  }

 private:
  td::Variant<ReactionTypePaid, ReactionTypeEmoji, ReactionTypeCustomEmoji> value_;
};

class ReactionSelectionWindowNew : public MenuWindowView {
 public:
  class ReactionElement {
   public:
    template <typename T>
    ReactionElement(const T &v, td::int32 count = -1, bool is_chosen = false)
        : reaction_(v), count_(count), is_chosen_(is_chosen) {
    }
    bool operator<(const ReactionElement &other) const {
      return count_ < other.count_ || (count_ == other.count_ && reaction_ < other.reaction_);
    }
    void output(Outputter &out) const {
      if (is_chosen_) {
        out << Color::Yellow;
      }
      out << "[";
      reaction_.output(out);
      if (count_ >= 0) {
        out << count_;
      }
      out << "]";
      if (is_chosen_) {
        out << Color::Revert;
      }
    }

    const auto &reaction() const {
      return reaction_;
    }

    td::tl_object_ptr<td::td_api::ReactionType> get_tl() const {
      return reaction_.get_tl();
    }

    bool is_chosen() const {
      return is_chosen_;
    }

    void set_chosen(bool value) {
      is_chosen_ = value;
    }

    auto incr_count() {
      count_++;
    }
    auto decr_count() {
      count_--;
    }

   private:
    ReactionType reaction_;
    td::int32 count_;
    bool is_chosen_;
  };

  std::vector<std::vector<ReactionElement>> elements_;

  size_t y_pos_{0}, x_pos_{0};
  td::uint32 reactions_in_one_row_{1};

 private:
  ChatWindow::MessageId message_id_;
  const td::int32 pad_size = 6;
  bool has_active_reactions_{false};

 public:
  ReactionSelectionWindowNew(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id, td::int64 message_id)
      : MenuWindowView(root, std::move(root_actor)), message_id_(ChatWindow::build_message_id(chat_id, message_id)) {
    request_message();
  }

  void request_message();
  void request_popular_reactions();

  void got_message(td::Result<td::tl_object_ptr<td::td_api::message>> R) {
    if (R.is_error()) {
      exit();
      return;
    }
    auto message_ptr = R.move_as_ok();
    auto &message = *message_ptr;

    if (message.interaction_info_ && message.interaction_info_->reactions_ &&
        message.interaction_info_->reactions_->reactions_.size() > 0) {
      has_active_reactions_ = true;
      elements_.emplace_back();
      for (const auto &reaction : message.interaction_info_->reactions_->reactions_) {
        elements_.back().emplace_back(*reaction->type_, reaction->total_count_, reaction->is_chosen_);
      }
    }

    request_popular_reactions();
  }

  void got_message_available_reactions(td::Result<td::tl_object_ptr<td::td_api::availableReactions>> R) {
    if (R.is_error()) {
      exit();
      return;
    }

    std::vector<ReactionType> chosen_emojis;

    if (elements_.size() > 0) {
      for (auto &e : elements_[0]) {
        if (e.is_chosen()) {
          chosen_emojis.emplace_back(e.reaction());
        }
      }
    }

    std::sort(chosen_emojis.begin(), chosen_emojis.end());

    const auto handle_reaction_list = [&](const std::vector<td::tl_object_ptr<td::td_api::availableReaction>> &vec) {
      if (vec.size() > 0) {
        elements_.emplace_back();
        for (auto &e : vec) {
          elements_.back().emplace_back(
              *e->type_, -1, std::binary_search(chosen_emojis.begin(), chosen_emojis.end(), ReactionType{*e->type_}));
        }
      }
    };

    auto ar = R.move_as_ok();
    handle_reaction_list(ar->top_reactions_);
    handle_reaction_list(ar->recent_reactions_);
    handle_reaction_list(ar->popular_reactions_);

    if (elements_.size() == 0) {
      exit();
      return;
    }

    update_text();
  }

  void update_text() {
    reactions_in_one_row_ = std::max(1, width() / pad_size);
    Outputter out;
    for (size_t i = 0; i < elements_.size(); i++) {
      for (size_t j = 0; j < elements_[i].size(); j++) {
        if (j != 0 && j % reactions_in_one_row_ == 0) {
          out << "\n";
        }
        out << Outputter::SoftTab{pad_size};
        if (i == y_pos_ && j == x_pos_) {
          out << Outputter::Reverse{Outputter::ChangeBool::Enable};
        }
        elements_[i][j].output(out);
        if (i == y_pos_ && j == x_pos_) {
          out << Outputter::Reverse{Outputter::ChangeBool::Revert};
        }
      }
      if (i != elements_.size() - 1) {
        out << "\n";
      }
    }

    replace_text(out.as_str(), out.markup());
  }

  void handle_input(const windows::InputEvent &info) override {
    set_need_refresh();
    if (info == "T-Enter" || info == " ") {
      toggle();
      return;
    } else if (info == "h" || info == "T-Left" || info == "C-b") {
      if (x_pos_ % reactions_in_one_row_ != 0) {
        x_pos_--;
        update_text();
      }
      return;
    } else if (info == "l" || info == "T-Right" || info == "C-f") {
      if (x_pos_ + 1 < elements_[y_pos_].size() && (x_pos_ + 1) % reactions_in_one_row_ != 0) {
        x_pos_++;
        update_text();
      }
      return;
    } else if (info == "k" || info == "T-Up") {
      if (x_pos_ >= reactions_in_one_row_) {
        x_pos_ -= reactions_in_one_row_;
        update_text();
      } else if (y_pos_ > 0) {
        y_pos_--;
        if (x_pos_ >= elements_[y_pos_].size()) {
          x_pos_ = elements_[y_pos_].size() - 1;
        }
        update_text();
      }
      return;
    } else if (info == "j" || info == "T-Down") {
      if (x_pos_ + reactions_in_one_row_ < elements_[y_pos_].size()) {
        x_pos_ += reactions_in_one_row_;
        update_text();
      } else if (y_pos_ + 1 < elements_.size()) {
        y_pos_++;
        if (x_pos_ >= elements_[y_pos_].size()) {
          x_pos_ = elements_[y_pos_].size() - 1;
        }
        update_text();
      }
      return;
    }
    MenuWindowView::handle_input(info);
  }

  void toggle() {
    auto &el = elements_[y_pos_][x_pos_];
    auto tl = el.get_tl();
    bool is_chosen = el.is_chosen();
    if (!is_chosen) {
      auto req = td::make_tl_object<td::td_api::addMessageReaction>(message_id_.chat_id, message_id_.message_id,
                                                                    std::move(tl), false, true);
      send_request(std::move(req), [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {});
      for (auto &e_l : elements_) {
        for (auto &e : e_l) {
          if (e.reaction() == el.reaction()) {
            e.set_chosen(true);
          }
        }
      }

      if (!has_active_reactions_) {
        elements_.insert(elements_.begin(), {});
        has_active_reactions_ = true;
        y_pos_++;
      }

      bool found = false;
      for (auto &e : elements_[0]) {
        if (e.reaction() == el.reaction()) {
          found = true;
          e.incr_count();
          break;
        }
      }

      if (!found) {
        elements_[0].emplace_back(*el.get_tl(), 1, true);
      }
    } else {
      auto req = td::make_tl_object<td::td_api::removeMessageReaction>(message_id_.chat_id, message_id_.message_id,
                                                                       std::move(tl));
      send_request(std::move(req), [](td::Result<td::tl_object_ptr<td::td_api::ok>> R) {});
      for (auto &e_l : elements_) {
        for (auto &e : e_l) {
          if (e.reaction() == elements_[y_pos_][x_pos_].reaction()) {
            e.set_chosen(false);
          }
        }
      }

      CHECK(has_active_reactions_);
      bool found = false;
      for (auto &e : elements_[0]) {
        if (e.reaction() == el.reaction()) {
          found = true;
          e.decr_count();
          break;
        }
      }

      CHECK(found);
    }

    update_text();
  }
};

}  // namespace tdcurses
