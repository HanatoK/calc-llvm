#include "Driver.h"

void Driver::HandleTopLevelExpression() {
  if (mParser.ParseTopLevelExpr()) {
    std::cerr << "Parsed a top-level expr\n";
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
