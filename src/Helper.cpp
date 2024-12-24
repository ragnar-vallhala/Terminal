#include "Helper.h"
#include <string>
std::string get_bytes(const char *str, int size) {
  std::string res;
  for (int i{}; i < size; i++) {
    res += std::to_string((int)str[i]) + std::string(" ");
    if ((i + 1) % 8 == 0)
      res += '\n';
  }
  return res;
}

// PTY_Payload struct
PTY_Payload::PTY_Payload(std::string payload, int payload_type)
    : data(payload), type(payload_type) {}

// PTY_Payload_list struct
PTY_Payload_List::PTY_Payload_List(PTY_Payload *node)
    : curr(node), next(nullptr) {}
