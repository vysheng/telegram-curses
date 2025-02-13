#pragma once

#include "td/utils/Variant.h"
#include "td/utils/common.h"
#include "Output.hpp"
#include <memory>

namespace windows {

class TextEditBuilder;

class MarkupElementBase {
 public:
  MarkupElementBase(size_t first_pos, size_t last_pos) : first_pos_(first_pos), last_pos_(last_pos) {
  }
  virtual ~MarkupElementBase() = default;
  virtual void install(WindowOutputter &rb) const = 0;
  virtual void uninstall(WindowOutputter &rb) const = 0;
  virtual bool apply_to_text_edit() const {
    return false;
  }

  auto first_pos() const {
    return first_pos_;
  }
  auto last_pos() const {
    return last_pos_;
  }

  void set_last_pos(size_t last_pos) {
    last_pos_ = last_pos;
  }

  void move(size_t offset) {
    first_pos_ += offset;
    last_pos_ += offset;
  }

 private:
  size_t first_pos_;
  size_t last_pos_;
};

class MarkupElementTextEdit : public MarkupElementBase {
 public:
  MarkupElementTextEdit(size_t first_pos, size_t last_pos) : MarkupElementBase(first_pos, last_pos) {
  }
  void install(WindowOutputter &rb) const override {
    UNREACHABLE();
  }
  void uninstall(WindowOutputter &rb) const override {
    UNREACHABLE();
  }
  bool apply_to_text_edit() const override {
    return true;
  }
  virtual void install(TextEditBuilder &rb) const = 0;
  virtual void uninstall(TextEditBuilder &rb) const = 0;
};

class MarkupElementFgColor : public MarkupElementBase {
 public:
  MarkupElementFgColor(size_t first_pos, size_t last_pos, Color color)
      : MarkupElementBase(first_pos, last_pos), color_(color) {
  }

  void install(WindowOutputter &rb) const override {
    rb.set_fg_color(color_);
  }
  void uninstall(WindowOutputter &rb) const override {
    rb.unset_fg_color();
  }

 private:
  Color color_;
};

class MarkupElementFgColorRGB : public MarkupElementBase {
 public:
  MarkupElementFgColorRGB(size_t first_pos, size_t last_pos, ColorRGB color)
      : MarkupElementBase(first_pos, last_pos), color_(color) {
  }

  void install(WindowOutputter &rb) const override {
    rb.set_fg_color_rgb(color_);
  }
  void uninstall(WindowOutputter &rb) const override {
    rb.unset_fg_color();
  }

 private:
  ColorRGB color_;
};

class MarkupElementBgColor : public MarkupElementBase {
 public:
  MarkupElementBgColor(size_t first_pos, size_t last_pos, Color color)
      : MarkupElementBase(first_pos, last_pos), color_(color) {
  }

  void install(WindowOutputter &rb) const override {
    rb.set_bg_color(color_);
  }
  void uninstall(WindowOutputter &rb) const override {
    rb.unset_bg_color();
  }

 private:
  Color color_;
};

class MarkupElementBgColorRGB : public MarkupElementBase {
 public:
  MarkupElementBgColorRGB(size_t first_pos, size_t last_pos, ColorRGB color)
      : MarkupElementBase(first_pos, last_pos), color_(color) {
  }

  void install(WindowOutputter &rb) const override {
    rb.set_bg_color_rgb(color_);
  }
  void uninstall(WindowOutputter &rb) const override {
    rb.unset_bg_color();
  }

 private:
  ColorRGB color_;
};

class MarkupElementBold : public MarkupElementBase {
 public:
  MarkupElementBold(size_t first_pos, size_t last_pos, bool value)
      : MarkupElementBase(first_pos, last_pos), value_(value) {
  }

  void install(WindowOutputter &rb) const override {
    rb.set_bold(value_);
  }
  void uninstall(WindowOutputter &rb) const override {
    rb.unset_bold();
  }

 private:
  bool value_;
};

class MarkupElementUnderline : public MarkupElementBase {
 public:
  MarkupElementUnderline(size_t first_pos, size_t last_pos, bool value)
      : MarkupElementBase(first_pos, last_pos), value_(value) {
  }

  void install(WindowOutputter &rb) const override {
    rb.set_underline(value_);
  }
  void uninstall(WindowOutputter &rb) const override {
    rb.unset_underline();
  }

 private:
  bool value_;
};

class MarkupElementItalic : public MarkupElementBase {
 public:
  MarkupElementItalic(size_t first_pos, size_t last_pos, bool value)
      : MarkupElementBase(first_pos, last_pos), value_(value) {
  }

