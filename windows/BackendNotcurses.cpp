#include "BackendNotcurses.h"
#include "Window.hpp"
#include "Output.hpp"
#include "td/utils/SharedSlice.h"
#include "td/utils/filesystem.h"
#include "unicode.h"

#include "rlottie/inc/rlottie.h"

#include <memory>
#include <notcurses/notcurses.h>
#include <jpeglib.h>
#include <vector>
#include <zlib.h>

namespace windows {

std::vector<int> color_to_rgb{0x000000, 0xcc0403, 0x19cb00, 0xcecb00, 0x0d73cc, 0xcb1ed1, 0x0dcdcd, 0xdddddd,
                              0x767676, 0xf2201f, 0x23fd00, 0xfffd00, 0x1a8fff, 0xfd28ff, 0x14ffff, 0xffffff};

class RenderedImageNotcurses : public RenderedImage {
 public:
  RenderedImageNotcurses(td::int32 renderd_to_width, td::int32 height, td::int32 width, td::int32 pix_height,
                         td::int32 pix_width, bool allow_pixel, struct ncplane *plane, struct ncvisual *visual)
      : renderd_to_width_(renderd_to_width)
      , height_(height)
      , width_(width)
      , pix_height_(pix_height)
      , pix_width_(pix_width)
      , allow_pixel_(allow_pixel)
      , plane_(plane)
      , visual_(visual) {
    rendered_height_ = height;
  }

  ~RenderedImageNotcurses() {
    if (plane_) {
      ncplane_destroy(plane_);
    }
    ncvisual_destroy(visual_);
  }

  void hide() {
    if (plane_) {
      ncplane_destroy(plane_);
      plane_ = nullptr;
    }
  }

  void move_yx(td::int32 y, td::int32 x) {
    if (plane_) {
      ncplane_move_yx(plane_, y, x);
    }
  }

  void render_slice(struct notcurses *nc, struct ncplane *baseplane, td::int32 offset, td::int32 slice_height,
                    bool is_active) {
    if (plane_ && offset_ == offset && rendered_height_ == slice_height && is_active_ == is_active) {
      return;
    }
    hide();
    CHECK(slice_height >= 0 && slice_height <= height_);
    if (slice_height == 0) {
      return;
    }
    struct ncvgeom geom;
    memset(&geom, 0, sizeof(geom));
    CHECK(ncvisual_geom(nc, visual_, nullptr, &geom) >= 0);
    CHECK(geom.cdimy > 0 && geom.cdimx > 0);

    struct ncplane *tmp_plane = nullptr;
    if (true) {
      struct ncplane_options plane_opts = {.y = 0,
                                           .x = 0,
                                           .rows = (unsigned int)slice_height,
                                           .cols = (unsigned int)width_,
                                           .userptr = nullptr,
                                           .name = nullptr,
                                           .resizecb = nullptr,
                                           .flags = NCPLANE_OPTION_FIXED,
                                           .margin_b = 0,
                                           .margin_r = 0};
      tmp_plane = ncplane_create(baseplane, &plane_opts);
      CHECK(tmp_plane);
      struct ncvisual_options opts = {.n = tmp_plane,
                                      .scaling = (is_active && allow_pixel_) ? NCSCALE_NONE_HIRES : NCSCALE_STRETCH,
                                      .y = 0,
                                      .x = 0,
                                      .begy = (unsigned int)(offset * geom.cdimy),
                                      .begx = 0,
                                      .leny = (unsigned int)(slice_height * geom.cdimy),
                                      .lenx = (unsigned int)pix_width_,
                                      .blitter = (is_active && allow_pixel_) ? NCBLIT_PIXEL : NCBLIT_DEFAULT,
                                      .flags = 0,
                                      .transcolor = 0,
                                      .pxoffy = 0,
                                      .pxoffx = 0};
      plane_ = ncvisual_blit(nc, visual_, &opts);
    } else if (true) {
      struct ncvisual_options opts = {.n = nullptr,
                                      .scaling = is_active ? NCSCALE_NONE_HIRES : NCSCALE_SCALE,
                                      .y = 0,
                                      .x = 0,
                                      .begy = (unsigned int)(offset * geom.cdimy),
                                      .begx = 0,
                                      .leny = (unsigned int)(slice_height * geom.cdimy),
                                      .lenx = (unsigned int)pix_width_,
                                      .blitter = NCBLIT_PIXEL,
                                      .flags = 0,
                                      .transcolor = 0,
                                      .pxoffy = 0,
                                      .pxoffx = 0};
      plane_ = ncvisual_blit(nc, visual_, &opts);
      if (plane_) {
        ncplane_reparent(plane_, baseplane);
        unsigned int y, x;
        ncplane_dim_yx(plane_, &y, &x);
        if (y != (unsigned int)slice_height) {
          memset(&geom, 0, sizeof(geom));
          struct ncvgeom geom;
          CHECK(ncvisual_geom(nc, visual_, &opts, &geom) >= 0);
          LOG(ERROR) << "opts: origin=" << opts.begy << "x" << opts.begx << " size=" << opts.leny << "x" << opts.lenx;
          LOG(ERROR) << "geom: origin=" << geom.begy << "x" << geom.begx << " size=" << geom.leny << "x" << geom.lenx
                     << " rpix=" << geom.rpixy << "x" << geom.rpixx << " rcell=" << geom.rcelly << "x" << geom.rcellx;
          LOG(ERROR) << "plane: size=" << y << "x" << x << " cellsize=" << geom.cdimy << "x" << geom.cdimx
                     << " pixelsize=" << (y * geom.cdimy) << "x" << (x * geom.cdimx);
        }
      }
    } else if (slice_height == height_) {
      struct ncvisual_options opts = {.n = baseplane,
                                      .scaling = NCSCALE_NONE,
                                      .y = 0,
                                      .x = 0,
                                      .begy = 0,
                                      .begx = 0,
                                      .leny = (unsigned int)pix_height_,
                                      .lenx = (unsigned int)pix_width_,
                                      .blitter = NCBLIT_PIXEL,
                                      .flags = NCVISUAL_OPTION_CHILDPLANE,
                                      .transcolor = 0,
                                      .pxoffy = 0,
                                      .pxoffx = 0};
      plane_ = ncvisual_blit(nc, visual_, &opts);
    } else {
      struct ncplane_options plane_opts = {.y = 0,
                                           .x = 0,
                                           .rows = (unsigned int)slice_height,
                                           .cols = (unsigned int)width_,
                                           .userptr = nullptr,
                                           .name = nullptr,
                                           .resizecb = nullptr,
                                           .flags = NCPLANE_OPTION_FIXED,
                                           .margin_b = 0,
                                           .margin_r = 0};
      plane_ = ncplane_create(baseplane, &plane_opts);
      CHECK(plane_);
    }
    if (plane_) {
      ncplane_move_below(plane_, baseplane);
    } else {
      if (tmp_plane) {
        ncplane_destroy(tmp_plane);
      }
    }
    offset_ = offset;
    rendered_height_ = slice_height;
    is_active_ = is_active;
  }

