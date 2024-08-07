#include "td/actor/impl/ActorId-decl.h"
#include "td/tl/TlObject.h"
#include "td/utils/Status.h"
#include "windows/padwindow.h"
#include "td/generate/auto/td/telegram/td_api.h"
#include "td/generate/auto/td/telegram/td_api.hpp"
#include "telegram-curses.h"
#include "telegram-curses-window-root.h"
#include <memory>

namespace tdcurses {

class Tdcurses;

class ChatWindow
    : public windows::PadWindow
    , public TdcursesWindowBase {
 public:
  ChatWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, td::int64 chat_id)
      : TdcursesWindowBase(root, std::move(root_actor)), chat_id_(chat_id) {
    set_pad_to(PadTo::Bottom);
    scroll_last_line();
    send_open();
  }

  class Element : public windows::PadWindowElement {
   public:
    Element(td::tl_object_ptr<td::td_api::message> message) : message(std::move(message)) {
    }

    td::int32 render(windows::PadWindow &root, TickitRenderBuffer *rb, bool is_selected) override;

    bool is_less(const windows::PadWindowElement &elx) const override {
      const Element &el = static_cast<const Element &>(elx);

      return message->id_ < el.message->id_;
    }

    void run(ChatWindow *window);

    td::tl_object_ptr<td::td_api::message> message;
  };

  auto chat_id() const {
    return chat_id_;
  }

  void handle_input(TickitKeyEventInfo *info) override;

  void request_bottom_elements_ex(td::int32 message_id);
  void request_bottom_elements() override {
    request_bottom_elements_ex(0);
  }
  void received_bottom_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R);
  void received_bottom_search_elements(td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R);
  void request_top_elements_ex(td::int32 message_id);
  void request_top_elements() override {
    request_top_elements_ex(0);
  }
  void received_top_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R);
  void received_top_search_elements(td::Result<td::tl_object_ptr<td::td_api::foundChatMessages>> R);
  void add_messages(std::vector<td::tl_object_ptr<td::td_api::message>> msgs);

  void process_update(td::td_api::updateNewMessage &update);
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
  void process_update(td::td_api::updateFile &update);

  void update_file(td::int64 message_id, td::td_api::file &file);

  std::string draft_message_text();

  void send_open();
  void send_close();

  ~ChatWindow() {
    send_close();
  }

  static td::int32 get_file_id(const td::td_api::message &message);
  static void update_message_file(td::td_api::message &message, const td::td_api::file &file);

  void del_file_message_pair(td::int64 msg_id, td::int32 file_id) {
    if (!file_id) {
      return;
    }
    auto it = file_id_2_messages_.find(file_id);
    CHECK(it != file_id_2_messages_.end());
    it->second.erase(msg_id);
    if (it->second.size() == 0) {
      file_id_2_messages_.erase(it);
    }
  }

  void add_file_message_pair(td::int64 msg_id, td::int32 file_id) {
    if (!file_id) {
      return;
    }
    file_id_2_messages_[file_id].insert(msg_id);
  }

  std::shared_ptr<Element> get_message(td::int64 message_id) {
    auto it = messages_.find(message_id);
    if (it != messages_.end()) {
      return it->second;
    } else {
      return nullptr;
    }
  }

  void set_search_pattern(std::string pattern);
  const auto &search_pattern() const {
    return search_pattern_;
  }

  void show_message_actions();

  void seek(td::int64 chat_id, td::int64 message_id);

 private:
  td::int64 chat_id_;
  std::map<td::int64, std::shared_ptr<Element>> messages_;
  std::map<td::int32, std::set<td::int64>> file_id_2_messages_;
  std::string search_pattern_;
  bool running_req_top_{false};
  bool running_req_bottom_{false};
  bool is_completed_top_{false};
  bool is_completed_bottom_{false};
};

}  // namespace tdcurses
