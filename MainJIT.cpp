#include "MainJIT.h"

MainJIT::MainJIT(JITTargetMachineBuilder JTMB, DataLayout DL):
  mObjectLayer(mES, [](){return make_unique<SectionMemoryManager>();}),
  mCompileLayer(mES, mObjectLayer,
                make_unique<ConcurrentIRCompiler>(move(JTMB))),
  mDL(move(DL)), mMangle(mES, mDL), mCtx(make_unique<LLVMContext>()),
  mMainJD(mES.createBareJITDylib("<main>")){
  using namespace llvm;
  using namespace llvm::orc;
  mMainJD.addGenerator(
    cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
      mDL.getGlobalPrefix())));
}
