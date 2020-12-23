#include "AbstractSyntaxTree.h"
#include "Driver.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Verifier.h>
#include <iostream>

unique_ptr<ExprAST> LogError(const string& Str) {
  std::cerr << Str << std::endl;
  return nullptr;
}

[[maybe_unused]] unique_ptr<FunctionAST> LogErrorF(const string& Str) {
  std::cerr << Str << std::endl;
  return nullptr;
}

unique_ptr<PrototypeAST> LogErrorP(const string& Str) {
  std::cerr << Str << std::endl;
  return nullptr;
}

Value *LogErrorV(const string& Str) {
  LogError(Str);
  return nullptr;
}

Value *NumberExprAST::codegen(Driver& TheDriver,
                              LLVMContext& TheContext,
                              IRBuilder<>& Builder,
                              Module& TheModule,
                              map<string, Value*>& NamedValues) {
  using llvm::ConstantFP;
  using llvm::APFloat;
  return ConstantFP::get(TheContext, APFloat(mValue));
}

Value *VariableExprAST::codegen(Driver& TheDriver,
                                LLVMContext& TheContext,
                                IRBuilder<>& Builder,
                                Module& TheModule,
                                map<string, Value*>& NamedValues) {
  Value *V = NamedValues[mName];
  if (!V)
    LogErrorV("Unknown variable name");
  return V;
}

Value *BinaryExprAST::codegen(Driver& TheDriver,
                              LLVMContext& TheContext,
                              IRBuilder<>& Builder,
                              Module& TheModule,
                              map<string, Value*>& NamedValues) {
  Value *L = mLHS->codegen(TheDriver, TheContext, Builder, TheModule, NamedValues);
  Value *R = mRHS->codegen(TheDriver, TheContext, Builder, TheModule, NamedValues);
  if (!L || !R)
    return nullptr;
  if (mOperator == "+") {
    return Builder.CreateFAdd(L, R, "addtmp");
  } else if (mOperator == "-") {
    return Builder.CreateFSub(L, R, "subtmp");
  } else if (mOperator == "*") {
    return Builder.CreateFMul(L, R, "multmp");
  } else if (mOperator == "/") {
    return Builder.CreateFDiv(L, R, "divtmp");
  } else if (mOperator == "^") {
    Function *CallPow = TheDriver.getFunction("pow");
    if (!CallPow)
      return LogErrorV("unknown function referenced");
    if (CallPow->arg_size() != 2) {
      std::cerr << "Should be " << CallPow->arg_size() << " arguments\n";
      return LogErrorV("incorrect # arguments passed");
    }
    vector<Value *> ArgsV;
    ArgsV.push_back(L);
    ArgsV.push_back(R);
    return Builder.CreateCall(CallPow, ArgsV, "powtmp");
  } else {
    return LogErrorV("invalid binary operator");
  }
}

Value *CallExprAST::codegen(Driver& TheDriver,
                            LLVMContext& TheContext,
                            IRBuilder<>& Builder,
                            Module& TheModule,
                            map<string, Value*>& NamedValues) {
  // Look up the name in the global module table.
  Function *CalleeF = TheDriver.getFunction(mCallee);
  if (!CalleeF)
    return LogErrorV("unknown function referenced");
  // If argument mismatch error.
  if (CalleeF->arg_size() != mArguments.size())
    return LogErrorV("incorrect # arguments passed");
  vector<Value *> ArgsV;
  for (unsigned i = 0, e = mArguments.size(); i != e; ++i) {
    ArgsV.push_back(mArguments[i]->codegen(TheDriver, TheContext, Builder, TheModule, NamedValues));
    if (!ArgsV.back())
      return nullptr;
  }
  return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

string PrototypeAST::getName() const {
  return this->mName;
}

Function *PrototypeAST::codegen(Driver& TheDriver,
                                LLVMContext& TheContext,
                                IRBuilder<>& Builder,
                                Module& TheModule,
                                map<string, Value*>& NamedValues) {
  using llvm::Type;
  using llvm::FunctionType;
  using llvm::Function;
  // Make the function type:  double(double,double) etc.
  vector<Type*> Doubles(mArguments.size(),
                        Type::getDoubleTy(TheContext));
  // Create a new function type:
  // This type takes "N" double types, and return an LLVM Double type.
  // The last false parameter indicates this is not variadic argument.
  FunctionType *FT =
    FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
  // Actually create the function
  Function *F = 
    Function::Create(FT, Function::ExternalLinkage, mName, &TheModule);
  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args()) {
    Arg.setName(mArguments[Idx++]);
  }
  return F;
}

