#pragma once

#include <expected>
#include <string>

namespace sysdashboard {

struct Error {
  int code;
  std::string message;
};

template <typename T>
using Result = std::expected<T, Error>;

} // namespace sysdashboard
