#include "sysdashboard/net_stats.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace sysdashboard {

static bool is_loopback(const std::string& iface) {
  return iface == "lo" || iface.starts_with("lo:");
}

Result<NetSnapshot> read_net_stats() {
  std::ifstream f("/proc/net/dev");
  if (!f.is_open()) {
    return std::unexpected(Error{1, "Cannot open /proc/net/dev"});
  }

  NetSnapshot snap;
  std::string line;

  // Skip first two header lines
  std::getline(f, line);
  std::getline(f, line);

  while (std::getline(f, line)) {
    auto colon = line.find(':');
    if (colon == std::string::npos) continue;

    NetDev nd;
    nd.interface = line.substr(0, colon);
    // Trim whitespace
    while (!nd.interface.empty() && (nd.interface.back() == ' ' || nd.interface.back() == '\t'))
      nd.interface.pop_back();

    if (is_loopback(nd.interface)) continue;

    std::istringstream ss(line.substr(colon + 1));
    ss >> nd.rx_bytes >> nd.rx_packets;
    // Skip fields 3-8
    for (int i = 0; i < 6; ++i) { unsigned long long dummy; ss >> dummy; }
    ss >> nd.tx_bytes >> nd.tx_packets;

    snap.interfaces.push_back(std::move(nd));
  }

  return snap;
}

std::vector<NetDelta> compute_net_deltas(const NetSnapshot& prev, const NetSnapshot& curr, double elapsed_s) {
  std::vector<NetDelta> deltas;
  if (elapsed_s <= 0.0) return deltas;

  for (const auto& cur : curr.interfaces) {
    auto it = std::find_if(prev.interfaces.begin(), prev.interfaces.end(),
      [&](const NetDev& p) { return p.interface == cur.interface; });
    if (it == prev.interfaces.end()) continue;

    double drx = static_cast<double>(cur.rx_bytes - it->rx_bytes);
    double dtx = static_cast<double>(cur.tx_bytes - it->tx_bytes);
    double drxp = static_cast<double>(cur.rx_packets - it->rx_packets);
    double dtxp = static_cast<double>(cur.tx_packets - it->tx_packets);

    deltas.push_back(NetDelta{
      .interface = cur.interface,
      .rx_kbps = drx * 8.0 / (1024.0 * elapsed_s),  // kbps (kilobits/sec)
      .tx_kbps = dtx * 8.0 / (1024.0 * elapsed_s),
      .rx_pps = drxp / elapsed_s,
      .tx_pps = dtxp / elapsed_s
    });
  }

  return deltas;
}

} // namespace sysdashboard
