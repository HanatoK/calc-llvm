#ifndef ABSTRACTSYNTAXTREE_H
#define ABSTRACTSYNTAXTREE_H
// #include <llvm/IR/Type.h>
// #include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <map>

using std::string;
using std::unique_ptr;
using std::make_unique;
using std::move;
using std::vector;
using std::map;

using llvm::Value;
using llvm::LLVMContext;
using llvm::IRBuilder;
using llvm::Module;
using llvm::Function;
using llvm::legacy::FunctionPassManager;

class CallExprAST;
class NumberExprAST;

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual string Type() const {
    return string{"ExprAST"};
  }
  virtual ~ExprAST() = default;
  virtual Value *codegen(LLVMContext& TheContext,
                         IRBuilder<>& Builder,
                         Module& TheModule,
                         map<string, Value*>& NamedValues) = 0;
  virtual unique_ptr<ExprAST> clone() const = 0;
  virtual unique_ptr<ExprAST> Derivative(const string& Variable) const;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST: public ExprAST {
private:
  double mValue;
public:
  virtual string Type() const {
    return string{"NumberExprAST"};
  }
  double getNumber() const {
    return mValue;
  }
  NumberExprAST(double Val): mValue(Val) {}
  virtual Value *codegen(LLVMContext& TheContext,
                         IRBuilder<>& Builder,
                         Module& TheModule,
                         map<string, Value*>& NamedValues);
  virtual unique_ptr<ExprAST> clone() const;
  virtual unique_ptr<ExprAST> Derivative(const string& Variable) const;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST: public ExprAST {
private:
  string mName;
public:
  virtual string Type() const {
    return string{"VariableExprAST"};
  }
  string getVariable() const {
    return mName;
  }
  VariableExprAST(const string& Name): mName(Name) {}
  virtual Value *codegen(LLVMContext& TheContext,
                         IRBuilder<>& Builder,
                         Module& TheModule,
                         map<string, Value*>& NamedValues);
  virtual unique_ptr<ExprAST> clone() const;
  virtual unique_ptr<ExprAST> Derivative(const string& Variable) const;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST: public ExprAST {
private:
  std::string mOperator;
  unique_ptr<ExprAST> mLHS;
  unique_ptr<ExprAST> mRHS;
public:
  virtual string Type() const {
    return string{"BinaryExprAST"};
  }
  string getOperator() const {
    return mOperator;
  }
  const ExprAST* getLHSExpr() const {
    if (!mLHS) return nullptr;
    return mLHS.get();
  }
  const ExprAST* getRHSExpr() const {
    if (!mRHS) return nullptr;
    return mRHS.get();
  }
  BinaryExprAST(const string& Op, unique_ptr<ExprAST> LHS,
                unique_ptr<ExprAST> RHS)
    : mOperator{Op}, mLHS(move(LHS)), mRHS(move(RHS)) {}
  virtual Value *codegen(LLVMContext& TheContext,
                         IRBuilder<>& Builder,
                         Module& TheModule,
                         map<string, Value*>& NamedValues);
  virtual unique_ptr<ExprAST> clone() const;
  virtual unique_ptr<ExprAST> Derivative(const string& Variable) const;
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
  virtual string Type() const {
    return string{"CallExprAST"};
  }
  string getCallee() const {
    return mCallee;
  }
  size_t getNumberOfArguments() const {
    return mArguments.size();
  }
  vector<const ExprAST*> getArguments() const {
    vector<const ExprAST*> result;
    for (const auto& i : mArguments) {
      if (!i) result.push_back(nullptr);
      else result.push_back(i.get());
    }
    return result;
  }
  CallExprAST(const string& Callee, vector<unique_ptr<ExprAST>> Args)
    : mCallee(Callee), mArguments(move(Args)) {}
  virtual Value *codegen(LLVMContext& TheContext,
                         IRBuilder<>& Builder,
                         Module& TheModule,
                         map<string, Value*>& NamedValues);
  virtual unique_ptr<ExprAST> clone() const;
  virtual unique_ptr<ExprAST> Derivative(const string& Variable) const;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
private:
  string mName;
  vector<string> mArguments;
public:
  virtual string Type() const {
    return string{"PrototypeAST"};
  }
  size_t getNumberOfArguments() const {
    return mArguments.size();
  }
  vector<string> getArgumentNames() const {
    return mArguments;
  }
  PrototypeAST(const string& Name, vector<string> Args)
    : mName(Name), mArguments(move(Args)) {}
  string getName() const;
  Function *codegen(LLVMContext& TheContext,
                    IRBuilder<>& Builder,
                    Module& TheModule,
                    map<string, Value*>& NamedValues);
  virtual unique_ptr<PrototypeAST> clone() const;
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
private:
  unique_ptr<PrototypeAST> mPrototype;
  unique_ptr<ExprAST> mBody;
public:
  virtual string Type() const {
    return string{"FunctionAST"};
  }
  const PrototypeAST* getPrototype() const {
    if (!mPrototype) return nullptr;
    return mPrototype.get();
  }
  const ExprAST* getBody() const {
    if (!mBody) return nullptr;
    return mBody.get();
  }
  FunctionAST(unique_ptr<PrototypeAST> Proto, unique_ptr<ExprAST> Body)
    : mPrototype(move(Proto)), mBody(move(Body)) {}
  Function *codegen(LLVMContext& TheContext,
                    IRBuilder<>& Builder,
                    Module& TheModule,
                    FunctionPassManager& FPM,
                    map<string, Value*>& NamedValues);
  virtual unique_ptr<FunctionAST> clone() const;
  virtual unique_ptr<FunctionAST> Derivative(const string& Variable) const;
};

unique_ptr<ExprAST> LogError(const string& Str);
unique_ptr<FunctionAST> LogErrorF(const string& Str);
unique_ptr<PrototypeAST> LogErrorP(const string& Str);
Value *LogErrorV(const string& Str);

#endif // ABSTRACTSYNTAXTREE_H
