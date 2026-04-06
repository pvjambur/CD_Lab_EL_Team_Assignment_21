// ShadowValueMap.h - Shadow Value Tracking
//
// Maintains a mapping from original LLVM IR Values to their
// higher-precision shadow values for consistency checking.

#ifndef SHADOW_VALUE_MAP_H
#define SHADOW_VALUE_MAP_H

#include "llvm/IR/Value.h"
#include <map>

class ShadowValueMap {
public:
  llvm::Value *getShadowValue(llvm::Value *V) {
    auto it = mapping_.find(V);
    return (it != mapping_.end()) ? it->second : nullptr;
  }

  void mapShadowValue(llvm::Value *Original, llvm::Value *Shadow) {
    mapping_[Original] = Shadow;
  }

  bool hasShadow(llvm::Value *V) { return mapping_.count(V) > 0; }

  void clear() { mapping_.clear(); }

private:
  std::map<llvm::Value *, llvm::Value *> mapping_;
};

#endif // SHADOW_VALUE_MAP_H
