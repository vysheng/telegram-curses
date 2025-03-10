#pragma once
#include "td/actor/impl/ActorId-decl.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "td/utils/Variant.h"
#include "td/utils/overloaded.h"
#include "windows/Output.hpp"
#include "windows/PadWindow.hpp"
#include "td/generate/auto/td/telegram/td_api.h"
#include "td/generate/auto/td/telegram/td_api.hpp"
#include "TdcursesWindowBase.hpp"
#include "MenuWindow.hpp"
#include <memory>
#include <set>
#include <vector>

namespace tdcurses {

class Tdcurses;

class ChatWindow
    : public windows::PadWindow
    , public TdcursesWindowBase {
 public:
  struct MessageId {
    MessageId(td::int64 chat_id, td::int64 message_id) : chat_id(chat_id), message_id(message_id) {
    }
    td::int64 chat_id;
    td::int64 message_id;
    bool operator<(const MessageId &other) const {
      return std::tie(chat_id, message_id) < std::tie(other.chat_id, other.message_id);
    }
    bool operator==(const MessageId &other) const {
      return std::tie(chat_id, message_id) == std::tie(other.chat_id, other.message_id);
    }
  };
  struct ModeDefault {
    bool operator==(const ModeDefault &) const {
      return true;
    }
  };
  struct ModeSearch {
    ModeSearch(std::string pattern) : search_pattern(std::move(pattern)) {
    }
    std::string search_pattern;
    bool operator==(const ModeSearch &other) const {
      return search_pattern == other.search_pattern;
    }
  };
  struct ModeComments {
    ModeComments(MessageId message_id, td::int64 thread_id) : message_id(message_id), thread_id(thread_id) {
    }
    MessageId message_id;
    td::int64 thread_id;
    bool operator==(const ModeComments &other) const {
      return message_id == other.message_id && thread_id == other.thread_id;
    }
  };
  using Mode = td::Variant<ModeDefault, ModeSearch, ModeComments>;
  ChatWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id);

  class Element : public windows::PadWindowElement {
   public:
    Element(td::tl_object_ptr<td::td_api::message> message, td::int32 generation)
        : message(std::move(message)), generation(generation) {
    }

    td::int32 render(windows::PadWindow &root, windows::WindowOutputter &rb, windows::SavedRenderedImagesDirectory &dir,
                     bool is_selected) override;

    bool is_less(const windows::PadWindowElement &elx) const override {
      const Element &el = static_cast<const Element &>(elx);

      return (generation < el.generation) || (generation == el.generation && message->id_ < el.message->id_);
    }

    void handle_input(PadWindow &root, const windows::InputEvent &info) override;

    void run(ChatWindow *window);

    td::tl_object_ptr<td::td_api::message> message;
    td::int32 generation;

    auto message_id() const {
      return MessageId{message->chat_id_, message->id_};
    }
  };

  td::int64 main_chat_id() const {
    return main_chat_id_;
  }

  void handle_input(const windows::InputEvent &info) override;