  void install(WindowOutputter &rb) const override {
    rb.set_italic(value_);
  }
  void uninstall(WindowOutputter &rb) const override {
    rb.unset_italic();
  }

 private:
  bool value_;
};

class MarkupElementReverse : public MarkupElementBase {
 public:
  MarkupElementReverse(size_t first_pos, size_t last_pos, bool value)
      : MarkupElementBase(first_pos, last_pos), value_(value) {
  }

  void install(WindowOutputter &rb) const override {
    rb.set_reverse(value_);
  }
  void uninstall(WindowOutputter &rb) const override {
    rb.unset_reverse();
  }

 private:
  bool value_;
};

class MarkupElementStrike : public MarkupElementBase {
 public:
  MarkupElementStrike(size_t first_pos, size_t last_pos, bool value)
      : MarkupElementBase(first_pos, last_pos), value_(value) {
  }

  void install(WindowOutputter &rb) const override {
    rb.set_strike(value_);
  }
  void uninstall(WindowOutputter &rb) const override {
    rb.unset_strike();
  }

 private:
  bool value_;
};

class MarkupElementBlink : public MarkupElementBase {
 public:
  MarkupElementBlink(size_t first_pos, size_t last_pos, bool value)
      : MarkupElementBase(first_pos, last_pos), value_(value) {
  }

  void install(WindowOutputter &rb) const override {
    rb.set_blink(value_);
  }
  void uninstall(WindowOutputter &rb) const override {
    rb.unset_blink();
  }

 private:
  bool value_;
};

class MarkupElementNoLb : public MarkupElementTextEdit {
 public:
  MarkupElementNoLb(size_t first_pos, size_t last_pos, bool value)
      : MarkupElementTextEdit(first_pos, last_pos), value_(value) {
  }

  void install(TextEditBuilder &rb) const override;
  void uninstall(TextEditBuilder &rb) const override;

 private:
  bool value_;
};

class MarkupElementLeftPad : public MarkupElementTextEdit {
 public:
  MarkupElementLeftPad(size_t first_pos, size_t last_pos, std::string pad, Color color)
      : MarkupElementTextEdit(first_pos, last_pos), pad_(pad), color_(color) {
  }
  MarkupElementLeftPad(size_t first_pos, size_t last_pos, std::string pad, ColorRGB color)
      : MarkupElementTextEdit(first_pos, last_pos), pad_(pad), color_(color) {
  }

  void install(TextEditBuilder &rb) const override;
  void uninstall(TextEditBuilder &rb) const override;

 private:
  std::string pad_;
  td::Variant<Color, ColorRGB> color_;
};

class MarkupElementImage : public MarkupElementTextEdit {
 public:
  MarkupElementImage(size_t first_pos, size_t last_pos, std::string image_path, td::int32 image_height,
                     td::int32 image_width, td::int32 rendered_height, td::int32 rendered_width, bool allow_pixel)
      : MarkupElementTextEdit(first_pos, last_pos)
      , image_path_(std::move(image_path))
      , image_height_(image_height)
      , image_width_(image_width)
      , rendered_height_(rendered_height)
      , rendered_width_(rendered_width)
      , allow_pixel_(allow_pixel) {
  }

  void install(TextEditBuilder &rb) const override;
  void uninstall(TextEditBuilder &rb) const override;

 private:
  std::string image_path_;
  td::int32 image_height_;
  td::int32 image_width_;
  td::int32 rendered_height_;
  td::int32 rendered_width_;
  bool allow_pixel_;
};

class MarkupElementImageData : public MarkupElementTextEdit {
 public:
  MarkupElementImageData(size_t first_pos, size_t last_pos, std::string image_data, td::int32 image_height,
                         td::int32 image_width, td::int32 rendered_height, td::int32 rendered_width, bool allow_pixel)
      : MarkupElementTextEdit(first_pos, last_pos)
      , image_data_(std::move(image_data))
      , image_height_(image_height)
      , image_width_(image_width)
      , rendered_height_(rendered_height)
      , rendered_width_(rendered_width)
      , allow_pixel_(allow_pixel) {
  }

  void install(TextEditBuilder &rb) const override;
  void uninstall(TextEditBuilder &rb) const override;

 private:
  std::string image_data_;
  td::int32 image_height_;
  td::int32 image_width_;
  td::int32 rendered_height_;
  td::int32 rendered_width_;
  bool allow_pixel_;
};

using MarkupElement = std::shared_ptr<MarkupElementBase>;

}  // namespace windows
