#ifndef MAINJIT_H
#define MAINJIT_H

#include <llvm/ADT/STLExtras.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/RuntimeDyld.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <memory>
#include <map>

using std::make_unique;
using std::make_shared;
using std::move;
using std::unique_ptr;
using std::string;
using std::map;
using std::shared_ptr;

namespace llvm {
namespace orc {

// The JIT code looks cryptic and what the tutorial says does not always
// match the full code list.
// LLVM 12 will significantly change the ORC API, and the following code
// should be adapted accordingly.
class MainJIT {
private:
  // provide context for JIT'd code and represent the JIT'd program
  ExecutionSession mES;
  unique_ptr<TargetMachine> mTM;
  const DataLayout mDL; // symbol mangling
  // object linking layer
  RTDyldObjectLinkingLayer mObjectLayer;

  // LLVM IR layer
  IRCompileLayer mCompileLayer;
  // optimization layer
  IRTransformLayer mOptimizeLayer;

  unique_ptr<JITCompileCallbackManager> mCompileCallbackManager;
//   CompileOnDemandLayer mCODLayer;

  MangleAndInterner mMangle; // symbol mangling
  ThreadSafeContext mCtx; // LLVMContext for building IR files
  // provide the symbol table
  JITDylib &mMainJD;
public:
//   MainJIT();
  MainJIT(JITTargetMachineBuilder JTMB, DataLayout DL);
  static Expected<unique_ptr<MainJIT>> Create();
  const DataLayout &getDataLayout() const;
  LLVMContext &getContext();
  Error addModule(unique_ptr<Module> M);
  Expected<JITEvaluatedSymbol> lookup(StringRef Name);
private:
//   string mangle(const string& Name);
  static Expected<ThreadSafeModule>
  optimizeModule(ThreadSafeModule TSM, const MaterializationResponsibility &R);
};

} // end namespace orc
} // end namespace llvm

#endif // MAINJIT_H