  td::int32 rendered_to_width() override {
    return renderd_to_width_;
  }
  td::int32 width() override {
    return width_;
  }
  td::int32 height() override {
    return height_;
  }

  auto plane() {
    return plane_;
  }

 private:
  td::int32 renderd_to_width_;
  td::int32 height_;
  td::int32 width_;
  td::int32 pix_height_;
  td::int32 pix_width_;
  bool allow_pixel_;
  td::int32 offset_{0};
  td::int32 rendered_height_{0};
  bool is_active_{false};
  struct ncplane *plane_;
  struct ncvisual *visual_;
};

class WindowOutputterNotcurses : public WindowOutputter {
 public:
  class BackendWindowNotcurses : public BackendWindow {
   public:
    BackendWindowNotcurses(struct ncplane *plane) : plane_(plane) {
    }
    ~BackendWindowNotcurses() {
      ncplane_destroy(plane_);
      plane_ = nullptr;
    }

    struct ncplane *plane() {
      return plane_;
    }

    void move_to_bottom() override {
      ncplane_move_bottom(plane_);
    }
    void move_below(BackendWindow *other) override {
      ncplane_move_below(plane_, static_cast<BackendWindowNotcurses *>(other)->plane_);
    }
    void move_above(BackendWindow *other) override {
      ncplane_move_above(plane_, static_cast<BackendWindowNotcurses *>(other)->plane_);
    }
    void move_to_top() override {
      ncplane_move_top(plane_);
    }
    void move_yx(td::int32 y_offset, td::int32 x_offset) override {
      ncplane_move_yx(plane_, y_offset, x_offset);
    }
    void resize(td::int32 height, td::int32 width) override {
      ncplane_resize(plane_, 0, 0, 0, 0, 0, 0, height, width);
    }

