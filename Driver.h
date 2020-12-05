#ifndef DRIVER_H
#define DRIVER_H

#include <map>
#include <string>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>

#include "Parser.h"

using std::map;
using std::string;
using llvm::LLVMContext;
using llvm::IRBuilder;
using llvm::Module;
using llvm::Value;

class Driver {
public:
  Driver(const Parser& p):
    mParser(p), mContext(), mBuilder(mContext), mModule("calculator", mContext) {}
  void HandleTopLevelExpression();
  void MainLoop();
private:
  Parser mParser;
  LLVMContext mContext;
  IRBuilder<> mBuilder;
  Module mModule;
  map<string, Value*> mNamedValues;
};

#endif // DRIVER_H
