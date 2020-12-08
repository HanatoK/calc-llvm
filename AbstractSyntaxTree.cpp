#include "AbstractSyntaxTree.h"
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

unique_ptr<FunctionAST> LogErrorF(const string& Str) {
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

Value *NumberExprAst::codegen(LLVMContext& TheContext,
                              IRBuilder<>& Builder,
                              Module& TheModule,
                              map<string, Value*>& NamedValues) {
  using llvm::ConstantFP;
  using llvm::APFloat;
  return ConstantFP::get(TheContext, APFloat(mValue));
}

Value *VariableExprAST::codegen(LLVMContext& TheContext,
                                IRBuilder<>& Builder,
                                Module& TheModule,
                                map<string, Value*>& NamedValues) {
  Value *V = NamedValues[mName];
  if (!V)
    LogErrorV("Unknown variable name");
  return V;
}

Value *BinaryExprAST::codegen(LLVMContext& TheContext,
                              IRBuilder<>& Builder,
                              Module& TheModule,
                              map<string, Value*>& NamedValues) {
  Value *L = mLHS->codegen(TheContext, Builder, TheModule, NamedValues);
  Value *R = mRHS->codegen(TheContext, Builder, TheModule, NamedValues);
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
    Function *CallPow = TheModule.getFunction("pow");
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

Value *CallExprAST::codegen(LLVMContext& TheContext,
                            IRBuilder<>& Builder,
                            Module& TheModule,
                            map<string, Value*>& NamedValues) {
  // Look up the name in the global module table.
  Function *CalleeF = TheModule.getFunction(mCallee);
  if (!CalleeF)
    return LogErrorV("unknown function referenced");
  // If argument mismatch error.
  if (CalleeF->arg_size() != mArguments.size())
    return LogErrorV("incorrect # arguments passed");
  vector<Value *> ArgsV;
  for (unsigned i = 0, e = mArguments.size(); i != e; ++i) {
    ArgsV.push_back(mArguments[i]->codegen(TheContext, Builder, TheModule, NamedValues));
    if (!ArgsV.back())
      return nullptr;
  }
  return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

string PrototypeAST::getName() const {
  return this->mName;
}

Function *PrototypeAST::codegen(LLVMContext& TheContext,
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

Function *FunctionAST::codegen(LLVMContext& TheContext,
                               IRBuilder<>& Builder,
                               Module& TheModule,
                               FunctionPassManager& FPM,
                               map<string, Value*>& NamedValues) {
  using llvm::Function;
  using llvm::BasicBlock;
  // First, check for an existing function from a previous 'extern' declaration.
  Function *TheFunction = TheModule.getFunction(mPrototype->getName());
  if (!TheFunction)
    TheFunction = mPrototype->codegen(TheContext, Builder, TheModule, NamedValues);
  if (!TheFunction)
    return nullptr;
  if (!TheFunction->empty())
    return (Function*)LogErrorV("Function cannot be redefined.");
  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
  Builder.SetInsertPoint(BB);
  // Record the function arguments in the NamedValues map.
  // TODO: this is a global map. Why do we clear it entirely?
  NamedValues.clear();
  for (auto &Arg : TheFunction->args())
    NamedValues[std::string(Arg.getName())] = &Arg;
  if (Value *RetVal = mBody->codegen(TheContext, Builder, TheModule, NamedValues)) {
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

unique_ptr<ExprAST> NumberExprAst::clone() const {
  return make_unique<NumberExprAst>(mValue);
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