   private:
    struct ncplane *plane_;
  };
  WindowOutputterNotcurses(struct notcurses *nc, struct ncplane *rb, int y_offset, int x_offset, int height, int width,
                           td::uint32 default_fg_channel, td::uint32 default_bg_channel, bool is_active)
      : nc_(nc)
      , rb_(rb)
      , base_y_offset_(y_offset)
      , base_x_offset_(x_offset)
      , y_offset_(y_offset)
      , x_offset_(x_offset)
      , height_(height)
      , width_(width)
      , is_active_(is_active) {
    fg_channels_.push_back(default_fg_channel);
    bg_channels_.push_back(default_bg_channel);
    set_channels();
  }
  ~WindowOutputterNotcurses() {
  }
  td::int32 putstr_yx(td::int32 y, td::int32 x, const char *s, size_t len) override {
    if (y + y_offset_ < base_y_offset_ || y + y_offset_ >= base_y_offset_ + height_) {
      return (td::int32)len;
    }
    if (x + x_offset_ < base_x_offset_ || x + x_offset_ >= base_x_offset_ + width_) {
      return (td::int32)len;
    }
    if (!len) {
      len = strlen(s);
    }
    if (rb_) {
      char *tmp = strndup(s, len);

      ncplane_putstr_yx(rb_, y + y_offset_, x + x_offset_, tmp);

      free(tmp);
    }
    return (td::int32)len;
  }
  void cursor_move_yx(td::int32 y, td::int32 x, WindowOutputter::CursorShape cursor_shape) override {
    cursor_y_ = y + y_offset_;
    cursor_x_ = x + x_offset_;
    cursor_shape_ = cursor_shape;
  }

  void set_channels() {
    auto fg = fg_channels_.back();
    auto bg = bg_channels_.back();
    if (is_reversed_) {
      std::swap(fg, bg);
    }
    auto chan = NCCHANNELS_INITIALIZER((fg >> 16) & 0xff, (fg >> 8) & 0xff, (fg) & 0xff, (bg >> 16) & 0xff,
                                       (bg >> 8) & 0xff, (bg) & 0xff);
    ncplane_set_channels(rb_, chan);
  }

  void set_channels_transparent() {
    uint64_t chan = 0;
    CHECK(ncchannels_set_fg_alpha(&chan, NCALPHA_TRANSPARENT) >= 0);
    CHECK(ncchannels_set_bg_alpha(&chan, NCALPHA_TRANSPARENT) >= 0);
  }

  void set_fg_color(Color color) override {
    fg_channels_.push_back(color_to_rgb[(td::int32)color]);
    set_channels();
  }
  void set_fg_color_rgb(ColorRGB color) override {
    fg_channels_.push_back(color.color);
    set_channels();
  }
  void unset_fg_color() override {
    CHECK(fg_channels_.size() > 1);
    fg_channels_.pop_back();
    set_channels();
  }
  void set_bg_color(Color color) override {
    bg_channels_.push_back(color_to_rgb[(td::int32)color]);
    set_channels();
  }
  void set_bg_color_rgb(ColorRGB color) override {
    bg_channels_.push_back(color.color);
    set_channels();
  }
  void unset_bg_color() override {
    CHECK(bg_channels_.size() > 1);
    bg_channels_.pop_back();
    set_channels();
  }
  void set_bold(bool value) override {
    if (value) {
      ncplane_on_styles(rb_, NCSTYLE_BOLD);
    } else {
      ncplane_off_styles(rb_, NCSTYLE_BOLD);
    }
  }
  void unset_bold() override {
    ncplane_off_styles(rb_, NCSTYLE_BOLD);
  }
  void set_underline(bool value) override {
    if (value) {
      ncplane_on_styles(rb_, NCSTYLE_UNDERLINE);
    } else {
      ncplane_off_styles(rb_, NCSTYLE_UNDERLINE);
    }
  }
  void unset_underline() override {
    ncplane_off_styles(rb_, NCSTYLE_UNDERLINE);
  }
  void set_italic(bool value) override {
    if (value) {
      ncplane_on_styles(rb_, NCSTYLE_ITALIC);
    } else {
      ncplane_off_styles(rb_, NCSTYLE_ITALIC);
    }
  }
  void unset_italic() override {
    ncplane_off_styles(rb_, NCSTYLE_ITALIC);
  }
  void set_reverse(bool value) override {
    is_reversed_ = value;
    set_channels();
  }
  void unset_reverse() override {
    is_reversed_ = false;
    set_channels();
  }
  void set_strike(bool value) override {
    if (value) {
      ncplane_on_styles(rb_, NCSTYLE_STRUCK);
    } else {
      ncplane_off_styles(rb_, NCSTYLE_STRUCK);
    }
  }
  void unset_strike() override {
    ncplane_off_styles(rb_, NCSTYLE_STRUCK);
  }
  void set_blink(bool value) override {
  }
  void unset_blink() override {
  }
  bool is_real() const override {
    return rb_ != nullptr;
  }
  td::int32 local_cursor_y() const override {
    return cursor_y_ - base_y_offset_;
  }
  td::int32 local_cursor_x() const override {
    return cursor_x_ - base_x_offset_;
  }
  td::int32 global_cursor_y() const override {
    return cursor_y_;
  }
  td::int32 global_cursor_x() const override {
    return cursor_x_;
  }
  CursorShape cursor_shape() const override {
    return cursor_shape_;
  }
  void translate(td::int32 delta_y, td::int32 delta_x) override {
    y_offset_ += delta_y;
    x_offset_ += delta_x;
  }

