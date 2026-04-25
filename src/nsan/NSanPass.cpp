// NSanPass.cpp - Main LLVM Instrumentation Pass for NSan
//
// Stub implementation — all methods declared, runOnFunction() is a
// no-op placeholder. Real instrumentation implemented in later phases.
// See: private_config/INSTRUCTIONS.md for implementation details.

#include "NSanPass.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

char NSanPass::ID = 0;
static RegisterPass<NSanPass> X("nsan", "Numerical Sanitizer Pass");

// ── Core pass entry point 
// This is what LLVM calls for every function in the module being compiled.
// Phase 1 stub: iterate functions, detect FP ops, log — no transformation yet.
bool NSanPass::runOnFunction(Function &F) {
  // TODO Phase 1: iterate over F's basic blocks and instructions,
  // call instrumentBinaryOp() on fadd/fsub/fmul/fdiv instructions.
  // Return true if the IR was modified (false = read-only pass for now).
  return false;
}

// ── Detection 
bool NSanPass::isFloatingPointValue(Value *V) {
  // TODO Phase 1: return V->getType()->isFloatingPointTy()
  return false;
}

// ── Shadow type mapping 
Type *NSanPass::getShadowType(Type *OrigType) {
  // TODO Phase 2: float -> double, double -> fp128
  // e.g. if (OrigType->isFloatTy()) return Type::getDoubleTy(OrigType->getContext());
  return nullptr;
}

// ── Shadow value creation 
Value *NSanPass::getShadowValue(Value *V) {
  return shadow_map_.getShadowValue(V);
}

Value *NSanPass::createShadowValue(Value *V, IRBuilder<> &B) {
  // TODO Phase 2: B.CreateFPExt(V, getShadowType(V->getType()), "shadow")
  return nullptr;
}

// ── Instrumentation stubs 
void NSanPass::instrumentBinaryOp(BinaryOperator *Op) {
  // TODO Phase 2: create shadow ops + insert consistency check call
}

void NSanPass::instrumentCastOp(CastInst *Cast) {
  // TODO Phase 3
}

void NSanPass::instrumentLoadStore(Instruction *I) {
  // TODO Phase 3
}

void NSanPass::instrumentFunctionCall(CallInst *Call) {
  // TODO Phase 3
}

void NSanPass::instrumentReturnValue(ReturnInst *Ret) {
  // TODO Phase 3
}

void NSanPass::instrumentFunctionEntry(Function &F) {
  // TODO Phase 3
}