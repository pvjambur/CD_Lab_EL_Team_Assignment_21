#ifndef SHADOW_VALUE_MAP_H
#define SHADOW_VALUE_MAP_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include <cassert>
#include <cstddef>

namespace llvm { class VectorType; }

class ShadowValueMap {
public:
  ShadowValueMap()  = default;
  ~ShadowValueMap() = default;

  ShadowValueMap(const ShadowValueMap &)            = delete;
  ShadowValueMap &operator=(const ShadowValueMap &) = delete;
  ShadowValueMap(ShadowValueMap &&)            = default;
  ShadowValueMap &operator=(ShadowValueMap &&) = default;

  void setShadowValue(llvm::Value *Original, llvm::Value *Shadow) {
    assert(Original != nullptr && "setShadowValue: Original must not be null");
    assert(Shadow   != nullptr && "setShadowValue: Shadow must not be null");

#ifndef NDEBUG
    llvm::Type *OT = Original->getType();
    llvm::Type *ST = Shadow->getType();

    if (auto *OVT = llvm::dyn_cast<llvm::VectorType>(OT)) {
      auto *SVT = llvm::dyn_cast<llvm::VectorType>(ST);
      assert(SVT && "setShadowValue: original is a vector but shadow is not");
      assert(OVT->getElementCount() == SVT->getElementCount() &&
             "setShadowValue: vector lane count mismatch");
      OT = OVT->getElementType();
      ST = SVT->getElementType();
    }

    assert(OT->isFloatingPointTy() && "setShadowValue: Original must have a floating-point type");
    assert(ST->isFloatingPointTy() && "setShadowValue: Shadow must have a floating-point type");
    assert(ST->getFPMantissaWidth() > OT->getFPMantissaWidth() &&
           "setShadowValue: shadow precision must be strictly greater than original");
#endif

    map_[Original] = Shadow;
  }

  llvm::Value *getShadowValue(llvm::Value *Original) const {
    assert(Original != nullptr && "getShadowValue: Original must not be null");
    auto It = map_.find(Original);
    return (It != map_.end()) ? It->second : nullptr;
  }

  bool hasShadowValue(llvm::Value *Original) const {
    assert(Original != nullptr && "hasShadowValue: Original must not be null");
    return map_.count(Original) != 0;
  }

  void eraseShadowValue(llvm::Value *Original) {
    assert(Original != nullptr && "eraseShadowValue: Original must not be null");
    map_.erase(Original);
  }

  void clear() { map_.clear(); }

  std::size_t size() const { return map_.size(); }

private:
  llvm::DenseMap<llvm::Value *, llvm::Value *> map_;
};

#endif // SHADOW_VALUE_MAP_H