  std::unique_ptr<WindowOutputter> create_subwindow_outputter(BackendWindow *bw, td::int32 y_offset, td::int32 x_offset,
                                                              td::int32 height, td::int32 width,
                                                              bool is_active) override {
    if (!bw) {
      return std::make_unique<WindowOutputterNotcurses>(
          nc_, rb_, y_offset + y_offset_, x_offset + x_offset_, height, width,
          color_to_rgb[(td::int32)(is_active ? Color::White : Color::Grey)], color_to_rgb[(td::int32)Color::Black],
          is_active);
    } else {
      auto b = static_cast<BackendWindowNotcurses *>(bw);
      return std::make_unique<WindowOutputterNotcurses>(
          nc_, b->plane(), 0, 0, height, width, color_to_rgb[(td::int32)(is_active ? Color::White : Color::Grey)],
          color_to_rgb[(td::int32)Color::Black], is_active);
    }
  }

  void update_cursor_position_from(WindowOutputter &from, BackendWindow *bw, td::int32 y_offset,
                                   td::int32 x_offset) override {
    if (!bw) {
      cursor_y_ = from.global_cursor_y();
      cursor_x_ = from.global_cursor_x();
      cursor_shape_ = from.cursor_shape();
    } else {
      cursor_y_ = from.global_cursor_y() + y_offset + y_offset_;
      cursor_x_ = from.global_cursor_x() + x_offset + x_offset_;
      cursor_shape_ = from.cursor_shape();
    }
  }

  bool is_active() const override {
    return is_active_;
  }

  bool allow_render_image() override {
    return true;
  }

  std::pair<td::int32, td::int32> rendered_image_height(td::int32 max_height, td::int32 max_width,
                                                        td::int32 image_height, td::int32 image_width,
                                                        std::string path) override {
    struct ncvisual *v = nullptr;
    SCOPE_EXIT {
      if (v) {
        ncvisual_destroy(v);
      }
    };
    struct ncvisual_options opts = {.n = rb_,
                                    .scaling = NCSCALE_NONE,
                                    .y = 0,
                                    .x = 0,
                                    .begy = 0,
                                    .begx = 0,
                                    .leny = 0,
                                    .lenx = 0,
                                    .blitter = NCBLIT_PIXEL,
                                    .flags = NCVISUAL_OPTION_CHILDPLANE,
                                    .transcolor = 0,
                                    .pxoffy = 0,
                                    .pxoffx = 0};

    struct ncvgeom geom;

    CHECK(ncvisual_geom(nc_, v, &opts, &geom) >= 0);

    if (!image_height || !image_width) {
      if (!geom.pixx || !geom.pixy) {
        return {0, 0};
      }
      image_height = geom.pixy;
      image_width = geom.pixx;
    }

    max_height *= geom.scaley;
    max_width *= geom.scalex;

    td::int32 real_height, real_width;

    if (1ll * max_width * image_height > 1ll * max_height * image_width) {
      real_height = max_height;
      real_width = (int)(1ll * max_height * image_width / image_height);
    } else {
      real_height = (int)(1ll * max_width * image_height / image_width);
      real_width = max_width;
    }

    return {(real_height + geom.scaley - 1) / geom.scaley, (real_width + geom.scalex - 1) / geom.scalex};
  }

  std::unique_ptr<RenderedImage> render_image_common(td::int32 max_height, td::int32 max_width, bool allow_pixel,
                                                     struct ncvisual *v) {
    auto saved_max_width = max_width;
    SCOPE_EXIT {
      if (v) {
        ncvisual_destroy(v);
      }
    };
    struct ncvisual_options opts = {.n = rb_,
                                    .scaling = NCSCALE_NONE,
                                    .y = 0,
                                    .x = 0,
                                    .begy = 0,
                                    .begx = 0,
                                    .leny = 0,
                                    .lenx = 0,
                                    .blitter = NCBLIT_PIXEL,
                                    .flags = NCVISUAL_OPTION_CHILDPLANE,
                                    .transcolor = 0,
                                    .pxoffy = 0,
                                    .pxoffx = 0};

    struct ncvgeom geom;

    CHECK(ncvisual_geom(nc_, v, &opts, &geom) >= 0);

    if (!geom.pixx || !geom.pixy) {
      return nullptr;
    }

