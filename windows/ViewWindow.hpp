#pragma once

#include "Window.hpp"
#include "Markup.hpp"

namespace windows {

class ViewWindow : public Window {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_answer(ViewWindow *window) = 0;
    virtual void on_abort(ViewWindow *window) = 0;
  };
  ViewWindow(std::string text, std::unique_ptr<Callback> callback)
      : text_(std::move(text)), callback_(std::move(callback)) {
  }
  ViewWindow(std::string text, std::vector<MarkupElement> markup, std::unique_ptr<Callback> callback)
      : text_(std::move(text)), markup_(std::move(markup)), callback_(std::move(callback)) {
  }

  void set_callback(std::unique_ptr<Callback> cb) {
    callback_ = std::move(cb);
  }

  void handle_input(const InputEvent &info) override;
  void render(WindowOutputter &rb, bool force) override;

  void on_resize(td::int32 old_height, td::int32 old_width, td::int32 new_height, td::int32 new_width) override {
    offset_from_top_ = 0;
  }

  td::int32 min_width() override {
    return 5;
  }
  td::int32 min_height() override {
    return 1;
  }
  td::int32 best_width() override {
    return 10000;
  }
  td::int32 best_height() override {
    return cached_height_ <= 0 ? 1 : cached_height_;
  }

  void replace_text(std::string text, std::vector<MarkupElement> markup = {}) {
    text_ = std::move(text);
    markup_ = std::move(markup);
    set_need_refresh();
  }

 private:
  std::string text_;
  std::vector<MarkupElement> markup_;
  td::int32 offset_from_top_{0};
  std::unique_ptr<Callback> callback_;
  SavedRenderedImages saved_images_;
  td::int32 cached_height_{0};
};

}  // namespace windows
