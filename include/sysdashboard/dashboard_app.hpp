#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "sysdashboard/cpu_stats.hpp"
#include "sysdashboard/mem_stats.hpp"
#include "sysdashboard/disk_stats.hpp"
#include "sysdashboard/net_stats.hpp"

namespace sysdashboard {

struct DashboardState {
  // CPU
  CpuSnapshot cpu_prev;
  CpuSnapshot cpu_curr;
  std::vector<CpuUsage> cpu_usages;

  // Memory
  MemInfo mem;

  // Disk
  DiskSnapshot disk_prev;
  DiskSnapshot disk_curr;
  std::vector<DiskDelta> disk_deltas;

  // Network
  NetSnapshot net_prev;
  NetSnapshot net_curr;
  std::vector<NetDelta> net_deltas;

  // System
  double uptime_seconds = 0.0;
  int num_processes = 0;
  double load_1 = 0.0, load_5 = 0.0, load_15 = 0.0;
  int num_cpus = 0;
};

class DashboardApp {
public:
  DashboardApp();
  ~DashboardApp();

  void run();

private:
  void refresh_data();
  void read_system_info();

  DashboardState state_;
  std::atomic<bool> running_{true};
  int refresh_interval_ms_ = 1000;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace sysdashboard
