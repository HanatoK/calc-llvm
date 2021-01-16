#ifndef DRIVER_H
#define DRIVER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <map>
#include <string>
#include <memory>
#include <tuple>
#include <vector>

#include "Parser.h"
#include "KaleidoscopeJIT.h"

using std::map;
using std::string;
using std::unique_ptr;
using std::tuple;
using std::vector;

using llvm::LLVMContext;
using llvm::IRBuilder;
using llvm::Module;
using llvm::Value;
using llvm::legacy::FunctionPassManager;
using llvm::orc::KaleidoscopeJIT;

class Driver {
public:
  Driver(const Parser& p);
  void HandleTopLevelExpression();
  void HandleExtern();
  void HandleDefinition();
  void LoadLibraryFunctions();
  void MainLoop();
  tuple<string, double> traverseAST(const ExprAST* Node) const;
  static void traverseAST(const PrototypeAST* Node) ;
  void traverseAST(const FunctionAST* Node) const;
  void InitializeModuleAndPassManager();
  Function *getFunction(const string& Name);
  // currently I do not have a clear idea for avoiding this public maps...
  // TODO: check the function signature!
  map<string, unique_ptr<PrototypeAST>> mFunctionProtos;
  map<string, unique_ptr<FunctionAST>> mDerivativeFunctions;
private:
  Parser mParser;
  LLVMContext mContext;
  IRBuilder<> mBuilder;
  unique_ptr<Module> mModule;
  unique_ptr<FunctionPassManager> mFPM;
  unique_ptr<KaleidoscopeJIT> mJIT;
  map<string, Value*> mNamedValues;
};

#endif // DRIVER_H
