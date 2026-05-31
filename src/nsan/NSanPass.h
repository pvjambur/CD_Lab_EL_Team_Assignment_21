#ifndef NSAN_PASS_H
#define NSAN_PASS_H

#include "ShadowValueMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

// Forward declarations — avoids pulling in heavy headers 
namespace llvm {
  class BinaryOperator;
  class CallInst;
  class CastInst;
  class ReturnInst;
  class AnalysisUsage;
  class Type;
  class Value;
}

class NSanPass : public llvm::PassInfoMixin<NSanPass> {
public:
  explicit NSanPass() = default;

  // New Pass Manager entry point
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

private:
  ShadowValueMap shadow_map_;

  bool         isFloatingPointValue(llvm::Value *V);
  llvm::Type  *getShadowType(llvm::Type *OrigType);
  llvm::Value *getShadowValue(llvm::Value *V);
  llvm::Value *createShadowValue(llvm::Value *V, llvm::IRBuilder<> &B);

  void instrumentBinaryOp(llvm::BinaryOperator *Op);
  void instrumentCastOp(llvm::CastInst *Cast);
  void instrumentLoadStore(llvm::Instruction *I);
  void instrumentFunctionCall(llvm::CallInst *Call);
  void instrumentReturnValue(llvm::ReturnInst *Ret);
  void instrumentFunctionEntry(llvm::Function &F);
};

#endif // NSAN_PASS_H