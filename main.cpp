#include "Lexer.h"

int main() {
  std::string str{"sin(2.2,1.3)"};
//   str = " + ";
//   str = " 5e-2 / 1e3 ";
  Lexer lexer(str);
  int token = lexer.getToken();
//   lexer.printState(token);
  return 0;
}
