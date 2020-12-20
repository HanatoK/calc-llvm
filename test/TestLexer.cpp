#include "Lexer.h"

#include <iostream>
#include <string>

void testLexer(const std::string& str) {
  Lexer l;
  l.appendString(str);
  auto result = l.getToken();
  do {
    switch (std::get<0>(result)) {
      case Token::Eof: {
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::LeftParenthesis: {
        std::cout << "Left parenthesis: ";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::RightParenthesis: {
        std::cout << "Right parenthesis: ";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::Identifier: {
        std::cout << "Identifier: ";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::Number: {
        std::cout << "Number: ";
        std::cout << std::get<double>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::Operator: {
        std::cout << "Operator: ";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::Comma: {
        std::cout << "Comma:";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      default: {
        std::cout << "Other token: " << int(std::get<0>(result)) << std::endl;
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
    }
    result = l.getToken();
  } while (std::get<0>(result) != Token::Eof);
}

int main() {
  testLexer(" def f(a) -1+2*-3e-1*-6+a  ;");
  return 0;
}
