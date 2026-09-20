#pragma once

#include <memory>

#include "EvictionPolicy.h"
#include "EvictionPolicyType.h"

class EvictionPolicyFactory {
public:
  static std::unique_ptr<EvictionPolicy> Create(EvictionPolicyType type);
};