#include "sysdashboard/mem_stats.hpp"

#include <cstdio>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace sysdashboard {

static long long parse_value(const std::string& line) {
  auto pos = line.find(':');
  if (pos == std::string::npos) return 0;
  long long val = 0;
  std::istringstream(line.substr(pos + 1)) >> val;
  return val;
}

double MemInfo::used_percent() const {
  return (total_kb > 0)
    ? (static_cast<double>(used_kb) / total_kb) * 100.0
    : 0.0;
}

double MemInfo::available_percent() const {
  return (total_kb > 0)
    ? (static_cast<double>(available_kb) / total_kb) * 100.0
    : 0.0;
}

double MemInfo::swap_used_percent() const {
  return (swap_total_kb > 0)
    ? (static_cast<double>(swap_used_kb) / swap_total_kb) * 100.0
    : 0.0;
}

std::string MemInfo::to_string(long long kb) const {
  if (kb >= 1024 * 1024) return std::format("{:.1f} GB", kb / (1024.0 * 1024.0));
  if (kb >= 1024) return std::format("{:.1f} MB", kb / 1024.0);
  return std::format("{} KB", kb);
}

Result<MemInfo> read_meminfo() {
  std::ifstream f("/proc/meminfo");
  if (!f.is_open()) {
    return std::unexpected(Error{1, "Cannot open /proc/meminfo"});
  }

  MemInfo mem;
  std::string line;
  while (std::getline(f, line)) {
    if (line.starts_with("MemTotal:"))       mem.total_kb = parse_value(line);
    else if (line.starts_with("MemFree:"))    mem.free_kb = parse_value(line);
    else if (line.starts_with("MemAvailable:")) mem.available_kb = parse_value(line);
    else if (line.starts_with("Buffers:"))    mem.buffers_kb = parse_value(line);
    else if (line.starts_with("Cached:"))     mem.cached_kb = parse_value(line);
    else if (line.starts_with("SwapTotal:"))  mem.swap_total_kb = parse_value(line);
    else if (line.starts_with("SwapFree:"))   mem.swap_free_kb = parse_value(line);
  }

  // used = total - available (or total - free - buffers - cached as fallback)
  if (mem.available_kb > 0) {
    mem.used_kb = mem.total_kb - mem.available_kb;
  } else {
    mem.used_kb = mem.total_kb - mem.free_kb - mem.buffers_kb - mem.cached_kb;
    if (mem.used_kb < 0) mem.used_kb = 0;
  }

  mem.swap_used_kb = mem.swap_total_kb - mem.swap_free_kb;
  if (mem.swap_used_kb < 0) mem.swap_used_kb = 0;

  return mem;
}

} // namespace sysdashboard
