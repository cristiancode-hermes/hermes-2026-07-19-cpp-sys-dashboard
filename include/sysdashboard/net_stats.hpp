#pragma once

#include "sysdashboard/types.hpp"
#include <string>
#include <vector>

namespace sysdashboard {

struct NetDev {
  std::string interface;
  unsigned long long rx_bytes = 0;
  unsigned long long rx_packets = 0;
  unsigned long long tx_bytes = 0;
  unsigned long long tx_packets = 0;
};

struct NetSnapshot {
  std::vector<NetDev> interfaces;
};

struct NetDelta {
  std::string interface;
  double rx_kbps = 0.0;
  double tx_kbps = 0.0;
  double rx_pps = 0.0;
  double tx_pps = 0.0;
};

// Error and Result are defined in types.hpp

/// Reads /proc/net/dev for non-loopback interfaces
Result<NetSnapshot> read_net_stats();

/// Computes deltas between two snapshots given elapsed seconds
std::vector<NetDelta> compute_net_deltas(const NetSnapshot& prev, const NetSnapshot& curr, double elapsed_s);

} // namespace sysdashboard
