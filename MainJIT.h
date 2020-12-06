#ifndef MAINJIT_H
#define MAINJIT_H

#include <llvm/ADT/StringRef.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>
#include <memory>

using llvm::orc::ExecutionSession;
using llvm::orc::RTDyldObjectLinkingLayer;
using llvm::orc::IRCompileLayer;
using llvm::DataLayout;
using llvm::orc::ThreadSafeContext;
using llvm::orc::JITTargetMachineBuilder;
using llvm::orc::ConcurrentIRCompiler;
using llvm::orc::MangleAndInterner;
using llvm::SectionMemoryManager;
using llvm::LLVMContext;
using llvm::orc::JITDylib;

using std::make_unique;
using std::move;

class MainJIT {
private:
  ExecutionSession mES;
  RTDyldObjectLinkingLayer mObjectLayer;
  IRCompileLayer mCompileLayer;
  DataLayout mDL;
  MangleAndInterner mMangle;
  ThreadSafeContext mCtx;
  JITDylib &mMainJD;
public:
  MainJIT(JITTargetMachineBuilder JTMB, DataLayout DL);
};

#endif // MAINJIT_H
