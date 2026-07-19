#include "sysdashboard/cpu_stats.hpp"

#include <cstdio>
#include <cstring>
#include <cctype>
#include <chrono>
#include <format>
#include <fstream>
#include <sstream>
#include <string>

namespace sysdashboard {

Result<CpuSnapshot> read_cpu_stats() {
  std::ifstream f("/proc/stat");
  if (!f.is_open()) {
    return std::unexpected(Error{1, "Cannot open /proc/stat"});
  }

  CpuSnapshot snap;
  std::string line;
  while (std::getline(f, line)) {
    if (!line.starts_with("cpu")) break;
    if (line.size() < 7) continue;

    CpuTime ct;
    std::istringstream ss(line);
    ss >> ct.label;
    // Fields: user nice system idle iowait irq softirq steal (guest/guest_nice ignored)
    ss >> ct.user >> ct.nice >> ct.system >> ct.idle;
    // Optional fields after idle
    long long val;
    if (ss >> val) ct.iowait = val;
    if (ss >> val) ct.irq = val;
    if (ss >> val) ct.softirq = val;
    if (ss >> val) ct.steal = val;

    snap.cpus.push_back(std::move(ct));
  }

  snap.timestamp = std::chrono::steady_clock::now();
  return snap;
}

std::vector<CpuUsage> compute_cpu_usage(const CpuSnapshot& prev, const CpuSnapshot& curr) {
  std::vector<CpuUsage> usages;
  size_t n = std::min(prev.cpus.size(), curr.cpus.size());
  for (size_t i = 0; i < n; ++i) {
    long long d_total = curr.cpus[i].total() - prev.cpus[i].total();
    long long d_idle = curr.cpus[i].idle - prev.cpus[i].idle;
    double pct = (d_total > 0)
      ? (static_cast<double>(d_total - d_idle) / d_total) * 100.0
      : 0.0;
    usages.push_back(CpuUsage{curr.cpus[i].label, pct});
  }
  return usages;
}

double compute_total_usage(const CpuSnapshot& prev, const CpuSnapshot& curr) {
  auto usages = compute_cpu_usage(prev, curr);
  if (usages.empty()) return 0.0;
  // First entry is "cpu" (total aggregate)
  return usages[0].percent;
}

} // namespace sysdashboard
