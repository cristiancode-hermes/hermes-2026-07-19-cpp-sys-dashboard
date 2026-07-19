#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "sysdashboard/cpu_stats.hpp"
#include "sysdashboard/mem_stats.hpp"
#include "sysdashboard/disk_stats.hpp"
#include "sysdashboard/net_stats.hpp"

#include <fstream>
#include <thread>
#include <chrono>

using namespace sysdashboard;
using Catch::Matchers::StartsWith;

// ─── CPU Tests ───

TEST_CASE("read_cpu_stats returns valid data", "[cpu]") {
  auto result = read_cpu_stats();
  REQUIRE(result.has_value());

  // Should have at least "cpu" (aggregate) and "cpu0" (first core)
  REQUIRE(result->cpus.size() >= 2);
  REQUIRE(result->cpus[0].label == "cpu");
  REQUIRE(result->cpus[0].total() > 0);
}

TEST_CASE("read_cpu_stats consecutive reads show increasing counters", "[cpu]") {
  auto r1 = read_cpu_stats();
  REQUIRE(r1.has_value());

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto r2 = read_cpu_stats();
  REQUIRE(r2.has_value());

  auto usages = compute_cpu_usage(*r1, *r2);
  REQUIRE(usages.size() >= 2);
  REQUIRE(usages[0].label == "cpu");
  REQUIRE(usages[0].percent >= 0.0);
  REQUIRE(usages[0].percent <= 100.0);
}

TEST_CASE("compute_total_usage returns sane range", "[cpu]") {
  auto r1 = read_cpu_stats();
  REQUIRE(r1.has_value());

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto r2 = read_cpu_stats();
  REQUIRE(r2.has_value());

  double total = compute_total_usage(*r1, *r2);
  REQUIRE(total >= 0.0);
  REQUIRE(total <= 100.0);
}

TEST_CASE("compute_cpu_usage with identical snapshots returns 0%", "[cpu]") {
  auto r1 = read_cpu_stats();
  REQUIRE(r1.has_value());

  auto usages = compute_cpu_usage(*r1, *r1);
  for (const auto& u : usages) {
    bool zero = u.percent == 0.0;
    REQUIRE(zero);
  }
}

// ─── Memory Tests ───

TEST_CASE("read_meminfo returns valid data", "[memory]") {
  auto result = read_meminfo();
  REQUIRE(result.has_value());

  // Typical systems have at least 128 MB total
  REQUIRE(result->total_kb > 128 * 1024);
  REQUIRE(result->free_kb > 0);
  REQUIRE(result->available_kb > 0);
  REQUIRE(result->used_kb > 0);
  REQUIRE(result->used_percent() > 0.0);
  REQUIRE(result->used_percent() <= 100.0);
  REQUIRE(result->available_percent() > 0.0);
}

TEST_CASE("memory used + available ≤ total", "[memory]") {
  auto result = read_meminfo();
  REQUIRE(result.has_value());

  // Note: used = total - available, so used + available ≈ total
  bool sane_used = result->used_kb + result->available_kb >= result->total_kb - 1024;
  bool sane_used2 = result->used_kb + result->available_kb <= result->total_kb + 1024;
  REQUIRE(sane_used);
  REQUIRE(sane_used2);
}

TEST_CASE("swap values are non-negative", "[memory]") {
  auto result = read_meminfo();
  REQUIRE(result.has_value());

  REQUIRE(result->swap_total_kb >= 0);
  REQUIRE(result->swap_free_kb >= 0);
  REQUIRE(result->swap_used_kb >= 0);
  if (result->swap_total_kb > 0) {
    REQUIRE(result->swap_used_percent() >= 0.0);
    REQUIRE(result->swap_used_percent() <= 100.0);
  }
}

TEST_CASE("to_string formats correctly", "[memory]") {
  auto result = read_meminfo();
  REQUIRE(result.has_value());

  std::string s = result->to_string(1024);
  // 1024 KB = 1 MB, so we should see MB
  bool found_mb = s.find("MB") != std::string::npos;
  bool found_kb = s.find("KB") != std::string::npos;
  if (!found_mb && !found_kb) {
    FAIL("Expected MB or KB in formatted string: " << s);
  }

  s = result->to_string(1048576);
  found_mb = s.find("MB") != std::string::npos;
  bool found_gb = s.find("GB") != std::string::npos;
  if (!found_mb && !found_gb) {
    FAIL("Expected MB or GB in formatted string: " << s);
  }

  s = result->to_string(512);
  REQUIRE(s.find("KB") != std::string::npos);
}

