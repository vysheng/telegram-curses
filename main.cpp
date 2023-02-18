#include "td/actor/PromiseFuture.h"
#include "td/actor/actor.h"
#include "td/actor/ConcurrentScheduler.h"
#include "td/actor/impl/ActorId-decl.h"
#include "td/actor/impl/Scheduler-decl.h"
#include "td/utils/Status.h"
#include "td/utils/common.h"
#include "td/utils/filesystem.h"
#include "windows/screen.h"
#include "windows/padwindow.h"
#include "windows/editorwindow.h"
#include "windows/window-layout-split.h"
#include "td/actor/impl/Actor-decl.h"
#include "td/utils/Time.h"
#include "td/utils/logging.h"
#include "td/utils/port/PollFlags.h"
#include "td/utils/port/StdStreams.h"
#include "td/utils/FileLog.h"
#include "windows/window.h"
#include "print.h"
#include <limits>
#include <memory>
#include <vector>
#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#include <unistd.h>
#include "td/utils/port/signals.h"

void termination_signal_handler(int signum) {
  td::signal_safe_write_signal_number(signum);

#if HAVE_EXECINFO_H
  void *buffer[64];
  int nptrs = backtrace(buffer, 64);
  (void)write(2, "\n------- Stack Backtrace -------\n", 33);
  backtrace_symbols_fd(buffer, nptrs, 2);
  (void)write(2, "-------------------------------\n", 32);
#endif

  _exit(EXIT_FAILURE);
}

class NcursesLogInterface : public td::LogInterface {
 public:
  void do_append(int log_level, td::CSlice slice) override {
  }
};

class NcursesRoot : public td::Actor {
 public:
  NcursesRoot() {
  }
  void start_up() override {
    self_ = actor_id(this);
    class Cb : public windows::Screen::Callback {
     public:
      Cb(td::ActorId<NcursesRoot> self) : self_(self) {
      }

      void on_close() override {
        td::send_closure(self_, &NcursesRoot::on_close);
      }

     private:
      td::ActorId<NcursesRoot> self_;
    };
    auto cb = std::make_unique<Cb>(actor_id(this));
    screen_ = std::make_unique<windows::Screen>(std::move(cb));
    screen_->init();
    td::Scheduler::subscribe(td::Stdin().get_poll_info().extract_pollable_fd(this), td::PollFlags::Read());
    loop();
    screen_->refresh(true);
  }
  void on_close() {
    exit(0);
  }
  void loop() override {
    td::sync_with_poll(td::Stdin());
    screen_->loop();
  }
  void tear_down() override {
    td::Scheduler::unsubscribe(td::Stdin().get_poll_info().get_pollable_fd_ref());
  }

  void notify() override {
    // NB: Interface will be changed
    td::send_closure_later(self_, &NcursesRoot::on_net);
  }
  void on_net() {
    loop();
  }

  class E : public windows::PadWindowElement {
   public:
    E(std::string s, td::int32 x) : x_(x) {
      data_ = std::move(s);
    }
    E(td::int32 x) : x_(x) {
      int len = rand() % 10 + 10;
      std::string a;
      for (int i = 0; i < len; i++) {
        a = a + (char)('a' + rand() % 26);
      }
      data_ = a;
    }
    td::int32 render(windows::PadWindow &root, TickitRenderBuffer *rb, bool is_selected) override {
      return render_plain_text(rb, data_, width(), 1000000, is_selected);
    }

    bool is_less(const windows::PadWindowElement &el) const override {
      return x_ < static_cast<const E &>(el).x_;
    }

   private:
    std::string data_;
    td::int32 x_;
  };

  void pad_add_el(std::string el) {
    static int x = 0;
    pad_window_->add_element(std::make_shared<E>(el, ++x));
  }

