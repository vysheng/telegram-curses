#pragma once

#include "windows/editorwindow.h"
#include "windows/padwindow.h"
#include "windows/window.h"
#include <memory>

namespace tdcurses {

class StatusLineWindow : public windows::ViewWindow {
 private:
  class Cb : public windows::ViewWindow::Callback {
   public:
    Cb() {
    }
    void on_answer(ViewWindow *window) override {
    }
    void on_abort(ViewWindow *window) override {
    }
  };

 public:
  StatusLineWindow() : windows::ViewWindow("", std::make_unique<Cb>()) {
  }
};

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
