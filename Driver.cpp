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

double Driver::traverseAST(const ExprAST* Node) const {
  static int index = 0;
  const string Type = Node->Type();
  if (Type == "ExprAST") {
    return 0;
  } else if (Type == "NumberExprAst") {
    std::cout << "Visiting a " << Type << ": "
              << static_cast<const NumberExprAst*>(Node)->getNumber() << std::endl;
    std::cout << "Compute var" << index << " = "
              << static_cast<const NumberExprAst*>(Node)->getNumber() << std::endl;
    ++index;
    return static_cast<const NumberExprAst*>(Node)->getNumber();
  } else if (Type == "VariableExprAST") {
    std::cout << "Visiting a " << Type << ": "
              << static_cast<const VariableExprAST*>(Node)->getVariable() << std::endl;
  } else if (Type == "BinaryExprAST") {
    const string Op = static_cast<const BinaryExprAST*>(Node)->getOperator();
    std::cout << "Visiting a " << Type << ": " << Op << std::endl;
    std::cout << "Visiting the LHS: " << std::endl;
    const double ValL = traverseAST(static_cast<const BinaryExprAST*>(Node)->getLHSExpr());
    std::cout << "Visiting the RHS: " << std::endl;
    const double ValR = traverseAST(static_cast<const BinaryExprAST*>(Node)->getRHSExpr());
    double result = 0;
    if (Op == "+") {result = ValL + ValR;}
    else if (Op == "-") {result = ValL - ValR;}
    else if (Op == "*") {result = ValL * ValR;}
    else if (Op == "/") {result = ValL / ValR;}
    else if (Op == "^") {result = std::pow(ValL, ValR);}
    else {result = 0;}
    std::cout << "Compute var" << index << " = "
              << ValL << " " << Op << " " << ValR << std::endl;
    ++index;
    return result;
  } else if (Type == "CallExprAST") {
    std::cout << "Visiting a " << Type << ": "
              << static_cast<const CallExprAST*>(Node)->getCallee() << " ; numargs = "
              << static_cast<const CallExprAST*>(Node)->getNumberOfArguments() << std::endl;
    std::cout << "Visiting the arguments:\n";
    vector<const ExprAST*> v = static_cast<const CallExprAST*>(Node)->getArguments();
    vector<double> ArgumentResults;
    for (const auto& i : v) {
      ArgumentResults.push_back(traverseAST(i));
    }
    // TODO: call the function with ArgumentResults
  }
  return 0;
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
  const double result = traverseAST(Node->getBody());
  std::cout << "Result = " << result << std::endl;
}