    max_height *= geom.scaley;
    max_width *= geom.scalex;

    td::int32 real_height, real_width;

    if (1ll * max_width * geom.pixy > 1ll * max_height * geom.pixx) {
      real_height = max_height;
      real_width = (int)(1ll * max_height * geom.pixx / geom.pixy);
    } else {
      real_height = (int)(1ll * max_width * geom.pixy / geom.pixx);
      real_width = max_width;
    }

    ncvisual_resize(v, real_height, real_width);

    /*d::int32 renderd_to_width, td::int32 height, td::int32 width, struct ncplane *plane,
                         struct ncvisual *visual*/
    auto img = std::make_unique<RenderedImageNotcurses>(saved_max_width, (real_height + geom.scaley - 1) / geom.scaley,
                                                        (real_width + geom.scalex - 1) / geom.scalex, real_height,
                                                        real_width, allow_pixel, nullptr, v);
    v = nullptr;

    return img;
  }

  std::unique_ptr<RenderedImage> render_image(td::int32 max_height, td::int32 max_width, bool allow_pixel,
                                              std::string path) override {
    if (path.size() >= 4 && td::Slice(path).remove_prefix(path.size() - 4) == ".tgs") {
      auto f = gzopen(path.c_str(), "rb");
      if (!f) {
        LOG(ERROR) << "failed to read tgs";
        return nullptr;
      }
      SCOPE_EXIT {
        gzclose_r(f);
      };

      td::UniqueSlice buf(1 << 20);
      auto r = gzread(f, buf.as_mutable_slice().data(), (unsigned int)buf.size());
      if (r < 0) {
        LOG(ERROR) << "failed to uncompress tgs: " << r;
        return nullptr;
      }
      if ((unsigned int)r >= buf.size()) {
        LOG(ERROR) << "uncompressed sticker is bigger than " << buf.size();
        return nullptr;
      }

      auto animation = rlottie::Animation::loadFromData(buf.as_slice().truncate(r).str(), path);
      if (!animation) {
        LOG(ERROR) << "failed to parse animation";
        return nullptr;
      }

      td::uint32 buffer[512 * 512];
      rlottie::Surface surface(buffer, 512, 512, 512 * 4);
      animation->renderSync(0, surface);

      auto v = ncvisual_from_rgba(buffer, 512, 512 * 4, 512);
      if (!v) {
        LOG(ERROR) << "failed to create ncvisual";
        return nullptr;
      }
      return render_image_common(max_height, max_width, allow_pixel, v);
    }
    auto v = ncvisual_from_file(path.c_str());
    if (!v) {
      return nullptr;
    }
    return render_image_common(max_height, max_width, allow_pixel, v);
  }

  std::unique_ptr<RenderedImage> render_image_data(td::int32 max_height, td::int32 max_width, bool allow_pixel,
                                                   std::string data) override {
    struct jpeg_decompress_struct cinfo;

    JSAMPARRAY buffer;

    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, (const unsigned char *)data.data(), data.size());
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    buffer =
        (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, cinfo.output_width * cinfo.output_components, 1);
    std::vector<unsigned char> r;
    r.resize(cinfo.output_width * (size_t)cinfo.output_height * 3);
    size_t pos = 0;

    while (cinfo.output_scanline < cinfo.output_height) {
      (void)jpeg_read_scanlines(&cinfo, buffer, 1);
      unsigned char *pixel_row = (unsigned char *)(buffer[0]);
      // iterate over the pixels:
      for (unsigned int i = 0; i < 3 * cinfo.output_width; i++) {
        r[pos++] = *pixel_row++;
      }
    }

    auto v = ncvisual_from_rgb_packed((const void *)r.data(), cinfo.output_height, 3 * cinfo.output_width,
                                      cinfo.output_width, 0xff);
    if (!v) {
      return nullptr;
    }
    return render_image_common(max_height, max_width, allow_pixel, v);
  }

  void draw_rendered_image(td::int32 y, td::int32 x, RenderedImage &image) override {
    if (y + y_offset_ + image.height() < base_y_offset_ || y + y_offset_ >= base_y_offset_ + height_) {
      return hide_rendered_image(image);
    }
    if (x + x_offset_ + image.rendered_to_width() < base_x_offset_ || x + x_offset_ >= base_x_offset_ + width_) {
      return hide_rendered_image(image);
    }
    td::int32 top_offset = y + y_offset_ < base_y_offset_ ? base_y_offset_ - y - y_offset_ : 0;
    td::int32 height = y + y_offset_ + image.height() <= base_y_offset_ + height_
                           ? image.height()
                           : base_y_offset_ + height_ - y - y_offset_;
    static_cast<RenderedImageNotcurses &>(image).render_slice(nc_, rb_, top_offset, height - top_offset, is_active_);
    auto plane = static_cast<RenderedImageNotcurses &>(image).plane();
    if (plane) {
      ncplane_move_above(plane, rb_);
      static_cast<RenderedImageNotcurses &>(image).move_yx(y + y_offset_ + top_offset, x + x_offset_);
    }
  }

