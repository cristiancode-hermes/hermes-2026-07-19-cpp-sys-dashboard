#pragma once

#include "sysdashboard/types.hpp"
#include <string>
#include <vector>

namespace sysdashboard {

struct DiskStat {
  std::string device;
  unsigned long long reads_completed = 0;
  unsigned long long reads_merged = 0;
  unsigned long long sectors_read = 0;
  unsigned long long time_reading_ms = 0;
  unsigned long long writes_completed = 0;
  unsigned long long writes_merged = 0;
  unsigned long long sectors_written = 0;
  unsigned long long time_writing_ms = 0;
  unsigned long long io_in_progress = 0;
};

struct DiskSnapshot {
  std::vector<DiskStat> disks;
};

struct DiskDelta {
  std::string device;
  double read_kbps = 0.0;
  double write_kbps = 0.0;
  double iops_read = 0.0;
  double iops_write = 0.0;
};

// Error and Result are defined in types.hpp

/// Reads /proc/diskstats for physical disks (sdX, nvmeXnY, vdX, mmcblkX)
Result<DiskSnapshot> read_disk_stats();

/// Computes deltas between two snapshots given elapsed seconds
std::vector<DiskDelta> compute_disk_deltas(const DiskSnapshot& prev, const DiskSnapshot& curr, double elapsed_s);

} // namespace sysdashboard
