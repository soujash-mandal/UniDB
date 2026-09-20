#include "EvictionPolicyFactory.h"

#include "FIFOEvictionPolicy.h"

std::unique_ptr<EvictionPolicy>
EvictionPolicyFactory::Create(EvictionPolicyType type) {
  switch (type) {
  case EvictionPolicyType::FIFO:
    return std::make_unique<FIFOEvictionPolicy>();
  }

  return nullptr;
}