// This is a simple logger system
// The code is in C style it's not intended to be part of release version
// Only for debugging

#include <string>

#ifndef LOGGER_H
#define LOGGER_H

#define ERR 1
#define NORM 0

#define RED_COLOR     "\033[31m"
#define GREEN_COLOR   "\033[32m"
#define BOLD          "\033[1m"
#define RESET         "\033[0m"


void pretty_log(std::string info, std::string message, int category = NORM);

#endif //!LOGGER_H
