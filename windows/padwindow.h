#pragma once

#include "td/utils/Slice-decl.h"
#include "window.h"
#include "text.h"
#include "td/utils/logging.h"
#include <vector>
#include <functional>
#include <map>

namespace windows {

class PadWindowElement {
 public:
  virtual ~PadWindowElement() = default;
  virtual td::int32 render(TickitRenderBuffer *rb, bool is_selected) = 0;
  void change_width(td::int32 width) {
    width_ = width;
  }

  auto width() const {
    return width_;
  }

  virtual bool is_less(const PadWindowElement &other) const = 0;
  virtual bool is_visible() const {
    return true;
  }

  /*virtual td::uint32 empty_element_effects() {
    return 0;
  }*/

  static td::int32 render_plain_text(TickitRenderBuffer *rb, td::Slice text, td::int32 width, td::int32 max_height,
                                     bool is_selected);
  static td::int32 render_plain_text(TickitRenderBuffer *rb, td::Slice text, std::vector<MarkupElement> markup,
                                     td::int32 width, td::int32 max_height, bool is_selected);

 private:
  td::int32 width_;
};

class PadWindow : public Window {
 public:
  PadWindow() {
  }
  struct ElementInfo {
    ElementInfo(std::shared_ptr<PadWindowElement> element) : element(std::move(element)) {
    }
    std::shared_ptr<PadWindowElement> element;
    td::int32 height;

    void update();
  };
  enum class GluedTo { Top, Bottom, None };
  enum class PadTo { Top, Bottom };
  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override;

  virtual void request_top_elements() {
  }
  virtual void request_bottom_elements() {
  }

  void render(TickitRenderBuffer *rb, td::int32 &cursor_x, td::int32 &cursor_y, TickitCursorShape &cursor_shape,
              bool force) override;

  ElementInfo *get_element(td::int32 height);
  void change_selection();

  void handle_input(TickitKeyEventInfo *info) override;

  virtual void pad_handle_input(TickitKeyEventInfo *info) {
  }

  td::int32 pad_height() const {
    return lines_before_cur_element_ + lines_after_cur_element_ + (cur_element_ ? cur_element_->height : 0);
  }

  void change_element(PadWindowElement *el);
  void change_element(std::shared_ptr<PadWindowElement> el, std::function<void()> change);
  void delete_element(PadWindowElement *el);
  void add_element(std::shared_ptr<PadWindowElement> element);

  void scroll_up(td::int32 lines);
  void scroll_down(td::int32 lines);
  void scroll_first_line();
  void scroll_last_line();
  virtual void scroll_next_element();
  virtual void scroll_prev_element();

  void adjust_cur_element(td::int32 lines);

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

  std::shared_ptr<PadWindowElement> get_active_element() {
    return cur_element_ ? cur_element_->element : nullptr;
  }

  void set_pad_to(PadTo pad_to) {
    pad_to_ = pad_to;
  }

 private:
  class Compare {
   public:
    bool operator()(const PadWindowElement *l, const PadWindowElement *r) const {
      return l->is_less(*r);
    }
  };
  std::map<PadWindowElement *, std::unique_ptr<ElementInfo>, Compare> elements_;

  td::int32 lines_before_cur_element_{0};
  td::int32 lines_after_cur_element_{0};
  td::int32 offset_in_cur_element_{0};
  td::int32 offset_from_window_top_{0};
  ElementInfo *cur_element_{nullptr};

  GluedTo glued_to_{GluedTo::None};
  PadTo pad_to_{PadTo::Top};
};

}  // namespace windows
