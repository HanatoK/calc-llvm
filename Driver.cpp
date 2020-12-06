#include "Driver.h"
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <cmath>

Driver::Driver(const Parser& p):
  mParser(p), mContext(), mBuilder(mContext), mModule("calculator", mContext),
  mFPM(&mModule) {
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  mFPM.add(llvm::createInstructionCombiningPass());
  // Reassociate expressions.
  mFPM.add(llvm::createReassociatePass());
  // Eliminate Common SubExpressions.
  mFPM.add(llvm::createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  mFPM.add(llvm::createCFGSimplificationPass());

  mFPM.doInitialization();
}

void Driver::HandleTopLevelExpression() {
  if (auto FnAST = mParser.ParseTopLevelExpr()) {
    if (auto *FnIR = FnAST->codegen(mContext, mBuilder, mModule, mFPM, mNamedValues)) {
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
              << static_cast<const CallExprAST*>(Node)->getNumberOfArguments() << std::endl;
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

const ExprAST* Driver::evaluateAST(const ExprAST* Node) const {
  // TODO: use smart pointer!!!
  const string Type = Node->Type();
  if (Type == "ExprAST") {
    return nullptr;
  } else if (Type == "NumberExprAst") {
    return Node;
  } else if (Type == "VariableExprAST") {
    std::cout << "Evaluation of VariableExprAST is not implemented.\n";
    return nullptr;
  } else if (Type == "BinaryExprAST") {
    const ExprAST* LHS = static_cast<const BinaryExprAST*>(Node)->getLHSExpr();
    const ExprAST* RHS = static_cast<const BinaryExprAST*>(Node)->getRHSExpr();
    LHS = evaluateAST(LHS);
    RHS = evaluateAST(RHS);
    if ((LHS->Type() == "NumberExprAst") && (RHS->Type() == "NumberExprAst")) {
      const string Op = static_cast<const BinaryExprAST*>(Node)->getOperator();
      const double ValL = static_cast<const NumberExprAst*>(LHS)->getNumber();
      const double ValR = static_cast<const NumberExprAst*>(RHS)->getNumber();
      if (Op == "+") {
        return new NumberExprAst(ValL + ValR);
      } else if (Op == "-") {
        return new NumberExprAst(ValL - ValR);
      } else if (Op == "*") {
        return new NumberExprAst(ValL * ValR);
      } else if (Op == "/") {
        return new NumberExprAst(ValL / ValR);
      } else if (Op == "^") {
        return new NumberExprAst(std::pow(ValL, ValR));
      } else {
        std::cout << "Unknown operator " << Op << "\n";
        return nullptr;
      }
    }
  }
  return nullptr;
}
