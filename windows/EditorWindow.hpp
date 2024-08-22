#pragma once
#include "td/actor/PromiseFuture.h"
#include "td/utils/Status.h"
#include "td/utils/common.h"
#include "Window.hpp"
#include "TextEdit.hpp"
#include <memory>
#include <string>
#include <vector>

namespace windows {

class EditorWindow : public Window {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_answer(EditorWindow *window, std::string text) = 0;
    virtual void on_abort(EditorWindow *window, std::string text) = 0;
  };
  EditorWindow(std::string text, std::unique_ptr<Callback> callback) : edit_(text), callback_(std::move(callback)) {
  }
  std::string export_data() {
    return edit_.export_data();
  }

  void handle_input(TickitKeyEventInfo *info) override;
  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override;

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
    return 10000;
  }

  void clear() {
    edit_.clear();
  }

  void install_callback(std::unique_ptr<Callback> callback) {
    callback_ = std::move(callback);
  }

 private:
  TextEdit edit_;
  td::int32 offset_from_top_{0};
  td::int32 last_cursor_y_{0};
  std::unique_ptr<Callback> callback_;
};

class OneLineInputWindow : public Window {
 public:
  class Callback {
   public:
    virtual ~Callback() = default;
    virtual void on_answer(OneLineInputWindow *self, std::string text) = 0;
    virtual void on_abort(OneLineInputWindow *self, std::string text) = 0;
  };
  OneLineInputWindow(std::string prompt, bool is_password, std::unique_ptr<Callback> callback)
      : prompt_(std::move(prompt)), is_password_(is_password), callback_(std::move(callback)) {
  }
  OneLineInputWindow(std::string prompt, std::string text, bool is_password, std::unique_ptr<Callback> callback)
      : prompt_(std::move(prompt)), is_password_(is_password), callback_(std::move(callback)) {
    edit_.replace_text(std::move(text));
  }
  std::string export_data() {
    return edit_.export_data();
  }

  void handle_input(TickitKeyEventInfo *info) override;
  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override;

  td::int32 min_width() override {
    return 1 + (td::int32)prompt_.size() + 10;
  }
  td::int32 min_height() override {
    return 1;
  }
  td::int32 best_width() override {
    return 1 + (td::int32)prompt_.size() + 10;
  }
  td::int32 best_height() override {
    return 3;
  }

  void set_callback(std::unique_ptr<Callback> cb) {
    callback_ = std::move(cb);
  }

  void clear() {
    edit_.clear();
  }

  bool is_empty() const {
    return edit_.is_empty();
  }

 private:
  std::string prompt_;
  TextEdit edit_;
  bool is_password_;
  std::unique_ptr<Callback> callback_;
};

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

  void handle_input(TickitKeyEventInfo *info) override;
  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override;

  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override {
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
    return 10000;
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
};

}  // namespace windows
