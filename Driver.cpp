#include "Driver.h"
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Support/TargetSelect.h>
#include <cmath>

// #define DEBUG_DRIVER

Driver::Driver(const Parser& p):
  mParser(p), mContext(), mBuilder(mContext) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  mJIT = make_unique<KaleidoscopeJIT>();
  InitializeModuleAndPassManager();
}

void Driver::HandleTopLevelExpression() {
  if (auto FnAST = mParser.ParseTopLevelExpr()) {
    if (auto *FnIR = FnAST->codegen(mContext, mBuilder, *mModule, *mFPM, mNamedValues)) {
      std::cerr << "Read a top-level expr:\n";
      FnIR->print(llvm::errs());
      std::cerr << std::endl;
      // JIT the module containing the anonymous expression, keeping a handle so
      // we can free it later.
      auto H = mJIT->addModule(move(mModule));
      InitializeModuleAndPassManager();
      // Search the JIT for the __anon_expr symbol.
      auto ExprSymbol = mJIT->findSymbol("__anon_expr");
      assert(ExprSymbol && "Function not found");
      double (*FP)() = (double (*)())(intptr_t)llvm::cantFail(ExprSymbol.getAddress());
      std::cout << "Evaluated to " << FP() << std::endl;
      // Delete the anonymous expression module from the JIT.
      mJIT->removeModule(H);
    }
    traverseAST(FnAST->clone().get());
    // TODO: read from a symbol table to replace the hard-coded "x"
//     if (auto FnDerivAST = FnAST->Derivative("x")) {
//       if (auto *FnDerivIR = FnDerivAST->codegen(mContext, mBuilder, *mModule, *mFPM, mNamedValues)) {
//         std::cerr << "Derivative function IR:\n";
//         FnDerivIR->print(llvm::errs());
//         std::cerr << std::endl;
//       }
//       std::cout << "Traverse the derivative:\n";
//       traverseAST(FnDerivAST.get());
//     }
  } else {
    mParser.getNextToken();
  }
}

void Driver::HandleDefinition() {
  if (auto FnAST = mParser.ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen(mContext, mBuilder, *mModule, *mFPM, mNamedValues)) {
      std::cerr << "Read function definition:\n";
      FnIR->print(llvm::errs());
      std::cerr << std::endl;
      auto H = mJIT->addModule(move(mModule));
      InitializeModuleAndPassManager();
    }
    traverseAST(FnAST.get());
  } else {
    mParser.getNextToken();
  }
}

void Driver::HandleExtern() {
  if (auto ProtoAST = mParser.ParseExtern()) {
    if (auto *ProtoIR = ProtoAST->codegen(mContext, mBuilder, *mModule, mNamedValues)) {
      std::cerr << "Read extern:\n";
      ProtoIR->print(llvm::errs());
      std::cerr << std::endl;
      traverseAST(ProtoAST.get());
      mFunctionProtos[ProtoAST->getName()] = move(ProtoAST);
    }
  } else {
    mParser.getNextToken();
  }
}

void Driver::MainLoop() {
//   mParser.getNextToken();
  bool firsttime = true;
  mParser.PrintCurrentToken();
  while (true) {
    std::cerr << "ready> ";
    std::string line;
    std::getline(std::cin, line);
//     std::cout << "Current input line: " << line << std::endl;
    if (firsttime) {
      mParser.SetupInput(line);
      mParser.getNextToken();
      firsttime = false;
    } else {
      mParser.SetupInput(line);
      mParser.getNextToken();
    }
    std::cout << "Current buffer: " << mParser.getInputString() << std::endl;
    mParser.PrintCurrentToken();
    switch (std::get<0>(mParser.getCurrentToken())) {
      case Token::Eof: {
        std::cout << "Current string:\n";
        std::cout << mParser.getInputString() << std::endl;
        return;
      }
      case Token::Semicolon:
        mParser.getNextToken();
        break;
      case Token::Definition:
        HandleDefinition();
        break;
      case Token::Extern:
        HandleExtern();
        break;
      default:
        HandleTopLevelExpression();
        break;
    }
//     std::cout << "switch end: ";
    mParser.PrintCurrentToken();
  }
}

