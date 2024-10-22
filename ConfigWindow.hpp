#pragma once

#include "MenuWindowCommon.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace tdcurses {

class ConfigWindow : public MenuWindowCommon {
 public:
  ConfigWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::vector<Tdcurses::Option> &options)
      : MenuWindowCommon(root, std::move(root_actor)), options_(options) {
    for (auto &opt : options_) {
      add_element(opt.name, opt.values[opt.cur_selected], {}, [&opt](MenuWindowElementRun &e) {
        opt.update();
        e.data = opt.values[opt.cur_selected];
        return false;
      });
    }
    set_title("config");
  }

  void handle_input(TickitKeyEventInfo *info) override {
    return MenuWindowCommon::handle_input(info);
  }

  td::int32 best_height() override {
    return (td::int32)options_.size() + 2;
  }

 private:
  std::vector<Tdcurses::Option> &options_;
};

}  // namespace tdcurses
