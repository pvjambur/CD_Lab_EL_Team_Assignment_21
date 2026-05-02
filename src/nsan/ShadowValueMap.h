// ShadowValueMap.h - Shadow Value Tracking for NSan
//
// Maintains a bijective map from every original LLVM IR Value* to its
// higher-precision shadow counterpart, as described in:
//   Courbet, "NSan: A Floating-Point Numerical Sanitizer", CC '21
//   https://doi.org/10.1145/3446804.3446848
//
// ── Lifetime model (§3.2) ─────────────────────────────────────────────────────
//
//   Temporary values  — shadow lives as long as the IR instruction.
//                       Registered by instrumentBinaryOp / instrumentCastOp
//                       immediately after emitting the shadow instruction.
//
//   Parameter values  — shadow registered on function entry by
//                       instrumentFunctionEntry(); looked up by any
//                       subsequent use in the function body.
//
//   Return values     — shadow is written to the thread-local tagged return
//                       slot by instrumentReturnValue(); not stored here.
//
//   Memory values     — shadow lives in the parallel shadow address space
//                       managed by the NSan runtime (__nsan_shadow_ptr_*).
//                       Only the IR Value* produced by a LoadInst is stored
//                       here; the shadow pointer itself lives in shadow memory.
//
// ── Container choice ──────────────────────────────────────────────────────────
//
//   llvm::DenseMap<Value*, Value*> is used instead of std::map<> for three
//   reasons:
//     1. O(1) amortised lookup/insert vs O(log n) for std::map.
//     2. Open-addressing layout keeps hot entries in cache — important when
//        iterating over all instructions in a large function body.
//     3. DenseMap uses LLVM's pointer hashing (DenseMapInfo<T*>) which is
//        optimised for the aligned pointer values that LLVM IR Values always
//        have.
//
// ── Thread safety ─────────────────────────────────────────────────────────────
//
//   One ShadowValueMap instance is owned by NSanPass and cleared between
//   functions (see NSanPass::runOnFunction).  No two threads ever touch the
//   same instance, so no synchronisation is required.

#ifndef SHADOW_VALUE_MAP_H
#define SHADOW_VALUE_MAP_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"   // isa<>, cast<>

#include <cassert>
#include <cstddef>                  // std::size_t

namespace llvm { class VectorType; }

class ShadowValueMap {
public:
  ShadowValueMap()  = default;
  ~ShadowValueMap() = default;

  // Not copyable — each Function owns exactly one map; copying would silently
  // alias the shadow Value* pointers across two instrumentation contexts.
  ShadowValueMap(const ShadowValueMap &)            = delete;
  ShadowValueMap &operator=(const ShadowValueMap &) = delete;

  // Movable — allows NSanPass to hold the map by value and avoids an extra
  // heap allocation compared to holding it through a unique_ptr.
  ShadowValueMap(ShadowValueMap &&)            = default;
  ShadowValueMap &operator=(ShadowValueMap &&) = default;

  // ── Write ──────────────────────────────────────────────────────────────────

