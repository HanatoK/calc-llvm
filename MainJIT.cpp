#include "MainJIT.h"

namespace llvm {
namespace orc {

// Constructor
// SectionMemoryManager: an off-the-shelf utility that provides all the basic 
//                       memory management functionality.
// ConcurrentIRCompiler: build LLVM target machine
MainJIT::MainJIT(JITTargetMachineBuilder JTMB, DataLayout DL):
  mTM(EngineBuilder().selectTarget()), mDL(move(DL)),
  mObjectLayer(mES, [](){return make_unique<SectionMemoryManager>();}),
  mCompileLayer(mES, mObjectLayer,
                make_unique<ConcurrentIRCompiler>(move(JTMB))),
  mOptimizeLayer(mES, mCompileLayer, optimizeModule),
  mCompileCallbackManager(move(createLocalCompileCallbackManager(
    mTM->getTargetTriple(), mES, 0).get())),
  mMangle(mES, mDL), mCtx(make_unique<LLVMContext>()),
  mMainJD(mES.createBareJITDylib("<main>")) {
  // mMainJD is a dylib that contains not only the symbols we add, but also
  // from runtime
  mMainJD.addGenerator(
    cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
      mDL.getGlobalPrefix())));
}

Expected<unique_ptr<MainJIT>> MainJIT::Create() {
  auto JTMB = JITTargetMachineBuilder::detectHost();
  if (!JTMB)
    return JTMB.takeError();
  auto DL = JTMB->getDefaultDataLayoutForTarget();
  if (!DL)
    return DL.takeError();
  return make_unique<MainJIT>(move(*JTMB), move(*DL));
}

const DataLayout& MainJIT::getDataLayout() const {
  return mDL;
}

LLVMContext& MainJIT::getContext() {
  return *mCtx.getContext();
}

Error MainJIT::addModule(unique_ptr<Module> M) {
  return mOptimizeLayer.add(mMainJD, ThreadSafeModule(move(M), mCtx));
}

Expected<JITEvaluatedSymbol> MainJIT::lookup(StringRef Name) {
  return mES.lookup({&mMainJD}, mMangle(Name.str()));
}

Expected<ThreadSafeModule> MainJIT::optimizeModule(ThreadSafeModule TSM,
                                          const MaterializationResponsibility &R) {
  TSM.withModuleDo([](Module &M) {
    // create a function pass manager
    auto FPM = make_unique<legacy::FunctionPassManager>(&M);
    // add some optimizations
    FPM->add(createInstructionCombiningPass());
    FPM->add(createReassociatePass());
    FPM->add(createGVNPass());
    FPM->add(createCFGSimplificationPass());
    FPM->doInitialization();
    // Run the optimizations over all functions in the module being added to
    // the JIT.
    for (auto &F : M)
      FPM->run(F);
  });
  return move(TSM);
}

} // end namespace orc
} // end namespace llvm
