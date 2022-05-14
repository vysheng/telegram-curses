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
  class Element;

  class Element : public windows::PadWindowElement {
   public:
    Element(td::tl_object_ptr<td::td_api::message> message) : message(std::move(message)) {
    }

    td::int32 render(TickitRenderBuffer *rb, bool is_selected) override;

    bool is_less(const windows::PadWindowElement &elx) const override {
      const Element &el = static_cast<const Element &>(elx);

      return message->id_ < el.message->id_;
    }

    td::tl_object_ptr<td::td_api::message> message;
  };

  auto chat_id() const {
    return chat_id_;
  }

  void handle_input(TickitKeyEventInfo *info) override;

  void request_bottom_elements() override;
  void received_bottom_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R);
  void request_top_elements() override;
  void received_top_elements(td::Result<td::tl_object_ptr<td::td_api::messages>> R);
  void add_messages(td::tl_object_ptr<td::td_api::messages> msgs);

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
  void process_update(td::td_api::updateMessageUnreadReactions &update);
  void process_update(td::td_api::updateMessageLiveLocationViewed &update);

  std::string draft_message_text();

  void send_open();
  void send_close();

  ~ChatWindow() {
    send_close();
  }

 private:
  td::int64 chat_id_;
  std::map<td::int64, std::shared_ptr<Element>> messages_;
  bool running_req_top_{false};
  bool running_req_bottom_{false};
  bool is_completed_top_{false};
  bool is_completed_bottom_{false};
};

}  // namespace tdcurses
