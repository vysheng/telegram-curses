#pragma once

#include "Window.hpp"
#include "Markup.hpp"
#include <memory>

namespace windows {

class ViewWindow : public Window {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_answer(ViewWindow *window) = 0;
    virtual void on_abort(ViewWindow *window) = 0;
  };
  struct no_progress {};
  ViewWindow(std::string text, std::unique_ptr<Callback> callback)
      : text_(std::move(text)), callback_(std::move(callback)) {
    alloc_body();
  }
  ViewWindow(std::string text, std::vector<MarkupElement> markup, std::unique_ptr<Callback> callback)
      : text_(std::move(text)), markup_(std::move(markup)), callback_(std::move(callback)) {
    alloc_body();
  }
  ViewWindow(no_progress, std::string text, std::unique_ptr<Callback> callback)
      : text_(std::move(text)), callback_(std::move(callback)) {
  }
  ViewWindow(no_progress, std::string text, std::vector<MarkupElement> markup, std::unique_ptr<Callback> callback)
      : text_(std::move(text)), markup_(std::move(markup)), callback_(std::move(callback)) {
  }

  void set_callback(std::unique_ptr<Callback> cb) {
    callback_ = std::move(cb);
  }

  void handle_input(const InputEvent &info) override;
  void render(WindowOutputter &rb, bool force) override;
  void render_body(WindowOutputter &rb, bool force);

  void on_resize(td::int32 old_height, td::int32 old_width, td::int32 new_height, td::int32 new_width) override;

  td::int32 min_width() override {
    return 5;
  }
  td::int32 min_height() override {
    return body_ ? 3 : 1;
  }
  td::int32 best_width() override {
    return 10000;
  }
  td::int32 best_height() override {
    if (body_) {
      return cached_height_ <= 0 ? 3 : cached_height_ + 2;
    } else {
      return cached_height_ <= 0 ? 1 : cached_height_;
    }
  }

  void replace_text(std::string text, std::vector<MarkupElement> markup = {}) {
    text_ = std::move(text);
    markup_ = std::move(markup);
    set_need_refresh();
  }

  auto effective_height() const {
    if (body_) {
      return height() - 2;
    } else {
      return height();
    }
  }

  td::Slice title() const {
    return title_;
  }

  void set_need_refresh() override {
    Window::set_need_refresh();
    if (body_) {
      body_->set_need_refresh();
    }
  }
  void set_need_refresh_force() override {
    Window::set_need_refresh_force();
    if (body_) {
      body_->set_need_refresh_force();
    }
  }

 private:
  void alloc_body();
  class ViewWindowBody : public Window {
   public:
    ViewWindowBody(ViewWindow *win) : win_(win) {
    }
    void render(WindowOutputter &rb, bool force) override {
      win_->render_body(rb, force);
    }

   private:
    ViewWindow *win_;
  };
  std::string text_;
  std::vector<MarkupElement> markup_;
  td::int32 offset_from_top_{0};
  std::unique_ptr<Callback> callback_;
  SavedRenderedImages saved_images_;
  td::int32 cached_height_{0};
  std::string title_;
  std::shared_ptr<ViewWindowBody> body_;
};

}  // namespace windows