  void hide_rendered_image(RenderedImage &image) override {
    static_cast<RenderedImageNotcurses &>(image).hide();
  }

  void transparent_rect(td::int32 y, td::int32 x, td::int32 height, td::int32 width) override {
    //set_channels_transparent();
    erase_rect(y, x, height, width);
    //set_channels();
  }

 private:
  struct notcurses *nc_{nullptr};
  struct ncplane *rb_{nullptr};
  int base_y_offset_;
  int base_x_offset_;
  int y_offset_;
  int x_offset_;
  int height_;
  int width_;

  std::vector<td::uint32> fg_channels_;
  std::vector<td::uint32> bg_channels_;
  bool is_reversed_{false};

  td::int32 cursor_y_{0};
  td::int32 cursor_x_{0};
  CursorShape cursor_shape_{CursorShape::Block};

  bool is_active_;
};

class WindowOutputterEmptyNotcurses : public WindowOutputter {
 public:
  WindowOutputterEmptyNotcurses(struct notcurses *nc, struct ncplane *rb) : nc_(nc), rb_(rb) {
  }
  ~WindowOutputterEmptyNotcurses() {
  }
  td::int32 putstr_yx(td::int32 y, td::int32 x, const char *s, size_t len) override {
    if (!len) {
      len = strlen(s);
    }
    return (td::int32)len;
  }
  void cursor_move_yx(td::int32 y, td::int32 x, WindowOutputter::CursorShape cursor_shape) override {
    cursor_y_ = y;
    cursor_x_ = x;
    cursor_shape_ = cursor_shape;
  }
  void set_fg_color(Color color) override {
  }
  void set_fg_color_rgb(ColorRGB color) override {
  }
  void unset_fg_color() override {
  }
  void set_bg_color(Color color) override {
  }
  void set_bg_color_rgb(ColorRGB color) override {
  }
  void unset_bg_color() override {
  }
  void set_bold(bool value) override {
  }
  void unset_bold() override {
  }
  void set_underline(bool value) override {
  }
  void unset_underline() override {
  }
  void set_italic(bool value) override {
  }
  void unset_italic() override {
  }
  void set_reverse(bool value) override {
  }
  void unset_reverse() override {
  }
  void set_strike(bool value) override {
  }
  void unset_strike() override {
  }
  void set_blink(bool value) override {
  }
  void unset_blink() override {
  }
  bool is_real() const override {
    return false;
  }
  td::int32 local_cursor_y() const override {
    return cursor_y_;
  }
  td::int32 local_cursor_x() const override {
    return cursor_x_;
  }
  td::int32 global_cursor_y() const override {
    return cursor_y_;
  }
  td::int32 global_cursor_x() const override {
    return cursor_x_;
  }
  CursorShape cursor_shape() const override {
    return cursor_shape_;
  }
  void translate(td::int32 delta_y, td::int32 delta_x) override {
  }

  std::unique_ptr<WindowOutputter> create_subwindow_outputter(BackendWindow *bw, td::int32 y_offset, td::int32 x_offset,
                                                              td::int32 height, td::int32 width,
                                                              bool is_active) override {
    return std::make_unique<WindowOutputterEmptyNotcurses>(nc_, rb_);
  }
  void update_cursor_position_from(WindowOutputter &from, BackendWindow *bw, td::int32 y_offset,
                                   td::int32 x_offset) override {
    cursor_y_ = from.global_cursor_y();
    cursor_x_ = from.global_cursor_x();
    cursor_shape_ = from.cursor_shape();
  }
  bool is_active() const override {
    return false;
  }

  bool allow_render_image() override {
    return true;
  }

