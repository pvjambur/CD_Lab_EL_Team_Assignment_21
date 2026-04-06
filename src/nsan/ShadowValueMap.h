// ShadowValueMap.h - Shadow Value Tracking
//
// Maintains a mapping from original LLVM IR Values to their
// higher-precision shadow values for consistency checking.

#ifndef SHADOW_VALUE_MAP_H
#define SHADOW_VALUE_MAP_H

#include "llvm/IR/Value.h"
#include <map>

using namespace llvm;

class ShadowValueMap {
public:
  Value *getShadowValue(Value *V) {
    auto it = mapping_.find(V);
    return (it != mapping_.end()) ? it->second : nullptr;
  }

  void mapShadowValue(Value *Original, Value *Shadow) {
    mapping_[Original] = Shadow;
  }

  bool hasShadow(Value *V) { return mapping_.count(V) > 0; }

  void clear() { mapping_.clear(); }

private:
  std::map<Value *, Value *> mapping_;
};

#endif // SHADOW_VALUE_MAP_H
