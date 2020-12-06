#include "Parser.h"

#include <exception>
#include <vector>

using std::move;
using std::unique_ptr;
using std::string;

#define DEBUG_PARSER

Parser::Parser() {
  SetupPrecedence();
}

Parser::Parser(const string& Str) {
  SetupPrecedence();
  SetupInput(Str);
}

Parser::Parser(const Parser& p) {
  this->SetupPrecedence();
  this->SetupInput(p.getInputString());
}

void Parser::SetupInput(const string& Str) {
  mInputString = Str;
  mInputStream.str(Str);
}

string Parser::getInputString() const {
  return mInputString;
}

void Parser::SetupPrecedence() {
  mBinaryOpPrecedence["+"] = 100;
  mBinaryOpPrecedence["-"] = 100;
  mBinaryOpPrecedence["*"] = 200;
  mBinaryOpPrecedence["/"] = 200;
  mBinaryOpPrecedence["^"] = 300;
  mUnaryOpPrecedence["+"] = 250;
  mUnaryOpPrecedence["-"] = 250;
  mRightAssociative["+"] = false;
  mRightAssociative["-"] = false;
  mRightAssociative["*"] = false;
  mRightAssociative["/"] = false;
  mRightAssociative["^"] = true;
}

int Parser::GetBinaryPrecedence(const string& Op) const {
  auto FindRes = mBinaryOpPrecedence.find(Op);
  if (FindRes != mBinaryOpPrecedence.end()) {
    return FindRes->second;
  } else {
    return -1;
  }
}

int Parser::GetUnaryPrecedence(const string& Op) const {
  auto FindRes = mUnaryOpPrecedence.find(Op);
  if (FindRes != mUnaryOpPrecedence.end()) {
    return FindRes->second;
  } else {
    return -1;
  }
}

bool Parser::IsRightAssociative(const string& Op) const {
  auto FindRes = mRightAssociative.find(Op);
  if (FindRes != mRightAssociative.end()) {
    return FindRes->second;
  } else {
    return false;
  }
}

tuple<Token, variant<string, double>> Parser::getNextToken() {
  return mCurrentToken = mLexer.getToken(mInputStream);
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
    case Token::Unknown: cout << "Unknown: " << get<string>(V); break;
  }
  cout << endl;
}

unique_ptr<ExprAST> Parser::ParseNumberExpr() {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseNumberExpr()\n";
  PrintCurrentToken();
#endif
  auto Result = make_unique<NumberExprAst>(std::get<double>(std::get<1>(mCurrentToken)));
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
        return ParseUnaryOpRHS(0);
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

unique_ptr<ExprAST> Parser::ParseUnaryOpRHS(int ExprPrec) {
  // TODO: I do not totally solve the problem here!
  // TODO: check the power operator ^
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseUnaryOpRHS()\n";
  PrintCurrentToken();
#endif
  using std::get;
  unique_ptr<ExprAST> RHS;
  const string Op = get<string>(get<1>(mCurrentToken));
  const int TokPrec = GetUnaryPrecedence(Op);
  if (TokPrec < ExprPrec)
    return RHS;
  getNextToken();
  auto LHS = make_unique<NumberExprAst>(0.0);
  RHS = ParsePrimary();
  if (!RHS)
    return nullptr;
  int NextPrec = GetBinaryPrecedence(get<string>(get<1>(mCurrentToken)));
  if (TokPrec < NextPrec) {
    // take the current RHS as the LHS of the new operator,
    // and parse the binary expression for the new one again
    RHS = ParseBinOpRHS(TokPrec + 1, move(RHS));
    if (!RHS)
      return nullptr;
  }
  RHS = make_unique<BinaryExprAST>(Op, move(LHS), move(RHS));
  return RHS;
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

unique_ptr<FunctionAST> Parser::ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = make_unique<PrototypeAST>("", vector<string>());
    return make_unique<FunctionAST>(move(Proto), move(E));
  }
  return nullptr;
}
