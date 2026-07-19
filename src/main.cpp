#include "sysdashboard/dashboard_app.hpp"

#include <cstdio>
#include <iostream>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "sysdashboard/cpu_stats.hpp"
#include "sysdashboard/mem_stats.hpp"
#include "sysdashboard/disk_stats.hpp"
#include "sysdashboard/net_stats.hpp"

using json = nlohmann::json;

static void print_json() {
  json out;

  auto cpu = sysdashboard::read_cpu_stats();
  if (cpu) {
    auto& jcpu = out["cpu"];
    for (const auto& c : cpu->cpus) {
      jcpu[c.label] = {
        {"user", c.user}, {"nice", c.nice}, {"system", c.system},
        {"idle", c.idle}, {"iowait", c.iowait}, {"irq", c.irq},
        {"softirq", c.softirq}, {"steal", c.steal}
      };
    }
  }

  auto mem = sysdashboard::read_meminfo();
  if (mem) {
    out["memory"] = {
      {"total_kb", mem->total_kb},
      {"free_kb", mem->free_kb},
      {"available_kb", mem->available_kb},
      {"used_kb", mem->used_kb},
      {"used_percent", mem->used_percent()},
      {"swap_total_kb", mem->swap_total_kb},
      {"swap_used_kb", mem->swap_used_kb}
    };
  }

  auto disk = sysdashboard::read_disk_stats();
  if (disk) {
    auto& jdisk = out["disk"] = json::array();
    for (const auto& d : disk->disks) {
      jdisk.push_back({
        {"device", d.device},
        {"reads", d.reads_completed},
        {"writes", d.writes_completed},
        {"sectors_read", d.sectors_read},
        {"sectors_written", d.sectors_written}
      });
    }
  }

  auto net = sysdashboard::read_net_stats();
  if (net) {
    auto& jnet = out["network"] = json::array();
    for (const auto& n : net->interfaces) {
      jnet.push_back({
        {"interface", n.interface},
        {"rx_bytes", n.rx_bytes},
        {"tx_bytes", n.tx_bytes},
        {"rx_packets", n.rx_packets},
        {"tx_packets", n.tx_packets}
      });
    }
  }

  std::cout << out.dump(2) << std::endl;
}

int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::warn);

  for (int i = 1; i < argc; ++i) {
    std::string_view arg{argv[i]};
    if (arg == "--json" || arg == "-j") {
      print_json();
      return 0;
    }
    if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: sys-dashboard [--json]\n";
      std::cout << "  Interactive TUI dashboard of system resources.\n";
      std::cout << "  --json    Print stats as JSON and exit.\n";
      return 0;
    }
  }

  sysdashboard::DashboardApp app;
  app.run();
  return 0;
}
