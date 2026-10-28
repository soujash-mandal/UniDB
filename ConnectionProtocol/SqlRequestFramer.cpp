#include "SqlRequestFramer.h"

#include <utility>

namespace ConnectionProtocol {

SqlRequestFramingResult SqlRequestFramer::append(std::string_view bytes) {
  SqlRequestFramingResult result;

  std::size_t start = 0;
  while (start < bytes.size()) {
    const auto newline = bytes.find('\n', start);
    const auto end = newline == std::string_view::npos ? bytes.size() : newline;
    pending.append(bytes.substr(start, end - start));

    if (newline == std::string_view::npos) {
      // A trailing CR may be the first half of a CRLF split across reads.
      const bool possibleCrLf = !pending.empty() && pending.back() == '\r' &&
                                pending.size() == MAX_REQUEST_BYTES + 1;
      if (pending.size() > MAX_REQUEST_BYTES && !possibleCrLf) {
        pending.clear();
        result.error = "SQL request exceeds 64 KiB limit";
      }
      break;
    }

    // The limit applies to SQL bytes, excluding the CR in a CRLF terminator.
    if (!pending.empty() && pending.back() == '\r') {
      pending.pop_back();
    }
    if (pending.size() > MAX_REQUEST_BYTES) {
      pending.clear();
      result.error = "SQL request exceeds 64 KiB limit";
      return result;
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
