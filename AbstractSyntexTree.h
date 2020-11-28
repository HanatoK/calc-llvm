#ifndef ABSTRACTSYNTAXTREE_H
#define ABSTRACTSYNTAXTREE_H
#include <string>
#include <memory>
#include <utility>
#include <vector>

using std::string;
using std::unique_ptr;
using std::move;
using std::vector;

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() = default;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAst: public ExprAST {
private:
  double mValue;
public:
  NumberExprAst(double Val): mValue(Val) {}
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST: public ExprAST {
private:
  string mName;
public:
  VariableExprAST(const string& Name): mName(Name) {}
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST: public ExprAST {
private:
  char mOperator;
  unique_ptr<ExprAST> mLHS;
  unique_ptr<ExprAST> mRHS;
public:
  BinaryExprAST(char Op, unique_ptr<ExprAST> LHS, unique_ptr<ExprAST> RHS)
    : mOperator(Op), mLHS(move(LHS)), mRHS(move(RHS)) {}
};

/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST: public ExprAST {
private:
  char mOperator;
  unique_ptr<ExprAST> mRHS;
public:
  UnaryExprAST(char Op, unique_ptr<ExprAST> RHS)
    : mOperator(Op), mRHS(move(RHS)) {}
};

/// CallExprAST - Expression class for function calls.
class CallExprAST: public {
private:
  string mCallee;
  vector<unique_ptr<ExprAST>> mArguments;
public:
  CallExprAST(const string& Callee, vector<unique_ptr<ExprAST>> Args)
    : mCallee(Callee), mArguments(move(Args)) {}
};

#endif // ABSTRACTSYNTAXTREE_H
