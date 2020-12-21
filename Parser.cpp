#include "Parser.h"

#include <exception>
#include <vector>

using std::move;
using std::unique_ptr;
using std::string;
using std::vector;

// #define DEBUG_PARSER

map<string, int> Parser::mBinaryOpPrecedence = {{"+", 100},
                                                {"-", 100},
                                                {"*", 200},
                                                {"/", 200},
                                                {"^", 300}};

map<string, int> Parser::mUnaryOpPrecedence = {{"+", 250},
                                               {"-", 250}};

map<string, bool> Parser::mRightAssociative = {{"+", false},
                                               {"-", false},
                                               {"*", false},
                                               {"/", false},
                                               {"^", true}};

Parser::Parser() = default;

Parser::Parser(const string& Str) {
  SetupInput(Str);
}

[[maybe_unused]] void Parser::AppendString(const string& Str) {
  mLexer.AppendString(Str);
}

void Parser::SetupInput(const string& Str) {
  mLexer = Lexer(Str);
}

string Parser::getInputString() const {
  return mLexer.str();
}

int Parser::GetBinaryPrecedence(const string& Op) {
  auto FindRes = mBinaryOpPrecedence.find(Op);
  if (FindRes != mBinaryOpPrecedence.end()) {
    return FindRes->second;
  } else {
    return -1;
  }
}

int Parser::GetUnaryPrecedence(const string& Op) {
  auto FindRes = mUnaryOpPrecedence.find(Op);
  if (FindRes != mUnaryOpPrecedence.end()) {
    return FindRes->second;
  } else {
    return -1;
  }
}

bool Parser::IsRightAssociative(const string& Op) {
  auto FindRes = mRightAssociative.find(Op);
  if (FindRes != mRightAssociative.end()) {
    return FindRes->second;
  } else {
    return false;
  }
}

tuple<Token, variant<string, double>> Parser::getNextToken() {
  return mCurrentToken = mLexer.getToken();
}

tuple<Token, variant<string, double>> Parser::getCurrentToken() const {
  return mCurrentToken;
}

void Parser::PrintCurrentToken() const {
  using std::cout;
  using std::get;
  using std::endl;
  const Token T = get<0>(mCurrentToken);
  const auto V = get<1>(mCurrentToken);
  cout << "Current token: " << static_cast<int>(T) << " ";
  switch (T) {
    case Token::Eof: cout << "Eof: " << get<string>(V); break;
    case Token::LeftParenthesis: cout << "LeftParenthesis: " << get<string>(V); break;
    case Token::RightParenthesis: cout << "RightParenthesis: " << get<string>(V); break;
    case Token::Identifier: cout << "Identifier: " << get<string>(V); break;
    case Token::Number: cout << "Number: " << get<double>(V); break;
    case Token::Operator: cout << "Operator: " << get<string>(V); break;
    case Token::Comma: cout << "Comma: " << get<string>(V); break;
    case Token::Semicolon: cout << "Semicolon: " << get<string>(V); break;
    case Token::Extern: cout << "Extern: " << get<string>(V); break;
    case Token::Definition: cout << "Definition: " << get<string>(V); break;
    case Token::Unknown: cout << "Unknown: " << get<string>(V); break;
  }
  cout << endl;
}

unique_ptr<ExprAST> Parser::ParseNumberExpr() {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseNumberExpr()\n";
  PrintCurrentToken();
#endif
  auto Result = make_unique<NumberExprAST>(std::get<double>(std::get<1>(mCurrentToken)));
  getNextToken();
  return move(Result);
}

unique_ptr<ExprAST> Parser::ParseParenExpr() {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseParenExpr()\n";
  PrintCurrentToken();
#endif
  getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V) {
    std::cout << "NULL HERE!\n";
    return nullptr;
  }
  if (std::get<0>(mCurrentToken) != Token::RightParenthesis)
    return LogError("expected ')'");
  getNextToken(); // eat ).
#ifdef DEBUG_PARSER
  std::cout << "End of ParseParenExpr()\n";
#endif
  return V;
}

unique_ptr<ExprAST> Parser::ParseIdentifierExpr() {
  using std::get;
  using std::vector;
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseIdentifierExpr()\n";
  PrintCurrentToken();
#endif
  const string IdName = get<string>(get<1>(mCurrentToken));
  getNextToken(); // eat identifier
  if (get<0>(mCurrentToken) != Token::LeftParenthesis) {
    // Simple variable ref.
    return make_unique<VariableExprAST>(IdName);
  }
  // '(' appears after an identifier, so this is a function call
  getNextToken();
  vector<unique_ptr<ExprAST>> Args;
  if (get<0>(mCurrentToken) != Token::RightParenthesis) {
    while (true) {
      if (auto Arg = ParseExpression())
        Args.push_back(move(Arg));
      else
        return nullptr;
      if (get<0>(mCurrentToken) == Token::RightParenthesis)
        break;
      if (get<0>(mCurrentToken) != Token::Comma)
        return LogError("Expected ')' or ',' in argument list");
      getNextToken();
    }
  }
  // Eat the ')'.
  getNextToken();
  return make_unique<CallExprAST>(IdName, move(Args));
}

unique_ptr<ExprAST> Parser::ParsePrimary() {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParsePrimary()\n";
  PrintCurrentToken();
#endif
  switch (std::get<0>(mCurrentToken)) {
    default: return LogError("unknown token when expecting an expression");
    case Token::Identifier: return ParseIdentifierExpr();
    case Token::Number: return ParseNumberExpr();
    case Token::LeftParenthesis: return ParseParenExpr();
    case Token::Operator: {
      // Parse a signed number
      const char Op = std::get<string>(std::get<1>(mCurrentToken))[0];
      if ((Op == '-') || (Op == '+')) {
        return ParseUnaryOpRHS();
      } else {
        const string ErrorMsg = string("Expect a number before ") + Op;
        return LogError(ErrorMsg);
      }
    }
  }
#ifdef DEBUG_PARSER
  std::cout << "End of ParsePrimary()\n";
#endif
}

