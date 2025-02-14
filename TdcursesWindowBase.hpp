#pragma once

#include "td/actor/impl/ActorId-decl.h"
#include "td/actor/impl/Scheduler-decl.h"
#include "Tdcurses.hpp"
#include "td/utils/Promise.h"
#include "td/utils/Status.h"
#include "td/telegram/SynchronousRequests.h"

namespace tdcurses {

#define ErrorCodeWindowDeleted -999

class Tdcurses;

#define DROP_IF_DELETED(R)                                            \
  do {                                                                \
    if (R.is_error() && R.error().code() == ErrorCodeWindowDeleted) { \
      return;                                                         \
    }                                                                 \
  } while (0)

class TdcursesWindowBase {
 public:
  TdcursesWindowBase(Tdcurses *root, td::ActorId<Tdcurses> root_actor)
      : root_(root), root_actor_(std::move(root_actor)) {
    change_unique_id();
  }
  Tdcurses *root() {
    return root_;
  }
  td::ActorId<Tdcurses> root_actor_id() {
    return root_actor_;
  }

  template <class T>
  td::Promise<T> safe_promise(td::Promise<T> promise) {
    return td::PromiseCreator::lambda(
        [promise = std::move(promise), id = unique_id_, self_ptr = root_](td::Result<T> R) mutable {
          if (!self_ptr->window_exists(id)) {
            promise.set_error(td::Status::Error(ErrorCodeWindowDeleted, "window already deleted"));
            return;
          }
          promise.set_result(std::move(R));
        });
  }

  template <class T>
  void send_request(td::tl_object_ptr<T> func, td::Promise<typename T::ReturnType> P) {
    using RetType = typename T::ReturnType;
    using RetTlType = typename RetType::element_type;
    auto Q = td::PromiseCreator::lambda([id = unique_id_, self = root_actor_, self_ptr = root_, P = std::move(P)](
                                            td::Result<td::tl_object_ptr<td::td_api::Object>> R) mutable {
      td::send_lambda(self, [id, self_ptr, R = std::move(R), P = std::move(P)]() mutable {
        if (!self_ptr->window_exists(id)) {
          P.set_error(td::Status::Error(ErrorCodeWindowDeleted, "window already deleted"));
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
  template <class T>
  typename T::ReturnType run_request_sync(td::tl_object_ptr<T> func) {
    using RetType = typename T::ReturnType;
    using RetTlType = typename RetType::element_type;
    CHECK(td::SynchronousRequests::is_synchronous_request(func.get()));
    auto res = td::SynchronousRequests::run_request(std::move(func));
    return td::move_tl_object_as<RetTlType>(std::move(res));
  }

  td::int64 window_unique_id() const {
    return unique_id_;
  }

  void change_unique_id() {
    if (unique_id_ != 0) {
      root_->unregister_alive_window(this);
    }
    unique_id_ = ++last_unique_id;
    root_->register_alive_window(this);
  }

  ~TdcursesWindowBase() {
    if (unique_id_ != 0) {
      root_->unregister_alive_window(this);
    }
  }

 private:
  Tdcurses *root_;
  td::ActorId<Tdcurses> root_actor_;
  td::int64 unique_id_{0};
  static std::atomic<td::int64> last_unique_id;
};

}  // namespace tdcurses
