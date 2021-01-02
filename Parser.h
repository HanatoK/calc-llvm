#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <iostream>
#include <string>
#include <memory>
#include <tuple>
#include <variant>
#include <sstream>

#include "Lexer.h"
#include "AbstractSyntaxTree.h"

using std::map;
using std::string;
using std::unique_ptr;
using std::variant;
using std::tuple;
using std::stringstream;
using std::make_unique;

class Parser {
public:
  static map<string, int> mBinaryOpPrecedence;
  static map<string, bool> mRightAssociative;
  static map<string, int> mUnaryOpPrecedence;
  Parser();
  Parser(const string& Str);
  void SetupInput(const string& Str);

    [[maybe_unused]] void AppendString(const string& Str);
  string getInputString() const;
  static int GetBinaryPrecedence(const string& Op) ;
  static int GetUnaryPrecedence(const string& Op) ;
  static bool IsRightAssociative(const string& Op) ;
  tuple<Token, variant<string, double>> getNextToken();
  tuple<Token, variant<string, double>> getCurrentToken() const;
  void PrintCurrentToken() const;
  unique_ptr<ExprAST> ParseNumberExpr();
  unique_ptr<ExprAST> ParseParenExpr();
  unique_ptr<ExprAST> ParseIdentifierExpr();
  unique_ptr<ExprAST> ParseIfExpr();
  unique_ptr<ExprAST> ParsePrimary();
  unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, unique_ptr<ExprAST> LHS);
  unique_ptr<ExprAST> ParseExpression();
  unique_ptr<ExprAST> ParseUnaryOpRHS();
  unique_ptr<ExprAST> ParseForExpr();
  unique_ptr<PrototypeAST> ParsePrototype();
  unique_ptr<FunctionAST> ParseDefinition();
  unique_ptr<FunctionAST> ParseTopLevelExpr();
  unique_ptr<PrototypeAST> ParseExtern();
private:
  tuple<Token, variant<string, double>> mCurrentToken;
  Lexer mLexer;
};

// extern map<string, int> Parser::mBinaryOpPrecedence;
// extern map<string, bool> Parser::mRightAssociative;
// extern map<string, int> Parser::mUnaryOpPrecedence;

#endif // PARSER_H