Function *FunctionAST::codegen(Driver& TheDriver,
                               LLVMContext& TheContext,
                               IRBuilder<>& Builder,
                               Module& TheModule,
                               FunctionPassManager& FPM,
                               map<string, Value*>& NamedValues) {
  using llvm::Function;
  using llvm::BasicBlock;
  auto &P = *mPrototype;
  TheDriver.mFunctionProtos[mPrototype->getName()] = mPrototype->clone();
  // First, check for an existing function from a previous 'extern' declaration.
  Function *TheFunction = TheDriver.getFunction(P.getName());
  if (!TheFunction)
    return nullptr;
  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
  Builder.SetInsertPoint(BB);
  // Record the function arguments in the NamedValues map.
  // TODO: this is a global map. Why do we clear it entirely?
  NamedValues.clear();
  for (auto &Arg : TheFunction->args())
    NamedValues[std::string(Arg.getName())] = &Arg;
  if (Value *RetVal = mBody->codegen(TheDriver, TheContext, Builder, TheModule, NamedValues)) {
    // Finish off the function.
    Builder.CreateRet(RetVal);
    // Validate the generated code, checking for consistency.
    llvm::verifyFunction(*TheFunction);
    // Run the optimizer on the function.
    FPM.run(*TheFunction);
    return TheFunction;
  }
  // Error reading body, remove function.
  TheFunction->eraseFromParent();
  return nullptr;
}

unique_ptr<ExprAST> NumberExprAST::clone() const {
  return make_unique<NumberExprAST>(mValue);
}

unique_ptr<ExprAST> VariableExprAST::clone() const {
  return make_unique<VariableExprAST>(mName);
}

unique_ptr<ExprAST> BinaryExprAST::clone() const {
  return make_unique<BinaryExprAST>(mOperator, mLHS->clone(), mRHS->clone());
}

unique_ptr<ExprAST> CallExprAST::clone() const {
  vector<unique_ptr<ExprAST>> NewArgs;
  for (const auto& i : mArguments) {
    NewArgs.emplace_back(i->clone());
  }
  return make_unique<CallExprAST>(mCallee, move(NewArgs));
}

unique_ptr<PrototypeAST> PrototypeAST::clone() const {
  return make_unique<PrototypeAST>(mName, mArguments);
}

unique_ptr<FunctionAST> FunctionAST::clone() const {
  return make_unique<FunctionAST>(mPrototype->clone(), mBody->clone());
}

unique_ptr<ExprAST> ExprAST::Derivative(Driver& TheDriver, const string& Variable) const {
  return nullptr;
}

unique_ptr<ExprAST> NumberExprAST::Derivative(Driver& TheDriver, const string& Variable) const {
  return make_unique<NumberExprAST>(0.0);
}

unique_ptr<ExprAST> VariableExprAST::Derivative(Driver& TheDriver, const string& Variable) const {
  if (Variable == mName) {
    return make_unique<NumberExprAST>(1.0);
  } else {
    return make_unique<NumberExprAST>(0.0);
  }
}