unique_ptr<ExprAST> Parser::ParseExpression() {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseExpression()\n";
  PrintCurrentToken();
#endif
  auto LHS = ParsePrimary();
  if (!LHS)
    return nullptr;
#ifdef DEBUG_PARSER
  std::cout << "End of ParseExpression()\n";
#endif
  return ParseBinOpRHS(0, move(LHS));
}

unique_ptr<ExprAST> Parser::ParseUnaryOpRHS() {
  // TODO: I am new to writing a parser. Does the code work for all cases??
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseUnaryOpRHS()\n";
  PrintCurrentToken();
#endif
  using std::get;
  // treat LHS as a zero number for signed values
  auto LHS = make_unique<NumberExprAST>(0.0);
  unique_ptr<ExprAST> RHS;
  // get the precedence of the current unary operator
  const string Op = get<string>(get<1>(mCurrentToken));
  const int TokPrec = GetUnaryPrecedence(Op);
  // eat up the operator
  getNextToken();
  // expect a primary expression after the sign
  RHS = ParsePrimary();
  if (!RHS)
    return nullptr;
  // continue to parse the next operator
  int NextPrec = GetBinaryPrecedence(get<string>(get<1>(mCurrentToken)));
  // NextPrec is -1 for non-operator tokens
  // The case that NextPrec is larger than TokPrec only happens when
  // there is a ^ (power operator).
  if (TokPrec < NextPrec) {
    // take the current RHS as the LHS of the new operator,
    // and parse the binary expression for the new one again.
    RHS = ParseBinOpRHS(TokPrec + 1, move(RHS));
    if (!RHS)
      return nullptr;
  }
  return make_unique<BinaryExprAST>(Op, move(LHS), move(RHS));
}

unique_ptr<ExprAST> Parser::ParseBinOpRHS(int ExprPrec,
                                          unique_ptr<ExprAST> LHS) {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseBinOpRHS()" << " ExprPrec = " << ExprPrec << "\n";
#endif
  using std::get;
  // TODO: figure out what happens in the following code
  while (true) {
    const string Op = get<string>(get<1>(mCurrentToken));
    int TokPrec = GetBinaryPrecedence(Op);
#ifdef DEBUG_PARSER
    std::cout << "Current token in Parser::ParseBinOpRHS(): ";
    PrintCurrentToken();
    std::cout << "Current precedence: TokPrec = " << TokPrec << "\n";
#endif
    if (TokPrec < ExprPrec)
      return LHS;
    getNextToken();
    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;
    const string NextOp = get<string>(get<1>(mCurrentToken));
    const int NextPrec = GetBinaryPrecedence(NextOp);
#ifdef DEBUG_PARSER
    std::cout << "Next token in Parser::ParseBinOpRHS(): ";
    PrintCurrentToken();
    std::cout << "Next precedence: NextPrec = " << NextPrec << "\n";
#endif
    if (Op == NextOp) {
      // check if the same operators are right-associative
      const bool Ra = IsRightAssociative(Op);
      if (Ra) {
#ifdef DEBUG_PARSER
        std::cout << "This is a right-associative operator\n";
#endif
        RHS = ParseBinOpRHS(TokPrec, move(RHS));
        if (!RHS)
          return nullptr;
      }
    }
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, move(RHS));
      if (!RHS)
        return nullptr;
    }
    LHS = make_unique<BinaryExprAST>(Op, move(LHS), move(RHS));
  }
}

unique_ptr<PrototypeAST> Parser::ParsePrototype() {
  using std::get;
  if (get<0>(mCurrentToken) != Token::Identifier) {
    std::cerr << "Token: " << get<string>(get<1>(mCurrentToken)) << std::endl;
    return LogErrorP("Expected function name in prototype");
  }
  string FnName = get<string>(get<1>(mCurrentToken));
  getNextToken();
  if (get<0>(mCurrentToken) != Token::LeftParenthesis)
    return LogErrorP("Expected '(' in prototype");
  vector<string> ArgNames;
  getNextToken();
  Token t = get<0>(mCurrentToken);
  while (t == Token::Identifier) {
    const string IdStr = get<string>(get<1>(mCurrentToken));
    ArgNames.push_back(IdStr);
    getNextToken();
    t = get<0>(mCurrentToken);
    if (t == Token::RightParenthesis) break;
    if (t != Token::Comma) {
      const string ErrorMsg = string{"Expected a comma(,) after "} + IdStr +
                              " but got " +
                              get<string>(get<1>(mCurrentToken));
      return LogErrorP(ErrorMsg);
    }
    getNextToken();
    t = get<0>(mCurrentToken);
  }
  if (t != Token::RightParenthesis)
    return LogErrorP("Expected ')' in prototype");
  getNextToken();
  return make_unique<PrototypeAST>(FnName, move(ArgNames));
}

unique_ptr<FunctionAST> Parser::ParseDefinition() {
  getNextToken(); // eat def.
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;
  if (auto E = ParseExpression())
    return make_unique<FunctionAST>(move(Proto), move(E));
  return nullptr;
}

unique_ptr<FunctionAST> Parser::ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = make_unique<PrototypeAST>("__anon_expr", vector<string>());
    return make_unique<FunctionAST>(move(Proto), move(E));
  }
  return nullptr;
}

unique_ptr<PrototypeAST> Parser::ParseExtern() {
  getNextToken(); // eat extern.
  return ParsePrototype();
}
