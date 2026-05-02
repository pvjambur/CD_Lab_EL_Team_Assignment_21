#include "NSanPass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/PassPlugin.h"

using namespace llvm;

bool NSanPass::isFloatingPointValue(Value *V) {
  Type *T = V->getType();
  if (T->isFloatingPointTy() || T->isFloatingPointTy()) return true; // Fixed ambiguity
  if (T->isVectorTy())
    return cast<VectorType>(T)->getElementType()->isFloatingPointTy();
  return false;
}

Type *NSanPass::getShadowType(Type *OrigType) {
  LLVMContext &Ctx = OrigType->getContext();
  if (OrigType->isFloatTy())    return Type::getDoubleTy(Ctx);
  if (OrigType->isDoubleTy() || OrigType->isX86_FP80Ty())
    return Type::getFP128Ty(Ctx);
  if (auto *VT = dyn_cast<VectorType>(OrigType)) {
    Type *ShadowElem = getShadowType(VT->getElementType());
    if (ShadowElem)
      return VectorType::get(ShadowElem, VT->getElementCount());
  }
  return nullptr;
}

Value *NSanPass::getShadowValue(Value *V) {
  return shadow_map_.getShadowValue(V);
}

Value *NSanPass::createShadowValue(Value *V, IRBuilder<> &B) {
  Type *ShadowTy = getShadowType(V->getType());
  if (!ShadowTy) return nullptr;
  Value *Shadow = B.CreateFPExt(V, ShadowTy, V->getName() + ".shadow");
  shadow_map_.setShadowValue(V, Shadow);
  return Shadow;
}

void NSanPass::instrumentBinaryOp(BinaryOperator *Op) {
  IRBuilder<> B(Op->getNextNode());
  Value *ShadowLHS = getShadowValue(Op->getOperand(0));
  Value *ShadowRHS = getShadowValue(Op->getOperand(1));

  if (!ShadowLHS) ShadowLHS = createShadowValue(Op->getOperand(0), B);
  if (!ShadowRHS) ShadowRHS = createShadowValue(Op->getOperand(1), B);
  if (!ShadowLHS || !ShadowRHS) return;

  Value *ShadowResult = nullptr;
  switch (Op->getOpcode()) {
    case Instruction::FAdd:
      ShadowResult = B.CreateFAdd(ShadowLHS, ShadowRHS, Op->getName() + ".s");
      break;
    case Instruction::FSub:
      ShadowResult = B.CreateFSub(ShadowLHS, ShadowRHS, Op->getName() + ".s");
      break;
    case Instruction::FMul:
      ShadowResult = B.CreateFMul(ShadowLHS, ShadowRHS, Op->getName() + ".s");
      break;
    case Instruction::FDiv:
      ShadowResult = B.CreateFDiv(ShadowLHS, ShadowRHS, Op->getName() + ".s");
      break;
    default:
      return;
  }
  shadow_map_.setShadowValue(Op, ShadowResult);
}

void NSanPass::instrumentCastOp(CastInst *Cast) {
  unsigned Opc = Cast->getOpcode();
  if (Opc != Instruction::FPExt && Opc != Instruction::FPTrunc)
    return;

  IRBuilder<> B(Cast->getNextNode());
  Value *ShadowSrc = getShadowValue(Cast->getOperand(0));
  if (!ShadowSrc) ShadowSrc = createShadowValue(Cast->getOperand(0), B);
  if (!ShadowSrc) return;

  Type *ShadowDestTy = getShadowType(Cast->getDestTy());
  if (!ShadowDestTy) return;

  Value *ShadowResult = nullptr;
  if (Opc == Instruction::FPExt)
    ShadowResult = B.CreateFPExt(ShadowSrc, ShadowDestTy,
                                 Cast->getName() + ".s");
  else
    ShadowResult = B.CreateFPTrunc(ShadowSrc, ShadowDestTy,
                                   Cast->getName() + ".s");

  shadow_map_.setShadowValue(Cast, ShadowResult);
}

