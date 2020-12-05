#include "Driver.h"

void Driver::HandleTopLevelExpression() {
  if (auto FnAST = mParser.ParseTopLevelExpr()) {
    if (auto *FnIR = FnAST->codegen(mContext, mBuilder, mModule, mNamedValues)) {
      std::cerr << "Read a top-level expr:\n";
      FnIR->print(llvm::errs());
      std::cerr << std::endl;
    }
    traverseAST(FnAST.get());
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

void Driver::traverseAST(const ExprAST* Node) const {
  const string Type = Node->Type();
  if (Type == "ExprAST") {
    return;
  } else if (Type == "NumberExprAst") {
    std::cout << "Visiting a " << Type << ": "
              << static_cast<const NumberExprAst*>(Node)->getNumber() << std::endl;
  } else if (Type == "VariableExprAST") {
    std::cout << "Visiting a " << Type << ": "
              << static_cast<const VariableExprAST*>(Node)->getVariable() << std::endl;
  } else if (Type == "BinaryExprAST") {
    std::cout << "Visiting a " << Type << ": "
              << static_cast<const BinaryExprAST*>(Node)->getOperator() << std::endl;
    std::cout << "Visiting the LHS: " << std::endl;
    traverseAST(static_cast<const BinaryExprAST*>(Node)->getLHSExpr());
    std::cout << "Visiting the RHS: " << std::endl;
    traverseAST(static_cast<const BinaryExprAST*>(Node)->getRHSExpr());
  } else if (Type == "CallExprAST") {
    std::cout << "Visiting a " << Type << ": "
              << static_cast<const CallExprAST*>(Node)->getCallee() << " ; numargs = "
              << static_cast<const CallExprAST*>(Node)->getNumberOfArguments();
    std::cout << "Visiting the arguments:\n";
    vector<const ExprAST*> v = static_cast<const CallExprAST*>(Node)->getArguments();
    for (const auto& i : v) {
      traverseAST(i);
    }
  }
}

void Driver::traverseAST(const PrototypeAST* Node) const {
  const string Type = Node->Type();
  std::cout << "Visiting a " << Type << "\n";
}

void Driver::traverseAST(const FunctionAST* Node) const {
  const string Type = Node->Type();
  std::cout << "Visiting a " << Type << ":\n";
  std::cout << "Visiting the prototype:\n";
  traverseAST(Node->getPrototype());
  std::cout << "Visiting the function body:\n";
  traverseAST(Node->getBody());
}