  void request_bottom_elements_ex(td::int32 message_id);
  void request_bottom_elements() override {
    request_bottom_elements_ex(0);
  }
  void received_bottom_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R, td::int64 pivot_message_id);
  void received_bottom_search_elements(td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R,
                                       td::int64 pivot_message_id);
  void request_top_elements_ex(td::int32 message_id);
  void request_top_elements() override {
    request_top_elements_ex(0);
  }
  void received_top_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R);
  void received_top_search_elements(td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R);
  void add_messages(std::vector<td::tl_object_ptr<td::td_api::message>> msgs);

  void process_update(td::td_api::updateNewMessage &update);
  void process_update_sent_message(td::tl_object_ptr<td::td_api::message> message);
  void process_update(td::td_api::updateMessageSendAcknowledged &update);
  void process_update(td::td_api::updateMessageSendSucceeded &update);
  void process_update(td::td_api::updateMessageSendFailed &update);
  void process_update(td::td_api::updateMessageContent &update);
  void process_update(td::td_api::updateMessageEdited &update);
  void process_update(td::td_api::updateMessageIsPinned &update);
  void process_update(td::td_api::updateMessageInteractionInfo &update);
  void process_update(td::td_api::updateMessageContentOpened &update);
  void process_update(td::td_api::updateMessageMentionRead &update);
  void process_update(td::td_api::updateMessageFactCheck &update);
  void process_update(td::td_api::updateMessageUnreadReactions &update);
  void process_update(td::td_api::updateMessageLiveLocationViewed &update);
  void process_update(td::td_api::updateDeleteMessages &update);
  void process_update(td::td_api::updateChatAction &update);
  void process_file_update(const td::td_api::updateFile &update);

  void update_file(MessageId message_id, const td::td_api::file &file);

  std::string draft_message_text();

  void send_open();
  void send_close();

  ~ChatWindow() {
    send_close();
    clear_file_subscriptions();
  }

  static td::int32 get_file_id(const td::td_api::message &message);
  static void update_message_file(td::td_api::message &message, const td::td_api::file &file);

  void del_file_message_pair(MessageId msg_id, td::int32 file_id);
  void add_file_message_pair(MessageId msg_id, td::int32 file_id);
  void clear_file_subscriptions();

  std::shared_ptr<Element> get_message(MessageId message_id) {
    auto it = messages_.find(message_id);
    if (it != messages_.end()) {
      return it->second;
    } else {
      return nullptr;
    }
  }

  const td::td_api::message *get_message_as_message(MessageId message_id) {
    auto it = messages_.find(message_id);
    if (it != messages_.end()) {
      return it->second->message.get();
    } else {
      return nullptr;
    }
  }

  const td::td_api::message *get_message_as_message(td::int64 chat_id, td::int64 message_id) {
    return get_message_as_message(MessageId{chat_id, message_id});
  }

  bool is_main_mode(const Mode &mode) const {
    return mode.get_offset() == mode.offset<ModeDefault>();
  }
  bool is_main_mode() const {
    return is_main_mode(mode_);
  }
  bool is_search_mode(const Mode &mode) const {
    return mode.get_offset() == mode.offset<ModeSearch>();
  }
  bool is_search_mode() const {
    return is_search_mode(mode_);
  }
  bool is_comments_mode(const Mode &mode) const {
    return mode.get_offset() == mode.offset<ModeComments>();
  }
  bool is_comments_mode() const {
    return is_comments_mode(mode_);
  }

  std::string text_mode() const {
    std::string result;
    mode_.visit(td::overloaded([&](const ModeDefault &) { result = ""; },
                               [&](const ModeSearch &m) { result = "/" + m.search_pattern; },
                               [&](const ModeComments &m) { result = "comments"; }));
    return result;
  }

  void set_search_pattern(std::string pattern) {
    set_mode(Mode{ModeSearch{pattern}});
  }
  void set_comments_mode(MessageId message_id, td::int64 thread_id) {
    set_mode(Mode{ModeComments{message_id, thread_id}});
  }

  void set_mode(Mode mode);

  void show_message_actions();

  void seek(td::int64 chat_id, td::int64 message_id);
  void seek(MessageId message_id);

  static auto build_message_id(const td::td_api::message &m) {
    return MessageId{m.chat_id_, m.id_};
  }
  static auto build_message_id(td::int64 chat_id, td::int64 message_id) {
    return MessageId{chat_id, message_id};
  }

  td::int32 get_chat_generation(td::int64 chat_id) const {
    return chat_id == main_chat_id_ ? 0 : -1;
  }

  MessageId get_oldest_message_id();
  MessageId get_newest_message_id();

  auto multi_message_selection_mode() const {
    return multi_message_selection_mode_;
  }

  bool multi_message_selection_mode_is_selected(MessageId id) const {
    return multi_message_selection_mode_ && selected_messages_.count(id) > 0;
  }

  void clear_multi_message_selection_mode() {
    multi_message_selection_mode_ = false;
    selected_messages_.clear();
  }

  void update_visible();

 private:
  const td::int64 main_chat_id_;

  std::map<MessageId, std::shared_ptr<Element>> messages_;
  struct FileSubscription {
    FileSubscription(td::int64 id) : subscription_id(id) {
    }
    const td::int64 subscription_id;
    std::set<MessageId> messages;
  };
  std::map<td::int32, FileSubscription> file_id_2_messages_;
  Mode mode_{ModeDefault{}};
  bool running_req_top_{false};
  bool running_req_bottom_{false};
  bool is_completed_top_{false};
  bool is_completed_bottom_{false};

  bool multi_message_selection_mode_{false};
  std::set<MessageId> selected_messages_;
};

}  // namespace tdcurses
