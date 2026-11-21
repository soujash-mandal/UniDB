#pragma once

#include "../port/LogStoragePort.h"

#include <fstream>
#include <string>

class FileLogStorageAdapter final : public LogStoragePort {
public:
  explicit FileLogStorageAdapter(std::string filePath);

  void writeAt(uint64_t offset,
               const std::vector<std::byte> &bytes) override;
  std::vector<std::byte> readAt(uint64_t offset,
                                std::size_t byteCount) override;
  void flush() override;

private:
  std::string filePath;
  std::fstream file;
};
