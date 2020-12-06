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

#include "Parser.h"

using std::map;
using std::string;
using std::unique_ptr;

using llvm::LLVMContext;
using llvm::IRBuilder;
using llvm::Module;
using llvm::Value;
using llvm::legacy::FunctionPassManager;

class Driver {
public:
  Driver(const Parser& p);
  void HandleTopLevelExpression();
  void MainLoop();
  double traverseAST(const ExprAST* Node) const;
  void traverseAST(const PrototypeAST* Node) const;
  void traverseAST(const FunctionAST* Node) const;
private:
  Parser mParser;
  LLVMContext mContext;
  IRBuilder<> mBuilder;
  Module mModule;
  FunctionPassManager mFPM;
  map<string, Value*> mNamedValues;
};

#endif // DRIVER_H