void NSanPass::instrumentLoadStore(Instruction *I) {
  LLVMContext &Ctx = I->getContext();
  Module      *M   = I->getModule();

  if (auto *Load = dyn_cast<LoadInst>(I)) {
    Type *OrigTy   = Load->getType();
    Type *ShadowTy = getShadowType(OrigTy);
    if (!ShadowTy) return;

    IRBuilder<> B(Load);
    FunctionCallee ShadowPtrFn = M->getOrInsertFunction(
        "__nsan_shadow_ptr_load",
        PointerType::getUnqual(Ctx),
        Load->getPointerOperand()->getType());
    Value *ShadowPtr = B.CreateCall(ShadowPtrFn,
                                    {Load->getPointerOperand()}, "shadow.ptr");

    IRBuilder<> B2(Load->getNextNode());
    Value *IsValid      = B2.CreateIsNotNull(ShadowPtr, "shadow.valid");
    Value *LoadedShadow = B2.CreateLoad(ShadowTy, ShadowPtr, "shadow.loaded");
    Value *ExtendedOrig = B2.CreateFPExt(Load, ShadowTy, "shadow.ext");
    Value *ShadowVal    = B2.CreateSelect(IsValid, LoadedShadow, ExtendedOrig,
                                          Load->getName() + ".s");
    shadow_map_.setShadowValue(Load, ShadowVal);

  } else if (auto *Store = dyn_cast<StoreInst>(I)) {
    Value *Val     = Store->getValueOperand();
    Type  *OrigTy  = Val->getType();
    Type  *ShadowTy = getShadowType(OrigTy);
    if (!ShadowTy) return;

    IRBuilder<> B(Store);
    Value *ShadowVal = getShadowValue(Val);
    if (!ShadowVal) ShadowVal = createShadowValue(Val, B);
    if (!ShadowVal) return;

    FunctionCallee CheckFn = M->getOrInsertFunction(
        "__nsan_check_consistency",
        Type::getVoidTy(Ctx), OrigTy, ShadowTy);
    B.CreateCall(CheckFn, {Val, ShadowVal});

    FunctionCallee ShadowStoreFn = M->getOrInsertFunction(
        "__nsan_shadow_ptr_store",
        PointerType::getUnqual(Ctx),
        Store->getPointerOperand()->getType());
    Value *ShadowPtr = B.CreateCall(ShadowStoreFn,
                                    {Store->getPointerOperand()}, "shadow.ptr");
    B.CreateStore(ShadowVal, ShadowPtr);
  }
}

void NSanPass::instrumentFunctionCall(CallInst *Call) {
  LLVMContext &Ctx = Call->getContext();
  Module      *M   = Call->getModule();
  IRBuilder<>  B(Call);

  FunctionCallee TagFn = M->getOrInsertFunction(
      "__nsan_set_shadow_stack_tag",
      Type::getVoidTy(Ctx), Type::getInt64Ty(Ctx));

  if (Function *Callee = Call->getCalledFunction()) {
    Value *FnAddr = B.CreatePtrToInt(Callee, Type::getInt64Ty(Ctx),
                                     "callee.addr");
    B.CreateCall(TagFn, {FnAddr});
  }

  for (unsigned i = 0, e = Call->arg_size(); i < e; ++i) {
    Value *Arg = Call->getArgOperand(i);
    if (!isFloatingPointValue(Arg)) continue;

    Value *ShadowArg = getShadowValue(Arg);
    if (!ShadowArg) ShadowArg = createShadowValue(Arg, B);
    if (!ShadowArg) continue;

    FunctionCallee PushFn = M->getOrInsertFunction(
        "__nsan_push_shadow_arg",
        Type::getVoidTy(Ctx),
        ShadowArg->getType(), Type::getInt32Ty(Ctx));
    B.CreateCall(PushFn,
                 {ShadowArg, ConstantInt::get(Type::getInt32Ty(Ctx), i)});
  }

  if (Call->getType()->isFloatingPointTy()) {
    IRBuilder<> B2(Call->getNextNode());
    Type *ShadowTy = getShadowType(Call->getType());
    if (!ShadowTy) return;

    FunctionCallee GetRetFn = M->getOrInsertFunction(
        "__nsan_get_shadow_return",
        ShadowTy, Type::getInt64Ty(Ctx));

    Value *FnAddr = Call->getCalledFunction()
        ? B2.CreatePtrToInt(Call->getCalledFunction(),
                            Type::getInt64Ty(Ctx), "callee.addr")
        : static_cast<Value *>(ConstantInt::get(Type::getInt64Ty(Ctx), 0));

    Value *ShadowFromSlot = B2.CreateCall(GetRetFn, {FnAddr},
                                          Call->getName() + ".ret.s");
    shadow_map_.setShadowValue(Call, ShadowFromSlot);
  }
}

