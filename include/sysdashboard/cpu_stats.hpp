#pragma once

#include "sysdashboard/types.hpp"
#include <string>
#include <vector>
#include <chrono>

namespace sysdashboard {

struct CpuTime {
  std::string label;       // "cpu0", "cpu1", or "cpu" for total
  long long user = 0;
  long long nice = 0;
  long long system = 0;
  long long idle = 0;
  long long iowait = 0;
  long long irq = 0;
  long long softirq = 0;
  long long steal = 0;

  long long total() const { return user + nice + system + idle + iowait + irq + softirq + steal; }
  long long active() const { return total() - idle - iowait; }
};

struct CpuSnapshot {
  std::vector<CpuTime> cpus;
  std::chrono::steady_clock::time_point timestamp;
};

struct CpuUsage {
  std::string label;
  double percent = 0.0;    // 0.0–100.0
};

// Error and Result are defined in types.hpp

/// Reads /proc/stat and returns per-CPU time snapshots
Result<CpuSnapshot> read_cpu_stats();

/// Computes per-CPU usage percent between two snapshots
std::vector<CpuUsage> compute_cpu_usage(const CpuSnapshot& prev, const CpuSnapshot& curr);

/// Returns total CPU usage percent
double compute_total_usage(const CpuSnapshot& prev, const CpuSnapshot& curr);

} // namespace sysdashboard
