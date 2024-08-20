#pragma once
#include "MenuWindow.hpp"

namespace tdcurses {

class MenuWindowCommon;

class MenuWindowElement {
 public:
  MenuWindowElement(std::string name, std::string data, std::vector<windows::MarkupElement> markup)
      : name(std::move(name)), data(std::move(data)), markup(std::move(markup)) {
  }
  virtual ~MenuWindowElement() = default;
  virtual void handle_input(MenuWindowCommon &root, TickitKeyEventInfo *info) {
  }
  std::string name;
  std::string data;
  std::vector<windows::MarkupElement> markup;
};

class MenuWindowElementSpawn : public MenuWindowElement {
 public:
  MenuWindowElementSpawn(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                         MenuWindowSpawnFunction cb)
      : MenuWindowElement(std::move(name), std::move(data), std::move(markup)), cb_(std::move(cb)) {
  }
  void handle_input(MenuWindowCommon &root, TickitKeyEventInfo *info) override;

 private:
  MenuWindowSpawnFunction cb_;
};

class MenuWindowElementRun : public MenuWindowElement {
 public:
  MenuWindowElementRun(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                       std::function<void()> cb)
      : MenuWindowElement(std::move(name), std::move(data), std::move(markup)), cb_(std::move(cb)) {
  }
  void handle_input(MenuWindowCommon &root, TickitKeyEventInfo *info) override;

 private:
  std::function<void()> cb_;
};

class MenuWindowCommon : public MenuWindow {
 public:
  MenuWindowCommon(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindow(root, root_actor) {
  }
  class Element : public windows::PadWindowElement {
   private:
   public:
    Element(size_t idx, std::shared_ptr<MenuWindowElement> element) : idx_(idx), element_(std::move(element)) {
    }

    void handle_input(windows::PadWindow &pad, TickitKeyEventInfo *info) override {
      element_->handle_input(static_cast<MenuWindowCommon &>(pad), info);
    }
    bool is_less(const PadWindowElement &other) const override {
      return idx_ < static_cast<const Element &>(other).idx_;
    }
    td::int32 render(PadWindow &root, TickitRenderBuffer *rb, bool is_selected) override {
      auto markup = element_->markup;
      auto text = element_->name;
      while (text.size() < static_cast<MenuWindowCommon &>(root).pad_size()) {
        text += " ";
      }
      text += ": ";
      auto size = text.size();
      text += element_->data;
      for (auto &e : markup) {
        e.first_pos += size;
        e.last_pos += size;
      }
      markup.push_back(windows::MarkupElement::bold(0, size));
      markup.push_back(windows::MarkupElement::fg_color(0, size, (td::int32)Color::White));
      return Element::render_plain_text(rb, text, std::move(markup), width(), std::numeric_limits<td::int32>::max(),
                                        is_selected);
    }

   private:
    size_t idx_;
    std::shared_ptr<MenuWindowElement> element_;
  };

  void add_element(std::shared_ptr<MenuWindowElement> element) {
    if (element->name.size() > max_size_) {
      max_size_ = element->name.size();
    }
    PadWindow::add_element(std::make_shared<Element>(last_idx_++, std::move(element)));
  }
  void add_element(std::string name, std::string data, std::vector<windows::MarkupElement> markup = {}) {
    add_element(std::make_shared<MenuWindowElement>(std::move(name), std::move(data), std::move(markup)));
  }
  void add_element(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                   MenuWindowSpawnFunction spawn) {
    add_element(std::make_shared<MenuWindowElementSpawn>(std::move(name), std::move(data), std::move(markup),
                                                         std::move(spawn)));
  }
  void add_element(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                   std::function<void()> cb) {
    add_element(
        std::make_shared<MenuWindowElementRun>(std::move(name), std::move(data), std::move(markup), std::move(cb)));
  }

  size_t pad_size() const {
    return max_size_;
  }

 private:
  size_t last_idx_ = 0;
  size_t max_size_ = 0;
};

}  // namespace tdcurses