  void create_popup(std::string popup_type) {
    if (popup_type == "view") {
      tdcurses::Outputter output;
      output << tdcurses::Outputter::Bold(true) << "bold" << tdcurses::Outputter::Bold(false) << "\n"
             << tdcurses::Outputter::Italic(true) << "italic" << tdcurses::Outputter::Italic(false) << "\n"
             << tdcurses::Outputter::Underline(true) << "underline" << tdcurses::Outputter::Underline(false) << "\n"
             << tdcurses::Outputter::Blink(true) << "blink" << tdcurses::Outputter::Blink(false) << "\n"
             << tdcurses::Outputter::Strike(true) << "strike" << tdcurses::Outputter::Strike(false) << "\n"
             << tdcurses::Outputter::Reverse(true) << "reverse" << tdcurses::Outputter::Reverse(false) << "\n"
             << tdcurses::Outputter::FgColor(tdcurses::Color::Blue) << "blue"
             << tdcurses::Outputter::FgColor(tdcurses::Color::Red) << "red"
             << tdcurses::Outputter::FgColor(tdcurses::Color::Revert) << "blue"
             << tdcurses::Outputter::FgColor(tdcurses::Color::Revert) << "\n";
      class Cb : public windows::ViewWindow::Callback {
       public:
        Cb(windows::Screen *ptr) : ptr_(ptr) {
        }
        void on_answer(windows::ViewWindow *self) override {
          ptr_->del_popup_window(self_ ? self_ : self);
        }
        void on_abort(windows::ViewWindow *self) override {
          on_answer(self);
        }

        void set_self(windows::Window *ptr) {
          self_ = ptr;
        }

       private:
        windows::Screen *ptr_;
        windows::Window *self_;
      };
      auto cb = std::make_unique<Cb>(screen_.get());
      auto w = std::make_shared<windows::ViewWindow>(output.as_str(), output.markup(), nullptr);
      auto box = std::make_shared<windows::BorderedWindow>(w, windows::BorderedWindow::BorderType::Double);
      cb->set_self(box.get());
      w->set_callback(std::move(cb));
      screen_->add_popup_window(box, 0);
    } else if (popup_type == "input") {
      class Cb : public windows::OneLineInputWindow::Callback {
       public:
        Cb(windows::Screen *ptr, NcursesRoot *main) : ptr_(ptr), main_(main) {
        }
        void on_answer(windows::OneLineInputWindow *self, std::string text) override {
          main_->pad_add_el(text);
          ptr_->del_popup_window(self_ ? self_ : self);
        }
        void on_abort(windows::OneLineInputWindow *self, std::string text) override {
          on_answer(self, text + " (aborted)");
        }

        void set_self(windows::Window *ptr) {
          self_ = ptr;
        }

       private:
        windows::Screen *ptr_;
        windows::Window *self_;
        NcursesRoot *main_;
      };
      auto cb = std::make_unique<Cb>(screen_.get(), this);
      auto w = std::make_shared<windows::OneLineInputWindow>("input", false, nullptr);
      auto box = std::make_shared<windows::BorderedWindow>(w, windows::BorderedWindow::BorderType::Double);
      cb->set_self(box.get());
      w->set_callback(std::move(cb));
      screen_->add_popup_window(box, 0);
    } else if (popup_type == "password") {
      class Cb : public windows::OneLineInputWindow::Callback {
       public:
        Cb(windows::Screen *ptr, NcursesRoot *main) : ptr_(ptr), main_(main) {
        }
        void on_answer(windows::OneLineInputWindow *self, std::string text) override {
          main_->pad_add_el(text);
          ptr_->del_popup_window(self_ ? self_ : self);
        }
        void on_abort(windows::OneLineInputWindow *self, std::string text) override {
          on_answer(self, text + " (aborted)");
        }

        void set_self(windows::Window *ptr) {
          self_ = ptr;
        }

       private:
        windows::Screen *ptr_;
        windows::Window *self_;
        NcursesRoot *main_;
      };
      auto cb = std::make_unique<Cb>(screen_.get(), this);
      auto w = std::make_shared<windows::OneLineInputWindow>("inputpass", true, nullptr);
      auto box = std::make_shared<windows::BorderedWindow>(w, windows::BorderedWindow::BorderType::Double);
      cb->set_self(box.get());
      w->set_callback(std::move(cb));
      screen_->add_popup_window(box, 0);
    }
  }

  void start() {
    screen_->init();
    //set_timeout_in(1.0);

    layout_ = std::make_shared<windows::WindowLayoutSplit>();
    screen_->change_layout(layout_);

    pad_window_ = std::make_shared<windows::PadWindow>();
    editor_window_ = std::make_shared<windows::EditorWindow>("", nullptr);
    auto data = td::read_file_str("/usr/include/tickit.h").move_as_ok();
    view_window_ = std::make_shared<windows::ViewWindow>(data, nullptr);

    class Cb : public windows::EditorWindow::Callback {
     public:
      Cb(NcursesRoot *self) : self_(self) {
      }
      void on_answer(windows::EditorWindow *window, std::string data) override {
        if (data.substr(0, 6) == "popup ") {
          self_->create_popup(data.substr(6));
        } else {
          self_->pad_add_el(data);
        }
        window->clear();
      }
      void on_abort(windows::EditorWindow *window, std::string data) override {
        window->clear();
      }

     private:
      NcursesRoot *self_;
    };

    log_window_ = std::make_shared<windows::EditorWindow>("", std::make_unique<Cb>(this));

    layout_->set_window(0, pad_window_);
    layout_->set_window(1, editor_window_);
    layout_->set_window(2, view_window_);
    layout_->set_window(3, log_window_);
  }

 private:
  td::ActorId<NcursesRoot> self_;
  std::shared_ptr<windows::WindowLayoutSplit> layout_;
  std::shared_ptr<windows::PadWindow> pad_window_;
  std::shared_ptr<windows::EditorWindow> editor_window_;
  std::shared_ptr<windows::ViewWindow> view_window_;
  std::shared_ptr<windows::EditorWindow> log_window_;

  std::unique_ptr<windows::Screen> screen_;
};

int main() {
  auto R = td::FileLog::create("log.txt");
  auto f = R.move_as_ok();
  td::log_interface = f.get();

  td::setup_signals_alt_stack().ensure();
  td::set_signal_handler(td::SignalType::Abort, termination_signal_handler).ensure();
  td::set_signal_handler(td::SignalType::Error, termination_signal_handler).ensure();
  td::ignore_signal(td::SignalType::Pipe).ensure();

  td::ConcurrentScheduler scheduler(4);
  auto act = scheduler.create_actor_unsafe<NcursesRoot>(1, "root");
  scheduler.start();
  {
    auto guard = scheduler.get_main_guard();
    td::send_closure_later(act.get(), &NcursesRoot::start);
  }
  while (scheduler.run_main(100)) {
  }
  scheduler.finish();

  return 0;
}
