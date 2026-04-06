// NSanPass.h - Header for NSan LLVM Pass
//
// Declares the NSanPass class, which is an LLVM FunctionPass
// that instruments floating-point operations with shadow
// computations for numerical stability checking.

#ifndef NSAN_PASS_H
#define NSAN_PASS_H

#include "ShadowValueMap.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"

using namespace llvm;

class NSanPass : public FunctionPass {
public:
  static char ID;
  NSanPass() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override;

private:
  ShadowValueMap shadow_map_;

  // Detection
  bool isFloatingPointValue(Value *V);

  // Shadow creation
  Value *getShadowValue(Value *V);
  Value *createShadowValue(Value *V, IRBuilder<> &B);
  Type *getShadowType(Type *OrigType);

  // Instrumentation
  void instrumentBinaryOp(BinaryOperator *Op);
  void instrumentCastOp(CastInst *Cast);
  void instrumentLoadStore(Instruction *I);
  void instrumentFunctionCall(CallInst *Call);
  void instrumentReturnValue(ReturnInst *Ret);
  void instrumentFunctionEntry(Function &F);
};

#endif // NSAN_PASS_H
