#include "Instrument.h"

using namespace llvm;

namespace instrument {

static const char *SanitizerFunctionName = "__sanitize__";
static const char *CoverageFunctionName = "__coverage__";

/*
 * Implement divide-by-zero sanitizer.
 */
void instrumentSanitize(Module *M, Instruction &I) {
  DebugLoc Debug = NULL;
  Value* Line = NULL;
  Value* Col = NULL;

  LLVMContext &Ctx = M->getContext();

  if (I.getDebugLoc()) {
    Debug = I.getDebugLoc();
    Line = llvm::ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getLine());
    Col = llvm::ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getCol());
  } 
  Value* Sanitizer = M->getOrInsertFunction(SanitizerFunctionName, Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx));
  Function* sanitize = cast<Function>(Sanitizer);

  CallInst *Call = CallInst::Create(sanitize, {I.getOperand(1), Line, Col}, "", &I);
  Call->setCallingConv(CallingConv::C);
  Call->setTailCall(true);
}

/*
 * Implement code coverage instrumentation.
 */
void instrumentCoverage(Module *M, Instruction &I) {
  DebugLoc Debug = I.getDebugLoc();
  Value* Line = NULL;
  Value* Col = NULL;

  LLVMContext &Ctx = M->getContext();

  // Line = llvm::ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getLine());
  // Col = llvm::ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getCol());

  if (Debug) {
    //Debug = I.getDebugLoc();
    Line = llvm::ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getLine());
    Col = llvm::ConstantInt::get(Type::getInt32Ty(Ctx), Debug.getCol());
    Value* Coverage = M->getOrInsertFunction(CoverageFunctionName, Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx));
    Function* coverage = cast<Function>(Coverage);

    CallInst *Call = CallInst::Create(coverage, {Line, Col}, "", &I);
    Call->setCallingConv(CallingConv::C);
    Call->setTailCall(true);
  }

}

bool Instrument::runOnFunction(Function &F) {
  for (BasicBlock &Bblock : F) {
    for (Instruction &Instr : Bblock) {
      instrumentCoverage(F.getParent(), Instr);
      int opCode = Instr.getOpcode();
      if (opCode == Instruction::SDiv || opCode == Instruction::UDiv) {
        instrumentSanitize(F.getParent(), Instr);
      }
    }
  }
  return true;
}

char Instrument::ID = 1;
static RegisterPass<Instrument>
    X("Instrument", "Instrumentations for Dynamic Analysis", false, false);

} // namespace instrument
