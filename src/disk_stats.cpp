#include "sysdashboard/disk_stats.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace sysdashboard {

/// Check if a device name is a physical disk (not ram, loop, dm-)
static bool is_physical_disk(const std::string& dev) {
  if (dev.starts_with("ram") || dev.starts_with("loop") || dev.starts_with("dm-"))
    return false;
  if (dev.starts_with("sd") || dev.starts_with("nvme") ||
      dev.starts_with("vd") || dev.starts_with("mmcblk") ||
      dev.starts_with("xvd"))
    return true;
  // Accept only major device numbers (sdX, nvme0n1, etc.) — skip partitions
  // Partitions have higher minor numbers; we accept all for simplicity
  return !dev.empty() && std::isalpha(static_cast<unsigned char>(dev[0]));
}

Result<DiskSnapshot> read_disk_stats() {
  std::ifstream f("/proc/diskstats");
  if (!f.is_open()) {
    return std::unexpected(Error{1, "Cannot open /proc/diskstats"});
  }

  DiskSnapshot snap;
  std::string line;
  while (std::getline(f, line)) {
    std::istringstream ss(line);
    int major = 0, minor = 0;
    DiskStat ds;
    ss >> major >> minor >> ds.device;

    if (!is_physical_disk(ds.device)) continue;

    ss >> ds.reads_completed >> ds.reads_merged >> ds.sectors_read
       >> ds.time_reading_ms >> ds.writes_completed >> ds.writes_merged
       >> ds.sectors_written >> ds.time_writing_ms >> ds.io_in_progress;

    // Read remaining fields (discard)
    snap.disks.push_back(std::move(ds));
  }

  return snap;
}

std::vector<DiskDelta> compute_disk_deltas(const DiskSnapshot& prev, const DiskSnapshot& curr, double elapsed_s) {
  std::vector<DiskDelta> deltas;
  if (elapsed_s <= 0.0) return deltas;

  for (const auto& cur : curr.disks) {
    auto it = std::find_if(prev.disks.begin(), prev.disks.end(),
      [&](const DiskStat& p) { return p.device == cur.device; });
    if (it == prev.disks.end()) continue;

    double dr = static_cast<double>(cur.sectors_read - it->sectors_read) * 512.0;  // bytes
    double dw = static_cast<double>(cur.sectors_written - it->sectors_written) * 512.0;
    double rr = static_cast<double>(cur.reads_completed - it->reads_completed);
    double wr = static_cast<double>(cur.writes_completed - it->writes_completed);

    deltas.push_back(DiskDelta{
      .device = cur.device,
      .read_kbps = dr / (1024.0 * elapsed_s),
      .write_kbps = dw / (1024.0 * elapsed_s),
      .iops_read = rr / elapsed_s,
      .iops_write = wr / elapsed_s
    });
  }

  return deltas;
}

} // namespace sysdashboard