unique_ptr<ExprAST> BinaryExprAST::Derivative(Driver& TheDriver, const string& Variable) const {
  auto LHSDeriv = mLHS->clone()->Derivative(TheDriver, Variable);
  auto RHSDeriv = mRHS->clone()->Derivative(TheDriver, Variable);
  if (mOperator == "+" || mOperator == "-") {
    // Derivative of "f(x) + g(x)" or "f(x) - g(x)"
    // = "f'(x) + g'(x)" or "f'(x) - g'(x)"
    return make_unique<BinaryExprAST>(mOperator, move(LHSDeriv), move(RHSDeriv));
  } else if (mOperator == "*") {
    // Derivative of "f(x) * g(x)"
    // = "f'(x) * g(x) + g'(x) * f(x)"
    auto NewLHS = make_unique<BinaryExprAST>("*", move(LHSDeriv), mRHS->clone());
    auto NewRHS = make_unique<BinaryExprAST>("*", move(RHSDeriv), mLHS->clone());
    return make_unique<BinaryExprAST>("+", move(NewLHS), move(NewRHS));
  } else if (mOperator == "/") {
    // Derivative of "f(x) / g(x)"
    // = "(f'(x) * g(x) - g'(x) * f(x)) / (g(x) * g(x))"
    auto NumeratorLHS = make_unique<BinaryExprAST>("*", move(LHSDeriv), mRHS->clone());
    auto NumeratorRHS = make_unique<BinaryExprAST>("*", move(RHSDeriv), mLHS->clone());
    auto Numerator = make_unique<BinaryExprAST>("-", move(NumeratorLHS), move(NumeratorRHS));
    auto Denominator = make_unique<BinaryExprAST>("*", mRHS->clone(), mRHS->clone());
    return make_unique<BinaryExprAST>("/", move(Numerator), move(Denominator));
  } else if (mOperator == "^") {
    // Derivative of "f(x) ^ g(x)"
    // let y = f(x) ^ g(x), then ln(y) = g(x) * ln(f(x))
    // y'/y = g'(x) * ln(f(x)) + g(x) * (1/f(x)) * f'(x)
    // y' = g'(x) * ln(f(x)) * (f(x) ^ g(x)) + g(x) * (1/f(x)) * f'(x) * (f(x) ^ g(x))
    // TODO: this requires the std::log function to be registered
    vector<unique_ptr<ExprAST>> Args;
    Args.push_back(mLHS->clone());
    auto LogLHS = make_unique<CallExprAST>("log", move(Args));
    auto NewLHS = make_unique<BinaryExprAST>("*", move(RHSDeriv), move(LogLHS));
    NewLHS = make_unique<BinaryExprAST>("*", move(NewLHS), this->clone());
    auto NewRHS = make_unique<BinaryExprAST>("*", move(LHSDeriv), mRHS->clone());
    auto TmpRHSRightFactor = make_unique<BinaryExprAST>("/", make_unique<NumberExprAST>(1.0), mLHS->clone());
    NewRHS = make_unique<BinaryExprAST>("*", move(NewRHS), move(TmpRHSRightFactor));
    NewRHS = make_unique<BinaryExprAST>("*", move(NewRHS), this->clone());
    return make_unique<BinaryExprAST>(mOperator, move(NewLHS), move(NewRHS));
  } else {
    return nullptr;
  }
}

unique_ptr<ExprAST> CallExprAST::Derivative(Driver& TheDriver, const string& Variable) const {
  // crazy code!
  vector<unique_ptr<ExprAST>> mArgsDerivative;
  for (const auto& i : mArguments) {
    mArgsDerivative.push_back(i->Derivative(TheDriver, Variable));
  }
  // find the function from the proto map
  auto FI = TheDriver.mFunctionProtos.find(mCallee);
  if (FI != TheDriver.mFunctionProtos.end()) {
    const vector<string> ArgNames = FI->second->getArgumentNames();
    vector<unique_ptr<CallExprAST>> DerivativeCalls;
    if (ArgNames.size() == mArguments.size()) {
      for (size_t i = 0; i < ArgNames.size(); ++i) {
        const string DerivativeFuncName = "d" + mCallee + "_d" + ArgNames[i];
        auto dFI = TheDriver.mDerivativeFunctions.find(DerivativeFuncName);
        if (dFI != TheDriver.mDerivativeFunctions.end()) {
          vector<unique_ptr<ExprAST>> ArgumentsClone;
          for (size_t j = 0; j < mArguments.size(); ++j) {
            ArgumentsClone.push_back(mArguments[j]->clone());
          }
          DerivativeCalls.push_back(make_unique<CallExprAST>(DerivativeFuncName, move(ArgumentsClone)));
        } else {
          std::cerr << "Function " << DerivativeFuncName << " not found!\n";
        }
      }
      // multiply DerivativeCalls and mArgsDerivative
      if (ArgNames.size() > 0) {
        auto LHS = make_unique<BinaryExprAST>("*", move(DerivativeCalls[0]), move(mArgsDerivative[0]));
        for (size_t i = 1; i < ArgNames.size(); ++i) {
          auto RHS = make_unique<BinaryExprAST>("*", move(DerivativeCalls[i]), move(mArgsDerivative[i]));
          LHS = make_unique<BinaryExprAST>("+", move(LHS), move(RHS));
        }
        return move(LHS);
      }
    } else {
      std::cerr << "Function args mismatch!\n";
    }
  } else {
    std::cerr << "The function call " << mCallee << " not found!\n";
  }
  return make_unique<NumberExprAST>(0.0);
}

