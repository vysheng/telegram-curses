#pragma once

#include "PadWindow.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace windows {

class SelectionWindow : public PadWindow {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_end() = 0;
    virtual void on_abort() = 0;
  };

  struct ElementInfo {
    ElementInfo() = default;
    ElementInfo(std::string text, std::vector<MarkupElement> markup, std::function<void()> cb)
        : text(std::move(text)), markup(std::move(markup)), cb(std::move(cb)) {
    }
    std::string text;
    std::vector<MarkupElement> markup;
    std::function<void()> cb;
  };

  SelectionWindow(std::vector<ElementInfo> elements, std::unique_ptr<Callback> callback)
      : callback_(std::move(callback)) {
    for (size_t i = 0; i < elements.size(); i++) {
      auto &from = elements[i];
      auto el = std::make_shared<Element>(std::move(from.text), std::move(from.markup), i, std::move(from.cb));
      add_element(std::move(el));
    }
    set_need_refresh();
  }
  void handle_input(const InputEvent &info) {
    if (info == "T-Enter") {
      auto a = get_active_element();
      if (a) {
        auto el = static_cast<Element *>(a.get());
        el->run_callback();
        callback_->on_end();
      }
      return;
    } else if (info == "T-Escape") {
      callback_->on_abort();
      return;
    }
    return PadWindow::handle_input(info);
  }

  void set_callback(std::unique_ptr<Callback> callback) {
    callback_ = std::move(callback);
  }

 private:
  class Element : public PadWindowElement {
   public:
    Element(std::string text, std::vector<MarkupElement> markup, size_t idx, std::function<void()> callback)
        : text_(std::move(text)), markup_(std::move(markup)), idx_(idx), callback_(std::move(callback)) {
    }
    td::int32 render(PadWindow &root, WindowOutputter &rb, SavedRenderedImagesDirectory &dir,
                     bool is_selected) override {
      return render_plain_text(rb, text_, markup_, width(), 1, is_selected, &dir);
    }

    bool is_less(const PadWindowElement &other) const override {
      const auto &l = static_cast<const Element &>(other);
      return idx_ < l.idx_;
    }

    void run_callback() {
      callback_();
    }

   private:
    std::string text_;
    std::vector<MarkupElement> markup_;
    size_t idx_;
    std::function<void()> callback_;
  };

  std::unique_ptr<Callback> callback_;
};

}  // namespace windows
