#include "Driver.h"

void Driver::HandleTopLevelExpression() {
  if (auto FnAST = mParser.ParseTopLevelExpr()) {
    if (auto *FnIR = FnAST->codegen(mContext, mBuilder, mModule, mNamedValues)) {
      std::cerr << "Read a top-level expr:\n";
      FnIR->print(llvm::errs());
      std::cerr << std::endl;
    }
  } else {
    mParser.getNextToken();
  }
}

void Driver::MainLoop() {
  mParser.getNextToken();
  while (true) {
    switch (std::get<0>(mParser.getCurrentToken())) {
      case Token::Eof: return;
      default:
        HandleTopLevelExpression();
        break;
    }
  }
}
