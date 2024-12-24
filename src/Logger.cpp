// This is a simple logger system
// The code is in C style it's not intended to be part of release version
// Only for debugging

#include "Logger.h"
#include <iostream>
#include <string>

void pretty_log(std::string info, std::string message, int category) {
  if (!category) {
    std::cout << GREEN_COLOR << BOLD << "[ " << info << " ] " << RESET
              << message << std::endl;
  } else {
    std::cerr << RED_COLOR << BOLD << "[ " << info << " ] " << RESET
              << message << std::endl;
  }
}
