#include "FileLogStorageAdapter.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace {
std::streamoff ToStreamOffset(uint64_t offset) {
  if (offset >
      static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    throw std::out_of_range("Log offset exceeds supported file range");
  }
  return static_cast<std::streamoff>(offset);
}

std::streamsize ToStreamSize(std::size_t size) {
  if (size >
      static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
    throw std::out_of_range("Log operation exceeds supported stream size");
  }
  return static_cast<std::streamsize>(size);
}
} // namespace

FileLogStorageAdapter::FileLogStorageAdapter(std::string path)
    : filePath(std::move(path)) {
  file.open(filePath, std::ios::in | std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    std::ofstream createFile(filePath, std::ios::out | std::ios::binary);
    if (!createFile.is_open()) {
      throw std::runtime_error("Could not create WAL storage file");
    }
    createFile.close();

    file.clear();
    file.open(filePath, std::ios::in | std::ios::out | std::ios::binary);
  }

  if (!file.is_open()) {
    throw std::runtime_error("Could not open WAL storage file");
  }
}

void FileLogStorageAdapter::writeAt(
    uint64_t offset, const std::vector<std::byte> &bytes) {
  if (bytes.empty()) {
    return;
  }

  file.clear();
  file.seekp(ToStreamOffset(offset));
  if (!file) {
    throw std::runtime_error("Could not seek to WAL write offset");
  }

  file.write(reinterpret_cast<const char *>(bytes.data()),
             ToStreamSize(bytes.size()));
  if (!file) {
    throw std::runtime_error("Could not write WAL bytes");
  }
}

std::vector<std::byte> FileLogStorageAdapter::readAt(uint64_t offset,
                                                     std::size_t byteCount) {
  std::vector<std::byte> bytes(byteCount);
  if (byteCount == 0) {
    return bytes;
  }

  file.clear();
  file.seekg(ToStreamOffset(offset));
  if (!file) {
    throw std::runtime_error("Could not seek to WAL read offset");
  }

  file.read(reinterpret_cast<char *>(bytes.data()), ToStreamSize(byteCount));
  if (file.gcount() != ToStreamSize(byteCount)) {
    throw std::runtime_error("Requested WAL bytes are not available");
  }
  return bytes;
}

void FileLogStorageAdapter::flush() {
  file.flush();
  if (!file) {
    throw std::runtime_error("Could not flush WAL storage stream");
  }
}
