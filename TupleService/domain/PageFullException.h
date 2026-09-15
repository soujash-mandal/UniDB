#pragma once

#include <stdexcept>

class PageFullException : public std::runtime_error {
public:
  explicit PageFullException(const char *message)
      : std::runtime_error(message) {}
};
