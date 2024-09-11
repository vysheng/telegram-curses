#pragma once

#include "td/utils/Promise.h"
#include "windows/Screen.hpp"
#include "windows/LogWindow.hpp"
#include "windows/Window.hpp"

#include "td/tdactor/td/actor/actor.h"
#include "td/generate/auto/td/telegram/td_api.h"
#include "td/actor/PromiseFuture.h"
#include "td/tl/TlObject.h"

#include <memory>
#include <vector>

namespace tdcurses {

class TdcursesLayout;
class DialogListWindow;
class ChatWindow;
class ComposeWindow;
class ConfigWindow;
class Chat;
class StatusLineWindow;
class CommandLineWindow;
class TdcursesWindowBase;

struct TdcursesParameters {
  bool log_window_enabled{false};
  td::int32 log_window_height{10};
  td::int32 dialog_list_window_width{10};
  td::int32 compose_window_height{10};
};

class TdcursesInterface : public td::Actor {
 private:
  template <typename T>
  static inline auto create_promise(td::Promise<td::tl_object_ptr<T>> P) {
    return td::PromiseCreator::lambda([P = std::move(P)](td::Result<td::tl_object_ptr<td::td_api::Object>> R) mutable {
      if (R.is_error()) {
        P.set_error(R.move_as_error());
      } else {
        auto res = R.move_as_ok();
        if (res->get_id() == td::td_api::error::ID) {
          auto err = td::move_tl_object_as<td::td_api::error>(std::move(res));
          P.set_error(td::Status::Error(err->code_, err->message_));
        } else {
          P.set_value(td::move_tl_object_as<T>(std::move(res)));
        }
      }
    });
  }

 public:
  virtual ~TdcursesInterface() = default;

  template <class T>
  inline void send_request(td::tl_object_ptr<T> func, td::Promise<typename T::ReturnType> P) {
    using RetType = typename T::ReturnType;
    using RetTlType = typename RetType::element_type;
    auto Q = create_promise<RetTlType>(std::move(P));
    do_send_request(td::move_tl_object_as<td::td_api::Function>(std::move(func)), std::move(Q));
  }

  template <class T>
  static void send_request(td::ActorId<TdcursesInterface> aid, td::tl_object_ptr<T> func,
                           td::Promise<typename T::ReturnType> P) {
    using RetType = typename T::ReturnType;
    using RetTlType = typename RetType::element_type;
    auto Q = create_promise<RetTlType>(std::move(P));
    td::send_closure(aid, &TdcursesInterface::do_send_request,
                     td::move_tl_object_as<td::td_api::Function>(std::move(func)), std::move(Q));
  }

  virtual void do_send_request(td::tl_object_ptr<td::td_api::Function> func,
                               td::Promise<td::tl_object_ptr<td::td_api::Object>> cb) = 0;
};

class Tdcurses : public TdcursesInterface {
 public:
  struct Option {
    Option(std::string name, std::vector<std::string> values, std::function<void(std::string)> on_change,
           td::uint32 cur_selected = 0)
        : name(std::move(name)), values(std::move(values)), on_change(on_change), cur_selected(cur_selected) {
    }
    Option(std::string name, std::vector<std::string> n_values, std::function<void(std::string)> on_change,
           std::string f, td::uint32 cur_selected = 0)
        : name(std::move(name)), values(std::move(n_values)), on_change(on_change), cur_selected(cur_selected) {
      for (td::uint32 i = 0; i < values.size(); i++) {
        if (values[i] == f) {
          cur_selected = i;
          return;
        }
      }
    }
    std::string name;
    std::vector<std::string> values;
    std::function<void(std::string)> on_change;
    td::uint32 cur_selected{0};

    void update() {
      if (!values.size()) {
        return;
      }
      cur_selected++;
      if (cur_selected >= values.size()) {
        cur_selected = 0;
      }
      on_change(values[cur_selected]);
    }
  };

  void initialize_options(TdcursesParameters &params);

 private:
  td::ActorId<Tdcurses> self_;
  std::unique_ptr<windows::Screen> screen_;
  std::shared_ptr<TdcursesLayout> layout_;
  std::shared_ptr<windows::LogWindow> log_window_;
  std::unique_ptr<td::LogInterface> log_interface_;
  std::shared_ptr<windows::Window> qr_code_window_;
  std::shared_ptr<DialogListWindow> dialog_list_window_;
  std::shared_ptr<ChatWindow> chat_window_;
  std::shared_ptr<ComposeWindow> compose_window_;
  std::shared_ptr<StatusLineWindow> status_line_window_;
  std::shared_ptr<CommandLineWindow> command_line_window_;
  std::map<td::int64, TdcursesWindowBase *> all_active_windows_;

  std::vector<Option> options_;

 public:
  Tdcurses() {
  }

  void start_curses(TdcursesParameters &params);

  bool window_exists(td::int64 id);

  void add_qr_code_window(std::shared_ptr<windows::Window> window) {
    if (qr_code_window_) {
      del_popup_window(qr_code_window_.get());
    }
    qr_code_window_ = window;
    add_popup_window(window, 0);
  }

  void del_qr_code_window() {
    if (qr_code_window_) {
      del_popup_window(qr_code_window_.get());
      qr_code_window_ = nullptr;
    }
  }

  void spawn_popup_view_window(std::string text, td::int32 priority) {
    spawn_popup_view_window(std::move(text), {}, priority);
  }
  void spawn_popup_view_window(std::string text, std::vector<windows::MarkupElement> markup, td::int32 priority);

  void spawn_chat_selection_window(bool local, td::Promise<std::shared_ptr<Chat>> promise);

  void add_popup_window(std::shared_ptr<windows::Window> window, td::int32 prio) {
    screen_->add_popup_window(std::move(window), prio);
  }
  void del_popup_window(windows::Window *window) {
    screen_->del_popup_window(window);
  }
  void open_chat(td::int64 chat_id);
  void seek_chat(td::int64 chat_id, td::int64 message_id);
  void open_compose_window(td::int64 chat_id, td::int64 message_id);
  void close_compose_window();

  void show_config_window();

  void loop() override;
  void refresh();
  void tear_down() override;

  auto width() const {
    return screen_->width();
  }
  auto height() const {
    return screen_->height();
  }
  auto dialog_list_window() const {
    return dialog_list_window_;
  }
  auto status_line_window() const {
    return status_line_window_;
  }
  auto command_line_window() const {
    return command_line_window_;
  }
  auto chat_window() const {
    return chat_window_;
  }

  void start_up() override {
  }
  void on_close() {
  }

  auto &screen() {
    return screen_;
  }
  auto &layout() {
    return layout_;
  }

  void register_alive_window(TdcursesWindowBase *window);
  void unregister_alive_window(TdcursesWindowBase *window);

  virtual void update_status_line() = 0;
};

}  // namespace tdcurses