tuple<string, double> Driver::traverseAST(const ExprAST* Node) const {
  using std::make_tuple;
  static int index = 0;
  const string Type = Node->Type();
  if (Type == "ExprAST") {
    return make_tuple("", 0);
  } else if (Type == "NumberExprAST") {
#ifdef DEBUG_DRIVER
    std::cout << "Visiting a " << Type << ": "
              << static_cast<const NumberExprAST*>(Node)->getNumber() << std::endl;
#endif
    const string ResName = "res" + std::to_string(index);
    std::cout << "Compute " << ResName << " = "
              << static_cast<const NumberExprAST*>(Node)->getNumber() << std::endl;
    ++index;
    return make_tuple(ResName, static_cast<const NumberExprAST*>(Node)->getNumber());
  } else if (Type == "VariableExprAST") {
#ifdef DEBUG_DRIVER
    std::cout << "Visiting a " << Type << ": "
              << static_cast<const VariableExprAST*>(Node)->getVariable() << std::endl;
#endif
    const string ResName = "res" + std::to_string(index);
    std::cout << "Compute " << ResName << " = "
              << static_cast<const VariableExprAST*>(Node)->getVariable() << std::endl;
    ++index;
    return make_tuple(ResName, 0.0);
  } else if (Type == "BinaryExprAST") {
    const string Op = static_cast<const BinaryExprAST*>(Node)->getOperator();
#ifdef DEBUG_DRIVER
    std::cout << "Visiting a " << Type << ": " << Op << std::endl;
    std::cout << "Visiting the LHS: " << std::endl;
#endif
    const auto [ResNameL, ValL] = traverseAST(static_cast<const BinaryExprAST*>(Node)->getLHSExpr());
#ifdef DEBUG_DRIVER
    std::cout << "Visiting the RHS: " << std::endl;
#endif
    const auto [ResNameR, ValR] = traverseAST(static_cast<const BinaryExprAST*>(Node)->getRHSExpr());
    double result = 0;
    if (Op == "+") {result = ValL + ValR;}
    else if (Op == "-") {result = ValL - ValR;}
    else if (Op == "*") {result = ValL * ValR;}
    else if (Op == "/") {result = ValL / ValR;}
    else if (Op == "^") {result = std::pow(ValL, ValR);}
    else {result = 0;}
    const string ResName = "res" + std::to_string(index);
    std::cout << "Compute " << ResName << " = "
              << ResNameL << " " << Op << " " << ResNameR << std::endl;
    ++index;
    return make_tuple(ResName, result);
  } else if (Type == "CallExprAST") {
#ifdef DEBUG_DRIVER
    std::cout << "Visiting a " << Type << ": "
              << static_cast<const CallExprAST*>(Node)->getCallee() << " ; numargs = "
              << static_cast<const CallExprAST*>(Node)->getNumberOfArguments() << std::endl;
    std::cout << "Visiting the arguments:\n";
#endif
    vector<const ExprAST*> v = static_cast<const CallExprAST*>(Node)->getArguments();
    vector<double> ArgumentResults;
    for (const auto& i : v) {
      ArgumentResults.push_back(std::get<1>(traverseAST(i)));
    }
    // TODO: call the function with ArgumentResults
  }
  return make_tuple("", 0);
}

void Driver::traverseAST(const PrototypeAST* Node) const {
  const string Type = Node->Type();
#ifdef DEBUG_DRIVER
  std::cout << "Visiting a " << Type << "\n";
#endif
}

void Driver::traverseAST(const FunctionAST* Node) const {
  const string Type = Node->Type();
#ifdef DEBUG_DRIVER
  std::cout << "Visiting a " << Type << ":\n";
  std::cout << "Visiting the prototype:\n";
#endif
  traverseAST(Node->getPrototype());
#ifdef DEBUG_DRIVER
  std::cout << "Visiting the function body:\n";
#endif
  const auto [ResName, result] = traverseAST(Node->getBody());
  std::cout << "Result = " << result << std::endl;
}

void Driver::InitializeModuleAndPassManager() {
  mModule = make_unique<Module>("calculator", mContext);
  mFPM = make_unique<llvm::legacy::FunctionPassManager>(mModule.get());
  
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  mFPM->add(llvm::createInstructionCombiningPass());
  // Reassociate expressions.
  mFPM->add(llvm::createReassociatePass());
  // Eliminate Common SubExpressions.
  mFPM->add(llvm::createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  mFPM->add(llvm::createCFGSimplificationPass());

  mFPM->doInitialization();
}