unique_ptr<FunctionAST> FunctionAST::Derivative(Driver& TheDriver, 
                                                const string& Variable,
                                                const string& FunctionName) const {
  auto Derivative = mBody->Derivative(TheDriver, Variable);
  auto DerivativePrototype = make_unique<PrototypeAST>(FunctionName, mPrototype->getArgumentNames());
  return make_unique<FunctionAST>(move(DerivativePrototype), move(Derivative));
}

unique_ptr<ExprAST> IfExprAST::clone() const {
  return make_unique<IfExprAST>(mCond->clone(), mThen->clone(), mElse->clone());
}

unique_ptr<ExprAST> IfExprAST::Derivative(Driver& TheDriver, const string &Variable) const {
  auto DerivativeThen = mThen->Derivative(TheDriver, Variable);
  auto DerivativeElse = mElse->Derivative(TheDriver, Variable);
  return make_unique<IfExprAST>(mCond->clone(), move(DerivativeThen), move(DerivativeElse));
}

Value *IfExprAST::codegen(Driver &TheDriver, LLVMContext &TheContext, IRBuilder<> &Builder, Module &TheModule,
                          map<string, Value *> &NamedValues) {
  using llvm::PHINode;
  using llvm::BasicBlock;
  using llvm::ConstantFP;
  using llvm::APFloat;
  Value *CondV = mCond->codegen(TheDriver, TheContext, Builder, TheModule, NamedValues);
  if (!CondV)
    return nullptr;
  // Convert condition to a bool by comparing non-equal to 0.0.
  CondV = Builder.CreateFCmpONE(CondV, ConstantFP::get(TheContext, APFloat(0.0)), "ifcond");
  // Get the current function object
  Function *TheFunction = Builder.GetInsertBlock()->getParent();
  // Create blocks for the then and else cases.  Insert the 'then' block at the
  // end of TheFunction.
  BasicBlock *ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
  BasicBlock *ElseBB = BasicBlock::Create(TheContext, "else");
  BasicBlock *MergeBB = BasicBlock::Create(TheContext, "ifcont");
  Builder.CreateCondBr(CondV, ThenBB, ElseBB);
  // change the insert point to the end of ThenBB
  Builder.SetInsertPoint(ThenBB);
  Value *ThenV = mThen->codegen(TheDriver, TheContext, Builder, TheModule, NamedValues);
  if (!ThenV)
    return nullptr;
  // Create an unconditional branch to the merge block
  Builder.CreateBr(MergeBB);
  // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
  ThenBB = Builder.GetInsertBlock();
  // Emit the else block.
  TheFunction->getBasicBlockList().push_back(ElseBB);
  Builder.SetInsertPoint(ElseBB);
  Value *ElseV = mElse->codegen(TheDriver, TheContext, Builder, TheModule, NamedValues);
  if (!ElseV)
    return nullptr;
  Builder.CreateBr(MergeBB);
  // codegen of 'Else' can change the current block, update ElseBB for the PHI.
  ElseBB = Builder.GetInsertBlock();
  // Emit merge block.
  TheFunction->getBasicBlockList().push_back(MergeBB);
  Builder.SetInsertPoint(MergeBB);
  PHINode *PN = Builder.CreatePHI(llvm::Type::getDoubleTy(TheContext), 2, "iftmp");
  PN->addIncoming(ThenV, ThenBB);
  PN->addIncoming(ElseV, ElseBB);
  return PN;
}
