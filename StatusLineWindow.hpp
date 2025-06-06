#pragma once

#include "windows/ViewWindow.hpp"
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
  StatusLineWindow() : windows::ViewWindow(windows::ViewWindow::no_progress{}, "", std::make_unique<Cb>()) {
  }
};

}  // namespace tdcurses
