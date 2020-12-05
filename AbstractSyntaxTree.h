#ifndef ABSTRACTSYNTAXTREE_H
#define ABSTRACTSYNTAXTREE_H
// #include <llvm/IR/Type.h>
// #include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <map>

using std::string;
using std::unique_ptr;
using std::move;
using std::vector;
using std::map;

using llvm::Value;
using llvm::LLVMContext;
using llvm::IRBuilder;
using llvm::Module;
using llvm::Function;

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() = default;
  virtual Value *codegen(LLVMContext& TheContext,
                         IRBuilder<>& Builder,
                         Module& TheModule,
                         map<string, Value*>& NamedValues) = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAst: public ExprAST {
private:
  double mValue;
public:
  NumberExprAst(double Val): mValue(Val) {}
  virtual Value *codegen(LLVMContext& TheContext,
                         IRBuilder<>& Builder,
                         Module& TheModule,
                         map<string, Value*>& NamedValues);
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST: public ExprAST {
private:
  string mName;
public:
  VariableExprAST(const string& Name): mName(Name) {}
  virtual Value *codegen(LLVMContext& TheContext,
                         IRBuilder<>& Builder,
                         Module& TheModule,
                         map<string, Value*>& NamedValues);
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST: public ExprAST {
private:
  std::string mOperator;
  unique_ptr<ExprAST> mLHS;
  unique_ptr<ExprAST> mRHS;
public:
  BinaryExprAST(char Op, unique_ptr<ExprAST> LHS, unique_ptr<ExprAST> RHS)
    : mOperator{Op}, mLHS(move(LHS)), mRHS(move(RHS)) {}
  virtual Value *codegen(LLVMContext& TheContext,
                         IRBuilder<>& Builder,
                         Module& TheModule,
                         map<string, Value*>& NamedValues);
};

/// UnaryExprAST - Expression class for a unary operator.
// Do I need this??
// class UnaryExprAST: public ExprAST {
// private:
//   char mOperator;
//   unique_ptr<ExprAST> mRHS;
// public:
//   UnaryExprAST(char Op, unique_ptr<ExprAST> RHS)
//     : mOperator(Op), mRHS(move(RHS)) {}
// };

/// CallExprAST - Expression class for function calls.
class CallExprAST: public ExprAST {
private:
  string mCallee;
  vector<unique_ptr<ExprAST>> mArguments;
public:
  CallExprAST(const string& Callee, vector<unique_ptr<ExprAST>> Args)
    : mCallee(Callee), mArguments(move(Args)) {}
  virtual Value *codegen(LLVMContext& TheContext,
                         IRBuilder<>& Builder,
                         Module& TheModule,
                         map<string, Value*>& NamedValues);
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
private:
  string mName;
  vector<string> mArguments;
public:
  PrototypeAST(const string& Name, vector<string> Args)
    : mName(Name), mArguments(move(Args)) {}
  string getName() const;
  Function *codegen(LLVMContext& TheContext,
                    IRBuilder<>& Builder,
                    Module& TheModule,
                    map<string, Value*>& NamedValues);
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
private:
  unique_ptr<PrototypeAST> mPrototype;
  unique_ptr<ExprAST> mBody;
public:
  FunctionAST(unique_ptr<PrototypeAST> Proto, unique_ptr<ExprAST> Body)
    : mPrototype(move(Proto)), mBody(move(Body)) {}
  Function *codegen(LLVMContext& TheContext,
                    IRBuilder<>& Builder,
                    Module& TheModule,
                    map<string, Value*>& NamedValues);
};

unique_ptr<ExprAST> LogError(const string& Str);
Value *LogErrorV(const string& Str);

#endif // ABSTRACTSYNTAXTREE_H
