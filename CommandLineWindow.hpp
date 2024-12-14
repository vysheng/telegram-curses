#pragma once

#include "windows/OneLineInputWindow.hpp"
#include <memory>

namespace tdcurses {

class CommandLineWindow : public windows::OneLineInputWindow {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_command(std::string command) = 0;
  };

  bool force_active() override {
    return !is_empty();
  }

 private:
  class Cb : public windows::OneLineInputWindow::Callback {
   public:
    Cb(std::unique_ptr<CommandLineWindow::Callback> cb) : cb_(std::move(cb)) {
    }
    void on_answer(OneLineInputWindow *self, std::string text) override {
      cb_->on_command(std::move(text));
      self->clear();
    }
    void on_abort(OneLineInputWindow *self, std::string text) override {
      self->clear();
    }

   private:
    std::unique_ptr<CommandLineWindow::Callback> cb_;
  };

 public:
  CommandLineWindow(std::unique_ptr<CommandLineWindow::Callback> cb)
      : windows::OneLineInputWindow("command: ", false, std::make_unique<Cb>(std::move(cb))) {
  }
};

}  // namespace tdcurses
