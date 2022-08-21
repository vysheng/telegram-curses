#pragma once

#include "td/actor/impl/ActorId-decl.h"
#include "td/actor/impl/Scheduler-decl.h"
#include "telegram-curses.hpp"

namespace tdcurses {

class Tdcurses;

class TdcursesWindowBase {
 public:
  TdcursesWindowBase(Tdcurses *root, td::ActorId<Tdcurses> root_actor)
      : root_(root), root_actor_(std::move(root_actor)) {
    change_unique_id();
  }
  Tdcurses *root() {
    return root_;
  }

  template <class T>
  void send_request(td::tl_object_ptr<T> func, td::Promise<typename T::ReturnType> P) {
    using RetType = typename T::ReturnType;
    using RetTlType = typename RetType::element_type;
    auto Q = td::PromiseCreator::lambda([id = unique_id_, self = root_actor_, self_ptr = root_, P = std::move(P)](
                                            td::Result<td::tl_object_ptr<td::td_api::Object>> R) mutable {
      td::send_lambda(self, [id, self_ptr, R = std::move(R), P = std::move(P)]() mutable {
        if (!self_ptr->window_exists(id)) {
          P.set_error(td::Status::Error("window already deleted"));
          return;
        }
        if (R.is_error()) {
          P.set_error(R.move_as_error());
        } else {
          auto res = R.move_as_ok();
          if (res->get_id() == td::td_api::error::ID) {
            auto err = td::move_tl_object_as<td::td_api::error>(std::move(res));
            P.set_error(td::Status::Error(err->code_, err->message_));
          } else {
            P.set_value(td::move_tl_object_as<RetTlType>(std::move(res)));
          }
        }
        self_ptr->refresh();
      });
    });
    td::send_closure(root_actor_, &Tdcurses::do_send_request,
                     td::move_tl_object_as<td::td_api::Function>(std::move(func)), std::move(Q));
  }

  td::int64 window_unique_id() const {
    return unique_id_;
  }

  void change_unique_id() {
    unique_id_ = ++last_unique_id;
  }

 private:
  Tdcurses *root_;
  td::ActorId<Tdcurses> root_actor_;
  td::int64 unique_id_;
  static std::atomic<td::int64> last_unique_id;
};

}  // namespace tdcurses
