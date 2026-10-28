#include "SqlRequestFramer.h"

#include <stdexcept>
#include <utility>

namespace ConnectionProtocol {

std::vector<std::string> SqlRequestFramer::append(std::string_view bytes) {
  std::vector<std::string> requests;

  std::size_t start = 0;
  while (start < bytes.size()) {
    const auto newline = bytes.find('\n', start);
    const auto end = newline == std::string_view::npos ? bytes.size() : newline;
    const auto segment = bytes.substr(start, end - start);

    if (segment.size() > MAX_REQUEST_BYTES - (pending.size() > MAX_REQUEST_BYTES
                                                  ? MAX_REQUEST_BYTES
                                                  : pending.size())) {
      throw std::length_error("SQL request exceeds 64 KiB limit");
    }
    pending.append(segment);

    if (newline == std::string_view::npos) {
      break;
    }

    if (!pending.empty() && pending.back() == '\r') {
      pending.pop_back();
    }
    if (pending.empty()) {
      throw std::invalid_argument("SQL request must not be empty");
    }
    requests.push_back(std::move(pending));
    pending.clear();
    start = newline + 1;
  }

  if (bytes.empty() && pending.size() > MAX_REQUEST_BYTES) {
    throw std::length_error("SQL request exceeds 64 KiB limit");
  }
  if (pending.size() > MAX_REQUEST_BYTES) {
    throw std::length_error("SQL request exceeds 64 KiB limit");
  }
  return requests;
}

} // namespace ConnectionProtocol
