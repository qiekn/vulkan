export module greeting;

import std;

export void greet(const std::string& name) {
  std::cout << "Hello, " << name << "!\n";
}
