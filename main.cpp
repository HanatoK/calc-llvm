#include "Lexer.h"

int main() {
  std::string str{"  sin(2.2,1.3) + (x + 1.5) *2.6  "};
//   str = " + ";
//   str = " 5e-2 / 1e3 ";
  Lexer lexer;
  lexer.getToken(str);
//   lexer.printState(token);
  return 0;
}
