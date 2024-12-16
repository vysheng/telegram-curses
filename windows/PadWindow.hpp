#pragma once

#include "td/utils/Slice-decl.h"
#include "Window.hpp"
#include "TextEdit.hpp"
#include "td/utils/logging.h"
#include <memory>
#include <vector>
#include <functional>
#include <map>

namespace windows {

class PadWindow;

class PadWindowElement {
 public:
  virtual ~PadWindowElement() = default;
  virtual td::int32 render(PadWindow &root, WindowOutputter &rb, SavedRenderedImagesDirectory &dir,
                           bool is_selected) = 0;
  virtual td::int32 render_fake(PadWindow &root, WindowOutputter &rb, bool is_selected) {
    CHECK(!rb.is_real());
    SavedRenderedImagesDirectory dir{{}};
    auto r = render(root, rb, dir, is_selected);
    CHECK(!dir.new_images.size());
    return r;
  }
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

  virtual void handle_input(PadWindow &root, const InputEvent &info) {
  }

  /*virtual td::uint32 empty_element_effects() {
    return 0;
  }*/

  td::int32 render_plain_text(WindowOutputter &rb, td::Slice text, td::int32 width, td::int32 max_height,
                              bool is_selected, SavedRenderedImagesDirectory *images);
  td::int32 render_plain_text(WindowOutputter &rb, td::Slice text, std::vector<MarkupElement> markup, td::int32 width,
                              td::int32 max_height, bool is_selected, SavedRenderedImagesDirectory *images);

 private:
  td::int32 width_;
};

class PadWindow : public Window {
 public:
  PadWindow();
  struct ElementInfo {
    ElementInfo(std::shared_ptr<PadWindowElement> element) : element(std::move(element)) {
    }
    std::shared_ptr<PadWindowElement> element;
    td::int32 height;

    void update();
  };
  enum class GluedTo { Top, RelTop, RelBottom, Bottom, None };
  enum class PadTo { Top, Bottom };
  void on_resize(td::int32 old_width, td::int32 old_height, td::int32 new_width, td::int32 new_height) override;

  virtual void request_top_elements() {
  }
  virtual void request_bottom_elements() {
  }

  void render(WindowOutputter &rb, bool force) override;
  void render_body(WindowOutputter &rb, bool force);

  ElementInfo *get_element(td::int32 height);
  void change_selection();

  void handle_input(const InputEvent &info) override;
  void active_element_handle_input(const InputEvent &info);

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

  enum class ScrollMode { FirstLine, LastLine, Minimal };
  void scroll_to_element(PadWindowElement *el, ScrollMode scroll_mode);

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

  std::vector<std::shared_ptr<PadWindowElement>> get_visible_elements();

  void set_pad_to(PadTo pad_to) {
    pad_to_ = pad_to;
  }

  void clear() {
    elements_.clear();
    cur_element_ = nullptr;
    if (pad_to_ == PadTo::Top) {
      glued_to_ = GluedTo::Top;
    } else {
      glued_to_ = GluedTo::Bottom;
    }
    offset_in_cur_element_ = 0;
    offset_from_window_top_ = 0;
    lines_after_cur_element_ = 0;
    lines_before_cur_element_ = 0;
  }

  void unglue() {
    glued_to_ = GluedTo::None;
  }

  const PadWindowElement *first_element() const {
    if (!elements_.size()) {
      return nullptr;
    }
    return elements_.begin()->first;
  }
  const PadWindowElement *last_element() const {
    if (!elements_.size()) {
      return nullptr;
    }
    return elements_.rbegin()->first;
  }

  auto effective_height() const {
    return height() - 2;
  }

  void set_title(std::string title) {
    title_ = std::move(title);
  }

  td::CSlice title() const {
    return title_;
  }

 private:
  class PadWindowBody : public Window {
   public:
    PadWindowBody(PadWindow *win) : win_(win) {
    }
    void render(WindowOutputter &rb, bool force) override {
      win_->render_body(rb, force);
    }

   private:
    PadWindow *win_;
  };
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

  GluedTo glued_to_{GluedTo::Top};
  PadTo pad_to_{PadTo::Top};

  std::string title_;
  std::shared_ptr<Window> pad_window_body_;

  SavedRenderedImages saved_images_;
};

}  // namespace windows
