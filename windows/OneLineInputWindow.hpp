#pragma once

#include "Window.hpp"
#include "TextEdit.hpp"

#include <string>

namespace windows {

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

  void handle_input(const InputEvent &info) override;
  void render(WindowOutputter &rb, bool force) override;

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

  void replace_text(std::string text) {
    edit_.replace_text(std::move(text));
    set_need_refresh();
  }

 private:
  std::string prompt_;
  TextEdit edit_;
  bool is_password_;
  std::unique_ptr<Callback> callback_;
};

}  // namespace windows
