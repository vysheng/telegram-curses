#include "Output.hpp"
#include <memory>
#include <utility>
#include <vector>
#include "td/utils/logging.h"
#include "td/utils/ScopeGuard.h"

namespace windows {

static std::unique_ptr<WindowOutputter> empty_window_outputter_var;

WindowOutputter &empty_window_outputter() {
  return *empty_window_outputter_var;
}

void set_empty_window_outputter(std::unique_ptr<WindowOutputter> outputter) {
  empty_window_outputter_var = std::move(outputter);
}

}  // namespace windows
