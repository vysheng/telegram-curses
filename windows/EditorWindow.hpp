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

  void handle_input(const InputEvent &info) override;
  void render(WindowOutputter &rb, bool force) override;

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

}  // namespace windows
