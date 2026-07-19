#pragma once

#include "sysdashboard/types.hpp"
#include <string>

namespace sysdashboard {

struct MemInfo {
  long long total_kb = 0;
  long long free_kb = 0;
  long long available_kb = 0;
  long long used_kb = 0;
  long long buffers_kb = 0;
  long long cached_kb = 0;
  long long swap_total_kb = 0;
  long long swap_free_kb = 0;
  long long swap_used_kb = 0;

  double used_percent() const;
  double available_percent() const;
  double swap_used_percent() const;
  std::string to_string(long long kb) const;
};

// Error and Result are defined in types.hpp

/// Reads /proc/meminfo and returns structured memory info
Result<MemInfo> read_meminfo();

} // namespace sysdashboard
