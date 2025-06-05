#pragma once
#include "MenuWindowPad.hpp"
#include "windows/Markup.hpp"
#include "windows/Output.hpp"
#include "windows/unicode.h"
#include <functional>
#include <memory>
#include <utility>

namespace tdcurses {

class MenuWindowCommon;

using MenuWindowSpawnFunction = std::function<std::shared_ptr<MenuWindow>(MenuWindow &parent)>;

template <typename T, typename... ArgsT>
MenuWindowSpawnFunction create_menu_window_spawn_function(ArgsT &&...args) {
  return [args = std::make_tuple(std::forward<ArgsT>(args)...)](MenuWindow &parent) mutable {
    return std::apply([&](auto &&...args) { return parent.spawn_submenu<T>(std::forward<ArgsT>(args)...); },
                      std::move(args));
  };
}

class MenuWindowElement {
 public:
  MenuWindowElement(std::string name, std::string data, std::vector<windows::MarkupElement> markup)
      : name(std::move(name)), data(std::move(data)), markup(std::move(markup)) {
  }
  virtual ~MenuWindowElement() = default;
  virtual void handle_input(MenuWindowCommon &root, const windows::InputEvent &info) {
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
  void handle_input(MenuWindowCommon &root, const windows::InputEvent &info) override;

 private:
  MenuWindowSpawnFunction cb_;
};

class MenuWindowElementRun : public MenuWindowElement {
 public:
  MenuWindowElementRun(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                       std::function<bool()> cb)
      : MenuWindowElement(std::move(name), std::move(data), std::move(markup))
      , cb_([cb = std::move(cb)](MenuWindowCommon &, MenuWindowElementRun &) { return cb(); }) {
  }
  MenuWindowElementRun(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                       std::function<bool(MenuWindowCommon &)> cb)
      : MenuWindowElement(std::move(name), std::move(data), std::move(markup))
      , cb_([cb = std::move(cb)](MenuWindowCommon &w, MenuWindowElementRun &) { return cb(w); }) {
  }
  MenuWindowElementRun(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                       std::function<bool(MenuWindowElementRun &)> cb)
      : MenuWindowElement(std::move(name), std::move(data), std::move(markup))
      , cb_([cb = std::move(cb)](MenuWindowCommon &, MenuWindowElementRun &r) { return cb(r); }) {
  }
  void handle_input(MenuWindowCommon &root, const windows::InputEvent &info) override;

  void replace_text(std::string data, std::vector<windows::MarkupElement> markup = {}) {
  }

 private:
  std::function<bool(MenuWindowCommon &, MenuWindowElementRun &)> cb_;
};

class MenuWindowCommon : public MenuWindowPad {
 public:
  MenuWindowCommon(Tdcurses *root, td::ActorId<Tdcurses> root_actor) : MenuWindowPad(root, root_actor) {
  }
  class Element : public windows::PadWindowElement {
   public:
    Element(size_t idx, std::shared_ptr<MenuWindowElement> element) : idx_(idx), element_(std::move(element)) {
    }

    void handle_input(windows::PadWindow &pad, const windows::InputEvent &info) override {
      element_->handle_input(static_cast<MenuWindowCommon &>(pad), info);
    }
    bool is_less(const PadWindowElement &other) const override {
      return idx_ < static_cast<const Element &>(other).idx_;
    }
    td::int32 render(PadWindow &root, windows::WindowOutputter &rb, windows::SavedRenderedImagesDirectory &dir,
                     bool is_selected) override {
      auto markup = element_->markup;
      auto text = element_->name;
      auto s = (td::uint32)utf8_string_width(text);
      if (s < static_cast<MenuWindowCommon &>(root).pad_size()) {
        auto l = static_cast<MenuWindowCommon &>(root).pad_size() - s;
        for (td::uint32 i = 0; i < l; i++) {
          text += " ";
        }
      }
      if (element_->data.size() != 0) {
        text += ": ";
      }
      auto size = text.size();
      text += element_->data;
      for (auto &e : markup) {
        e->move(size);
      }
      SCOPE_EXIT {
        for (auto &e : element_->markup) {
          e->move(-size);
        }
      };
      markup.push_back(std::make_shared<windows::MarkupElementBold>(windows::MarkupElementPos(0, 1),
                                                                    windows::MarkupElementPos(size, 1), true));
      markup.push_back(std::make_shared<windows::MarkupElementFgColor>(
          windows::MarkupElementPos(0, 1), windows::MarkupElementPos(size, 1), windows::Color::White));
      return render_plain_text(rb, text, std::move(markup), width(), std::numeric_limits<td::int32>::max(), is_selected,
                               &dir);
    }

    void update_element(std::shared_ptr<MenuWindowElement> element) {
      element_ = std::move(element);
    }

    auto &menu_element() {
      return element_;
    }

   private:
    size_t idx_;
    std::shared_ptr<MenuWindowElement> element_;
  };

  std::shared_ptr<Element> add_element(std::shared_ptr<MenuWindowElement> element) {
    auto w = (td::uint32)utf8_string_width(element->name);
    if (w > max_size_) {
      max_size_ = w;
    }
    auto el = std::make_shared<Element>(last_idx_++, std::move(element));
    PadWindow::add_element(el);
    return el;
  }
  std::shared_ptr<Element> add_element(std::string name, std::string data,
                                       std::vector<windows::MarkupElement> markup = {}) {
    return add_element(std::make_shared<MenuWindowElement>(std::move(name), std::move(data), std::move(markup)));
  }
  std::shared_ptr<Element> add_element(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                                       MenuWindowSpawnFunction spawn) {
    return add_element(std::make_shared<MenuWindowElementSpawn>(std::move(name), std::move(data), std::move(markup),
                                                                std::move(spawn)));
  }
  std::shared_ptr<Element> add_element(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                                       std::function<bool()> cb) {
    return add_element(
        std::make_shared<MenuWindowElementRun>(std::move(name), std::move(data), std::move(markup), std::move(cb)));
  }
  std::shared_ptr<Element> add_element(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                                       std::function<bool(MenuWindowCommon &)> cb) {
    return add_element(
        std::make_shared<MenuWindowElementRun>(std::move(name), std::move(data), std::move(markup), std::move(cb)));
  }
  std::shared_ptr<Element> add_element(std::string name, std::string data, std::vector<windows::MarkupElement> markup,
                                       std::function<bool(MenuWindowElementRun &)> cb) {
    return add_element(
        std::make_shared<MenuWindowElementRun>(std::move(name), std::move(data), std::move(markup), std::move(cb)));
  }

  size_t pad_size() const {
    return max_size_;
  }

 private:
  size_t last_idx_ = 0;
  size_t max_size_ = 0;
};

using ElInfo = MenuWindowCommon::Element;

}  // namespace tdcurses
