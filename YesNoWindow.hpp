#include "MenuWindow.hpp"
#include "td/utils/Promise.h"
#include "windows/EditorWindow.hpp"
#include "windows/Markup.hpp"
#include "windows/Window.hpp"
#include <memory>
#include <vector>

namespace tdcurses {

class YesNoWindow
    : public MenuWindow
    , public windows::Window {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_abort(YesNoWindow &) = 0;
    virtual void on_answer(YesNoWindow &, bool answer) = 0;
  };
  YesNoWindow(Tdcurses *root, td::ActorId<Tdcurses> root_actor, std::string text,
              std::vector<windows::MarkupElement> markup, std::unique_ptr<Callback> callback, bool default_value = true)
      : MenuWindow(root, std::move(root_actor)), callback_(std::move(callback)), ok_(default_value) {
    view_window_ = std::make_shared<windows::ViewWindow>(std::move(text), std::move(markup), nullptr);
  }
  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
    view_window_->resize(new_width, new_height - 2);
  }
  std::shared_ptr<windows::Window> get_window(std::shared_ptr<MenuWindow> self) override {
    return std::static_pointer_cast<YesNoWindow>(std::move(self));
  }

  td::int32 min_width() override {
    return 12;
  }
  td::int32 min_height() override {
    return 3;
  }
  td::int32 best_width() override {
    return 40;
  }
  td::int32 best_height() override {
    return 6;
  }

  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override {
    if (!rb) {
      return;
    }

    view_window_->render(rb, cursor_x, cursor_y, cursor_shape, force);

    CHECK(width() >= 10);
    auto pad = (width() - 10) / 2;

    std::string text = "\n";
    std::vector<windows::MarkupElement> markup;
    for (td::int32 i = 0; i < pad; i++) {
      text += ' ';
    }
    auto s = (td::int32)text.size();
    if (ok_) {
      text += "<YES>  NO ";
      markup.emplace_back(windows::MarkupElement::reverse(s, s + 5));
    } else {
      text += " YES  <NO>";
      markup.emplace_back(windows::MarkupElement::reverse(s + 6, s + 10));
    }

    if (rb) {
      tickit_renderbuffer_save(rb);
      tickit_renderbuffer_translate(rb, height() - 2, 0);
    }
    windows::TextEdit::render(rb, cursor_x, cursor_y, cursor_shape, width(), text, 0, markup, false, false, 0, "");
    if (rb) {
      tickit_renderbuffer_restore(rb);
    }

    cursor_y = 0;
    cursor_x = 0;
    cursor_shape = (TickitCursorShape)0;
  }

  void handle_input(TickitKeyEventInfo *info) override {
    set_need_refresh();
    if (info->type == TICKIT_KEYEV_KEY) {
      if (!strcmp(info->str, "Enter")) {
        if (!sent_answer_) {
          callback_->on_answer(*this, ok_);
        }
        sent_answer_ = true;
        return;
      } else if (!strcmp(info->str, "Right")) {
        ok_ = false;
        return;
      } else if (!strcmp(info->str, "Left")) {
        ok_ = true;
        return;
      }
    } else {
      if (!strcmp(info->str, "h")) {
        ok_ = true;
        return;
      } else if (!strcmp(info->str, "l")) {
        ok_ = false;
        return;
      }
    }
    if (menu_window_handle_input(info)) {
      return;
    }
    view_window_->handle_input(info);
  }

  ~YesNoWindow() {
    if (!sent_answer_) {
      callback_->on_abort(*this);
    }
    sent_answer_ = true;
  }

 private:
  std::unique_ptr<Callback> callback_;
  bool ok_ = false;
  bool sent_answer_ = false;

  std::shared_ptr<windows::ViewWindow> view_window_;
};

}  // namespace tdcurses