  std::pair<td::int32, td::int32> rendered_image_height(td::int32 max_height, td::int32 max_width,
                                                        td::int32 image_height, td::int32 image_width,
                                                        std::string path) override {
    struct ncvisual *v = nullptr;
    SCOPE_EXIT {
      if (v) {
        ncvisual_destroy(v);
      }
    };
    struct ncvisual_options opts = {.n = rb_,
                                    .scaling = NCSCALE_NONE,
                                    .y = 0,
                                    .x = 0,
                                    .begy = 0,
                                    .begx = 0,
                                    .leny = 0,
                                    .lenx = 0,
                                    .blitter = NCBLIT_PIXEL,
                                    .flags = NCVISUAL_OPTION_CHILDPLANE,
                                    .transcolor = 0,
                                    .pxoffy = 0,
                                    .pxoffx = 0};

    struct ncvgeom geom;

    CHECK(ncvisual_geom(nc_, v, &opts, &geom) >= 0);

    if (!image_height || !image_width) {
      /* we do not want to open file here */
      image_height = 10;
      image_width = 10;
    }

    max_height *= geom.scaley;
    max_width *= geom.scalex;

    td::int32 real_height, real_width;

    if (1ll * max_width * image_height > 1ll * max_height * image_width) {
      real_height = max_height;
      real_width = (int)(1ll * max_height * image_width / image_height);
    } else {
      real_height = (int)(1ll * max_width * image_height / image_width);
      real_width = max_width;
    }

    return {(real_height + geom.scaley - 1) / geom.scaley, (real_width + geom.scalex - 1) / geom.scalex};
  }

 private:
  td::int32 cursor_y_{0};
  td::int32 cursor_x_{0};
  CursorShape cursor_shape_{CursorShape::Block};

  struct notcurses *nc_{nullptr};
  struct ncplane *rb_{nullptr};
};

void create_empty_window_outputter_notcurses(void *notcurses, void *baseplane, void *renderplane) {
  set_empty_window_outputter(
      std::make_unique<WindowOutputterEmptyNotcurses>((struct notcurses *)notcurses, (struct ncplane *)renderplane));
}

std::unique_ptr<WindowOutputter> notcurses_window_outputter(void *notcurses, void *baseplane, void *renderplane,
                                                            td::int32 height, td::int32 width) {
  return std::make_unique<WindowOutputterNotcurses>((struct notcurses *)notcurses, (struct ncplane *)renderplane, 0, 0,
                                                    height, width, 0xdddddd, 0x000000, true);
}

std::unique_ptr<InputEvent> parse_notcurses_input_event(td::int32 code, td::MutableCSlice utf8, bool alt, bool ctrl,
                                                        bool shift) {
  if (code >= 'A' && code <= 'Z' && (!shift && (alt || ctrl))) {
    CHECK(utf8[0] == code);
    code += 'a' - 'A';
    utf8[0] += 'a' - 'A';
  }
  switch (code) {
    case NCKEY_UP:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Up");
    case NCKEY_RIGHT:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Right");
    case NCKEY_DOWN:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Down");
    case NCKEY_LEFT:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Left");
    case NCKEY_INS:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Insert");
    case NCKEY_DEL:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Delete");
    case NCKEY_BACKSPACE:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Backspace");
    case NCKEY_PGDOWN:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "PageDown");
    case NCKEY_PGUP:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "PageUp");
    case NCKEY_HOME:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Home");
    case NCKEY_END:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "End");
    case NCKEY_F00 ... NCKEY_F09: {
      static char s[3];
      s[0] = 'F';
      s[1] = (char)('0' + (code - NCKEY_F00));
      s[2] = 0;
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, td::CSlice(s, s + 2));
    }
    case NCKEY_F10 ... NCKEY_F40: {
      static char s[4];
      s[0] = 'F';
      auto v = (code - NCKEY_F00);
      s[1] = (char)('0' + (v / 10));
      s[2] = (char)('0' + (v % 10));
      s[3] = 0;
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, td::CSlice(s, s + 3));
    }
    case NCKEY_ENTER:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Enter");
    case NCKEY_TAB:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Tab");
    case NCKEY_ESC:
      return std::make_unique<CommonInputEvent>(true, alt, ctrl, "Escape");
  }

  LOG(DEBUG) << "code=" << code << " utf8=" << utf8.data();

  if (!nckey_pua_p(code) && !nckey_supppuaa_p(code) && !nckey_supppuab_p(code) &&
      code <= 1114112 /* beyond plane 17 */) {
    return std::make_unique<CommonInputEvent>(false, alt, ctrl, utf8);
  }

  return nullptr;
}

struct BackendNotcurses : public Backend {
  struct notcurses *nc_{nullptr};
  struct ncplane *baseplane_{nullptr};
  struct ncplane *renderplane_{nullptr};
  Screen *screen_{nullptr};
  bool cursor_enabled_{true};

  bool stop() override {
    if (nc_) {
      notcurses_stop(nc_);
      nc_ = nullptr;
      return true;
    } else {
      return false;
    }
  }

  void on_resize() override {
    if (baseplane_) {
      ncplane_resize_maximize(baseplane_);
    }
    if (renderplane_) {
      ncplane_resize_maximize(renderplane_);
    }
  }

  td::int32 height() override {
    unsigned lines, cols;
    notcurses_stddim_yx(nc_, &lines, &cols);
    return lines;
  }