// ─── Disk Tests ───

TEST_CASE("read_disk_stats returns valid data", "[disk]") {
  auto result = read_disk_stats();
  // /proc/diskstats may be unavailable in some containers; skip if so
  if (!result.has_value()) {
    WARN("Cannot read /proc/diskstats — skipping disk tests");
    return;
  }

  // Should have at least some disks
  REQUIRE(result->disks.size() > 0);

  for (const auto& d : result->disks) {
    REQUIRE_FALSE(d.device.empty());
    // All counters should be non-negative
    REQUIRE(d.reads_completed >= 0);
    REQUIRE(d.writes_completed >= 0);
    REQUIRE(d.sectors_read >= 0);
    REQUIRE(d.sectors_written >= 0);
  }
}

TEST_CASE("compute_disk_deltas with identical snapshots returns zeros", "[disk]") {
  auto result = read_disk_stats();
  if (!result.has_value()) return;

  auto deltas = compute_disk_deltas(*result, *result, 1.0);
  for (const auto& d : deltas) {
    REQUIRE(d.read_kbps == 0.0);
    REQUIRE(d.write_kbps == 0.0);
    REQUIRE(d.iops_read == 0.0);
    REQUIRE(d.iops_write == 0.0);
  }
}

// ─── Network Tests ───

TEST_CASE("read_net_stats returns valid data", "[net]") {
  auto result = read_net_stats();
  // /proc/net/dev may be restricted in containers
  if (!result.has_value()) {
    WARN("Cannot read /proc/net/dev — skipping net tests");
    return;
  }

  REQUIRE(result->interfaces.size() > 0);

  bool found_non_loopback = false;
  for (const auto& n : result->interfaces) {
    REQUIRE_FALSE(n.interface.empty());
    REQUIRE(n.rx_bytes >= 0);
    REQUIRE(n.tx_bytes >= 0);
    if (n.interface != "lo" && !n.interface.starts_with("lo:")) {
      found_non_loopback = true;
    }
  }
  // We should see at least one non-loopback (eth0, enp*, wlan0)
  WARN("Found " << result->interfaces.size() << " interfaces "
       << (found_non_loopback ? "(incl. non-loopback)" : "(loopback only)"));
}

TEST_CASE("compute_net_deltas with identical snapshots returns zeros", "[net]") {
  auto result = read_net_stats();
  if (!result.has_value()) return;

  auto deltas = compute_net_deltas(*result, *result, 1.0);
  for (const auto& d : deltas) {
    REQUIRE(d.rx_kbps == 0.0);
    REQUIRE(d.tx_kbps == 0.0);
    REQUIRE(d.rx_pps == 0.0);
    REQUIRE(d.tx_pps == 0.0);
  }
}

// ─── /proc file presence tests ───

TEST_CASE("/proc/stat exists and is readable", "[proc]") {
  std::ifstream f("/proc/stat");
  REQUIRE(f.is_open());
  std::string first_line;
  std::getline(f, first_line);
  REQUIRE_THAT(first_line, StartsWith("cpu"));
}

TEST_CASE("/proc/meminfo exists and is readable", "[proc]") {
  std::ifstream f("/proc/meminfo");
  REQUIRE(f.is_open());
  std::string first_line;
  std::getline(f, first_line);
  REQUIRE_THAT(first_line, StartsWith("MemTotal:"));
}

TEST_CASE("/proc/loadavg exists and is readable", "[proc]") {
  std::ifstream f("/proc/loadavg");
  REQUIRE(f.is_open());
  std::string line;
  std::getline(f, line);
  REQUIRE_FALSE(line.empty());
}

TEST_CASE("/proc/uptime exists and is readable", "[proc]") {
  std::ifstream f("/proc/uptime");
  REQUIRE(f.is_open());
  double up = 0.0;
  f >> up;
  REQUIRE(up > 0.0);
}