void NSanPass::instrumentReturnValue(ReturnInst *Ret) {
  Value *RetVal = Ret->getReturnValue();
  if (!RetVal || !RetVal->getType()->isFloatingPointTy()) return;

  LLVMContext &Ctx = Ret->getContext();
  Module      *M   = Ret->getModule();
  IRBuilder<>  B(Ret);

  Value *ShadowRetVal = getShadowValue(RetVal);
  if (!ShadowRetVal) ShadowRetVal = createShadowValue(RetVal, B);
  if (!ShadowRetVal) return;

  Function *F      = Ret->getFunction();
  Value    *FnAddr = B.CreatePtrToInt(F, Type::getInt64Ty(Ctx), "fn.addr");

  FunctionCallee SetRetFn = M->getOrInsertFunction(
      "__nsan_set_shadow_return",
      Type::getVoidTy(Ctx), ShadowRetVal->getType(), Type::getInt64Ty(Ctx));
  B.CreateCall(SetRetFn, {ShadowRetVal, FnAddr});
}

void NSanPass::instrumentFunctionEntry(Function &F) {
  if (F.isDeclaration()) return;

  LLVMContext &Ctx      = F.getContext();
  Module      *M        = F.getParent();
  Instruction *InsertPt = &*F.getEntryBlock().getFirstInsertionPt();
  IRBuilder<>  B(InsertPt);

  Value *FnAddr = B.CreatePtrToInt(&F, Type::getInt64Ty(Ctx), "fn.addr");

  for (Argument &Arg : F.args()) {
    if (!isFloatingPointValue(&Arg)) continue;

    Type *ShadowTy = getShadowType(Arg.getType());
    if (!ShadowTy) continue;

    FunctionCallee LoadArgFn = M->getOrInsertFunction(
        "__nsan_load_shadow_arg",
        ShadowTy,
        Arg.getType(), Type::getInt64Ty(Ctx), Type::getInt32Ty(Ctx));
    unsigned Idx      = Arg.getArgNo();
    Value   *ShadowArg = B.CreateCall(
        LoadArgFn,
        {&Arg, FnAddr, ConstantInt::get(Type::getInt32Ty(Ctx), Idx)},
        Arg.getName() + ".s");
    shadow_map_.setShadowValue(&Arg, ShadowArg);
  }
}

PreservedAnalyses NSanPass::run(Function &F, FunctionAnalysisManager &AM) {
  shadow_map_.clear();
  instrumentFunctionEntry(F);

  bool Modified = false;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (auto *BinOp = dyn_cast<BinaryOperator>(&I)) {
        if (I.getType()->isFPOrFPVectorTy()) {
          instrumentBinaryOp(BinOp);
          Modified = true;
        }
      } else if (auto *Cast = dyn_cast<CastInst>(&I)) {
        unsigned Opc = Cast->getOpcode();
        if ((Opc == Instruction::FPExt || Opc == Instruction::FPTrunc) &&
            Cast->getSrcTy()->isFPOrFPVectorTy()) {
          instrumentCastOp(Cast);
          Modified = true;
        }
      } else if (isa<LoadInst>(&I) || isa<StoreInst>(&I)) {
        Type *AccessTy = isa<LoadInst>(&I)
            ? I.getType()
            : cast<StoreInst>(&I)->getValueOperand()->getType();
        if (AccessTy->isFPOrFPVectorTy()) {
          instrumentLoadStore(&I);
          Modified = true;
        }
      } else if (auto *Call = dyn_cast<CallInst>(&I)) {
        instrumentFunctionCall(Call);
        Modified = true;
      } else if (auto *Ret = dyn_cast<ReturnInst>(&I)) {
        instrumentReturnValue(Ret);
        Modified = true;
      }
    }
  }

  return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "nsan", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "nsan") {
                    FPM.addPass(NSanPass());
                    return true;
                  }
                  return false;
                });
          }};
}