  td::int32 width() override {
    unsigned lines, cols;
    notcurses_stddim_yx(nc_, &lines, &cols);
    return cols;
  }

  void tick() override {
    bool had_keys = false;
    while (true) {
      struct timespec ts = {.tv_sec = 0, .tv_nsec = 0};
      struct ncinput ni;
      auto res = notcurses_get(nc_, &ts, &ni);
      if (!res) {
        break;
      }

      if (ni.id == NCKEY_RESIZE) {
        notcurses_refresh(nc_, nullptr, nullptr);
        screen_->on_resize(height(), width());
        continue;
      }

      if (ni.evtype != NCTYPE_PRESS && ni.evtype != NCTYPE_REPEAT) {
        continue;
      }

      if (ncinput_ctrl_p(&ni) && ni.id == 'C') {
        notcurses_stop(nc_);
        _Exit(0);
      }

      char buf[64];
      size_t pos = 0;
      size_t p = 0;
      while (ni.eff_text[p]) {
        pos += utf8_code_to_str(ni.eff_text[p++], buf + pos);
      }

      td::MutableCSlice eff_text(buf, buf + pos);

      auto r =
          parse_notcurses_input_event(ni.id, eff_text, ncinput_alt_p(&ni), ncinput_ctrl_p(&ni), ncinput_shift_p(&ni));
      if (!r) {
        continue;
      }
      screen_->handle_input(*r);
      had_keys = true;
    }
    screen_->refresh(had_keys);
  }

  void refresh(bool force, std::shared_ptr<Window> base_window) override {
    if (!force && (!base_window->need_refresh() || !base_window->need_refresh_at().is_in_past())) {
      return;
    }
    td::int32 cursor_y = 0, cursor_x = 0;
    WindowOutputter::CursorShape cursor_shape = WindowOutputter::CursorShape::None;
    {
      auto rb = notcurses_window_outputter(nc_, baseplane_, renderplane_, height(), width());
      base_window->render_wrap(*rb, force);
      cursor_y = rb->global_cursor_y();
      cursor_x = rb->global_cursor_x();
      cursor_shape = rb->cursor_shape();
    }

    notcurses_render(nc_);

    if (cursor_shape != WindowOutputter::CursorShape::None && cursor_y >= 0 && cursor_x >= 0) {
      notcurses_cursor_enable(nc_, cursor_y, cursor_x);
      cursor_enabled_ = true;
    } else {
      if (cursor_enabled_) {
        notcurses_cursor_enable(nc_, 0, 0);
        notcurses_cursor_disable(nc_);
        cursor_enabled_ = false;
      }
    }
  }

  td::int32 poll_fd() override {
    return notcurses_inputready_fd(nc_);
  }

  void create_backend_window(std::shared_ptr<Window> window) override {
    struct ncplane_options plane_opts = {.y = 0,
                                         .x = 0,
                                         .rows = 1,
                                         .cols = 1,
                                         .userptr = nullptr,
                                         .name = nullptr,
                                         .resizecb = nullptr,
                                         .flags = NCPLANE_OPTION_FIXED,
                                         .margin_b = 0,
                                         .margin_r = 0};
    auto plane = ncplane_create(baseplane_, &plane_opts);
    auto bw = std::make_unique<WindowOutputterNotcurses::BackendWindowNotcurses>(plane);
    window->set_backend_window(std::move(bw));
  }

  void delete_backend_window(Window *window) override {
    window->release_backend_window();
  }
};

void init_notcurses_backend(Screen *screen) {
  auto backend = std::make_unique<BackendNotcurses>();

  struct notcurses_options curses_opts {
    .termtype = NULL, .loglevel = NCLOGLEVEL_WARNING, .margin_t = 0, .margin_r = 0, .margin_b = 0, .margin_l = 0,
    .flags = NCOPTION_NO_QUIT_SIGHANDLERS
  };

  backend->screen_ = screen;
  backend->nc_ = notcurses_init(&curses_opts, NULL);
  backend->baseplane_ = notcurses_stdplane(backend->nc_);

  struct ncplane_options plane_opts = {.y = 0,
                                       .x = 0,
                                       .rows = (unsigned int)backend->height(),
                                       .cols = (unsigned int)backend->width(),
                                       .userptr = nullptr,
                                       .name = nullptr,
                                       .resizecb = nullptr,
                                       .flags = NCPLANE_OPTION_FIXED,
                                       .margin_b = 0,
                                       .margin_r = 0};

  backend->renderplane_ = ncplane_create(backend->baseplane_, &plane_opts);
  create_empty_window_outputter_notcurses(backend->nc_, backend->baseplane_, backend->renderplane_);
  screen->set_backend(std::move(backend));
}

}  // namespace windows
