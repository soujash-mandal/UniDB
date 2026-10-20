#pragma once

#include "Frame.h"
#include <cstdint>
#include <mutex>
#include <vector>

class BufferPool {

private:
  std::vector<Frame> frames;
  std::mutex synchronizationMutex;

public:
  explicit BufferPool(uint32_t poolSize) : frames(poolSize) {}
  Frame &getFrame(uint32_t frameId) { return frames.at(frameId); }
  const Frame &getFrame(uint32_t frameId) const { return frames.at(frameId); }
  uint32_t size() const { return static_cast<uint32_t>(frames.size()); }

  // Serialize frame identity, pin counts, and eviction-policy transitions.
  std::mutex &getSynchronizationMutex() { return synchronizationMutex; }
};
