#include "SqlRequestFramer.h"

namespace ConnectionProtocol {

SqlRequestFramingResult SqlRequestFramer::append(std::string_view bytes) {
  SqlRequestFramingResult result;

  std::size_t start = 0;
  while (start < bytes.size()) {
    const auto newline = bytes.find('\n', start);
    const auto end = newline == std::string_view::npos ? bytes.size() : newline;
    auto segment = bytes.substr(start, end - start);

    // CRLF's carriage return is a line terminator, not part of the SQL frame.
    const bool hasCrLf = newline != std::string_view::npos && !segment.empty() &&
                         segment.back() == '\r';
    const auto segmentSize = segment.size() - (hasCrLf ? 1U : 0U);
    if (pending.size() > MAX_REQUEST_BYTES ||
        segmentSize > MAX_REQUEST_BYTES - pending.size()) {
      pending.clear();
      result.error = "SQL request exceeds 64 KiB limit";
      return result;
    }

    pending.append(segment);
    if (newline == std::string_view::npos) {
      break;
    }

    if (!pending.empty() && pending.back() == '\r') {
      pending.pop_back();
    }
    if (pending.empty()) {
      result.error = "SQL request must not be empty";
      return result;
    }

    result.requests.push_back(std::move(pending));
    pending.clear();
    start = newline + 1;
  }

  return result;
}

} // namespace ConnectionProtocol