  /// Register (or overwrite) the shadow for an original IR value.
  ///
  /// "Overwrite" semantics are intentional: Phase 3 load instrumentation may
  /// replace a Phase 2 fpext fallback with a more precise value loaded from
  /// shadow memory (§3.2.3 — "resume computations by re-extending the
  /// original value").
  ///
  /// Called by:
  ///   createShadowValue()       — fpext of a constant or bare argument
  ///   instrumentBinaryOp()      — after emitting shadow fadd/fsub/fmul/fdiv
  ///   instrumentCastOp()        — after emitting shadow fpext/fptrunc
  ///   instrumentLoadStore()     — after emitting the shadow-load select
  ///   instrumentFunctionCall()  — after reading the shadow return slot
  ///   instrumentFunctionEntry() — after loading shadow stack arguments
  void setShadowValue(llvm::Value *Original, llvm::Value *Shadow) {
    assert(Original != nullptr && "setShadowValue: Original must not be null");
    assert(Shadow   != nullptr && "setShadowValue: Shadow must not be null");

    // ── Debug-build precision check ──────────────────────────────────────────
    // The shadow type must be strictly wider than the original type (§3.1):
    //   float    ->  double    (32 -> 64 bit mantissa)
    //   double   ->  fp128     (64 -> 113 bit mantissa)
    //   x86_fp80 ->  fp128     (64 -> 113 bit mantissa, chosen by the paper)
    // Vectors are checked element-wise; lane count must be preserved.
    // We assert in debug builds so type mismatches surface at the call site
    // rather than producing cryptic type errors inside a later IR pass.
#ifndef NDEBUG
    llvm::Type *OT = Original->getType();
    llvm::Type *ST = Shadow->getType();

    // Unwrap vectors to compare element types; also verify lane count matches.
    if (auto *OVT = llvm::dyn_cast<llvm::VectorType>(OT)) {
      auto *SVT = llvm::dyn_cast<llvm::VectorType>(ST);
      assert(SVT && "setShadowValue: original is a vector but shadow is not");
      assert(OVT->getElementCount() == SVT->getElementCount() &&
             "setShadowValue: vector lane count mismatch between "
             "original and shadow");
      OT = OVT->getElementType();
      ST = SVT->getElementType();
    }

    assert(OT->isFloatingPointTy() &&
           "setShadowValue: Original must have a floating-point type");
    assert(ST->isFloatingPointTy() &&
           "setShadowValue: Shadow must have a floating-point type");
    assert(ST->getFPMantissaWidth() > OT->getFPMantissaWidth() &&
           "setShadowValue: shadow precision must be strictly greater than "
           "original precision (float->double or double/x86_fp80->fp128)");
#endif

    map_[Original] = Shadow;
  }


  /// Return the shadow for Original, or nullptr if not yet registered.
  ///
  /// A nullptr return is the signal for the instrument* methods to fall back
  /// to createShadowValue() (a fresh fpext of the original), matching the
  /// paper's rule that every FP value always has a valid shadow — at worst
  /// S(v) = fpext(v).
  llvm::Value *getShadowValue(llvm::Value *Original) const {
    assert(Original != nullptr && "getShadowValue: Original must not be null");
    auto It = map_.find(Original);
    return (It != map_.end()) ? It->second : nullptr;
  }

  /// Return true iff Original already has a registered shadow.
  ///
  /// Prefer this over (getShadowValue(V) != nullptr) when the caller wants
  /// to branch without touching the shadow Value* itself.
  bool hasShadowValue(llvm::Value *Original) const {
    assert(Original != nullptr && "hasShadowValue: Original must not be null");
    return map_.count(Original) != 0;
  }

  // ── Lifecycle ──────────────────────────────────────────────────────────────

  /// Remove the shadow entry for Original.
  ///
  /// Called when the original instruction is erased from the IR during a
  /// cleanup pass so the map does not hold a dangling Value* key.
  /// Safe to call even when Original has no registered shadow (no-op).
  void eraseShadowValue(llvm::Value *Original) {
    assert(Original != nullptr &&
           "eraseShadowValue: Original must not be null");
    map_.erase(Original);
  }

  /// Drop all entries.
  ///
  /// Called by NSanPass::runOnFunction at the start of each new function so
  /// that Value* pointers from the previous function body cannot leak into
  /// the current one.  Clearing rather than destroying and re-constructing
  /// reuses the DenseMap's heap allocation, which matters when compiling
  /// large translation units with many small functions.
  void clear() { map_.clear(); }

  /// Number of currently tracked shadow pairs.
  /// Useful for unit tests and -debug-pass dumps.
  std::size_t size() const { return map_.size(); }

private:
  // DenseMap<Value*, Value*>:
  //   Key   — original IR Value* (always non-null; asserted above)
  //   Value — shadow IR Value*   (always non-null; asserted above)
  //
  // Default DenseMapInfo<T*> uses the pointer value as the hash key,
  // which is correct here because LLVM guarantees that two distinct IR
  // Values never share an address within the same LLVMContext.
  llvm::DenseMap<llvm::Value *, llvm::Value *> map_;
};

#endif // SHADOW_VALUE_MAP_H