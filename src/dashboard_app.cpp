#include "sysdashboard/dashboard_app.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <format>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <spdlog/spdlog.h>

using namespace ftxui;

namespace sysdashboard {

// ── Helpers ──

static std::string fmt_pct(double pct) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%5.1f%%", pct);
  return buf;
}

static Decorator pct_color(double pct) {
  return (pct > 80.0) ? color(Color::Red) :
         (pct > 50.0) ? color(Color::YellowLight) : color(Color::Green);
}

static Element make_percent_bar(double pct, int width = 15) {
  int filled = std::clamp(static_cast<int>(std::round(pct / 100.0 * width)), 0, width);
  auto fill = text(std::string(filled, '|')) | pct_color(pct);
  auto empty_elem = text(std::string(width - filled, '.')) | color(Color::GrayDark);
  return hbox(Elements{std::move(fill), std::move(empty_elem)});
}

static Element labeled_bar(const std::string& label, double pct) {
  return hbox(Elements{
    text(label) | size(WIDTH, GREATER_THAN, 12),
    separator(),
    make_percent_bar(pct),
    text(" " + fmt_pct(pct)),
  });
}

// ── System info readers ──

static double read_uptime() {
  std::ifstream f("/proc/uptime");
  if (!f.is_open()) return 0.0;
  double up = 0.0;
  f >> up;
  return up;
}

static void read_loadavg(double& l1, double& l5, double& l15, int& running, int& total) {
  std::ifstream f("/proc/loadavg");
  if (!f.is_open()) return;
  char slash;
  f >> l1 >> l5 >> l15 >> running >> slash >> total;
}

// ── DashboardApp implementation ──

struct DashboardApp::Impl {
  ScreenInteractive screen = ScreenInteractive::Fullscreen();
};

DashboardApp::DashboardApp()
  : impl_(std::make_unique<Impl>()) {}

DashboardApp::~DashboardApp() = default;

void DashboardApp::read_system_info() {
  state_.uptime_seconds = read_uptime();
  read_loadavg(state_.load_1, state_.load_5, state_.load_15,
               state_.num_processes, state_.num_processes);
  state_.num_cpus = static_cast<int>(std::thread::hardware_concurrency());
}

void DashboardApp::refresh_data() {
  // CPU
  state_.cpu_prev = std::move(state_.cpu_curr);
  auto cpu_result = read_cpu_stats();
  if (cpu_result) {
    state_.cpu_curr = std::move(*cpu_result);
    if (!state_.cpu_prev.cpus.empty()) {
      state_.cpu_usages = compute_cpu_usage(state_.cpu_prev, state_.cpu_curr);
    }
  }

  // Memory
  auto mem_result = read_meminfo();
  if (mem_result) state_.mem = std::move(*mem_result);

  // Disk
  state_.disk_prev = std::move(state_.disk_curr);
  auto disk_result = read_disk_stats();
  if (disk_result) {
    state_.disk_curr = std::move(*disk_result);
    if (!state_.disk_prev.disks.empty()) {
      state_.disk_deltas = compute_disk_deltas(state_.disk_prev, state_.disk_curr, 1.0);
    }
  }

  // Network
  state_.net_prev = std::move(state_.net_curr);
  auto net_result = read_net_stats();
  if (net_result) {
    state_.net_curr = std::move(*net_result);
    if (!state_.net_prev.interfaces.empty()) {
      state_.net_deltas = compute_net_deltas(state_.net_prev, state_.net_curr, 1.0);
    }
  }

  // System info (refresh once)
  if (state_.num_cpus == 0) read_system_info();
}

void DashboardApp::run() {
  auto& screen = impl_->screen;

  // Initial data
  refresh_data();

  // Build renderer
  auto renderer = Renderer([this] {
    Elements cpu_lines;
    for (const auto& usage : state_.cpu_usages) {
      if (usage.label == "cpu") continue; // skip aggregate, show per-core
      cpu_lines.push_back(labeled_bar(usage.label + ":", usage.percent));
    }

    Elements mem_lines;
    mem_lines.push_back(labeled_bar("Used:", state_.mem.used_percent()));
    mem_lines.push_back(labeled_bar("Available:", state_.mem.available_percent()));
    if (state_.mem.swap_total_kb > 0) {
      mem_lines.push_back(labeled_bar("Swap:", state_.mem.swap_used_percent()));
    }

    Elements disk_lines;
    for (const auto& dd : state_.disk_deltas) {
      disk_lines.push_back(hbox(Elements{
        text(dd.device) | size(WIDTH, GREATER_THAN, 10),
        separator(),
        text(std::format("R:{:.1f}K W:{:.1f}K", dd.read_kbps, dd.write_kbps)) | color(Color::Cyan),
      }));
    }
    if (disk_lines.empty()) {
      disk_lines.push_back(text("No disk activity") | color(Color::GrayDark));
    }

    Elements net_lines;
    for (const auto& nd : state_.net_deltas) {
      net_lines.push_back(hbox(Elements{
        text(nd.interface) | size(WIDTH, GREATER_THAN, 10),
        separator(),
        text(std::format("RX:{:.1f}K TX:{:.1f}K", nd.rx_kbps, nd.tx_kbps)) | color(Color::Cyan),
      }));
    }
    if (net_lines.empty()) {
      net_lines.push_back(text("No network activity") | color(Color::GrayDark));
    }

    // System info
    int days = static_cast<int>(state_.uptime_seconds) / 86400;
    int hours = (static_cast<int>(state_.uptime_seconds) % 86400) / 3600;
    int mins = (static_cast<int>(state_.uptime_seconds) % 3600) / 60;
    char uptime_buf[32];
    std::snprintf(uptime_buf, sizeof(uptime_buf), "%dd %02d:%02d", days, hours, mins);

    Elements sys_lines;
    sys_lines.push_back(text(std::format("Uptime:    {}", uptime_buf)));
    sys_lines.push_back(text(std::format("CPU cores: {}", state_.num_cpus)));
    sys_lines.push_back(text(std::format("Load avg:  {:.2f} {:.2f} {:.2f}",
      state_.load_1, state_.load_5, state_.load_15)));
    sys_lines.push_back(text(std::format("Mem:       {} / {} ({} free)",
      state_.mem.to_string(state_.mem.used_kb),
      state_.mem.to_string(state_.mem.total_kb),
      state_.mem.to_string(state_.mem.free_kb))));

    return vbox(Elements{
      text(" ⚙ SysDashboard v1.0") | bold | color(Color::Cyan) | center,
      separator(),
      text(" SYSTEM") | bold | color(Color::YellowLight),
      vbox(std::move(sys_lines)) | border,
      separator(),
      text(" CPU") | bold | color(Color::YellowLight),
      vbox(std::move(cpu_lines)) | border,
      separator(),
      text(" MEMORY") | bold | color(Color::YellowLight),
      vbox(std::move(mem_lines)) | border,
      separator(),
      text(" DISK") | bold | color(Color::YellowLight),
      vbox(std::move(disk_lines)) | border,
      separator(),
      text(" NETWORK") | bold | color(Color::YellowLight),
      vbox(std::move(net_lines)) | border,
      separator(),
      text("Ctrl+C to exit") | color(Color::GrayDark) | center,
    }) | border;
  });

  // Refresh thread
  std::thread refresh_thread([this, &screen] {
    while (running_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(refresh_interval_ms_));
      if (!running_) break;
      refresh_data();
      screen.PostEvent(Event::Custom);
    }
  });

  screen.Loop(renderer);
  running_ = false;
  if (refresh_thread.joinable()) refresh_thread.join();
}

} // namespace sysdashboard
