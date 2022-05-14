#pragma once

#include "td/tdactor/td/actor/actor.h"
#include "td/generate/auto/td/telegram/td_api.h"
#include "td/actor/PromiseFuture.h"
#include "td/tl/TlObject.h"

#include <string>

namespace tdcurses {

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

}  // namespace tdcurses
