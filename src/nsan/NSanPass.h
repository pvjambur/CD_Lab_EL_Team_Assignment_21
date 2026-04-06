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
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"

class NSanPass : public llvm::FunctionPass {
public:
  static char ID;
  NSanPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &F) override;

private:
  ShadowValueMap shadow_map_;

  // Detection
  bool isFloatingPointValue(llvm::Value *V);

  // Shadow creation
  llvm::Value *getShadowValue(llvm::Value *V);
  llvm::Value *createShadowValue(llvm::Value *V, llvm::IRBuilder<> &B);
  llvm::Type *getShadowType(llvm::Type *OrigType);

  // Instrumentation
  void instrumentBinaryOp(llvm::BinaryOperator *Op);
  void instrumentCastOp(llvm::CastInst *Cast);
  void instrumentLoadStore(llvm::Instruction *I);
  void instrumentFunctionCall(llvm::CallInst *Call);
  void instrumentReturnValue(llvm::ReturnInst *Ret);
  void instrumentFunctionEntry(llvm::Function &F);
};

#endif // NSAN_PASS_H
