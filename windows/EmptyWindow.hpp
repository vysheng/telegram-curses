#pragma once

#include "td/utils/Time.h"
#include "td/utils/logging.h"
#include <atomic>
#include <memory>
#include <utility>
#include "Window.hpp"

namespace windows {

class EmptyWindow : public Window {
 public:
  td::int32 min_width() override {
    return 0;
  }
  td::int32 min_height() override {
    return 0;
  }
  void render(WindowOutputter &rb, bool force) override;
};

}  // namespace windows
