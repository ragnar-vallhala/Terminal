#include "EscapeHandler.h"
#include <string>

int handle_escape_sequence(std::string s, void (*callback)(int signal)) {
  if (s.find("\x1b[") != std::string::npos)
    return __handle_csi_escapes(s, callback);
  return 0;
}

int __handle_csi_escapes(std::string s, void (*callback)(int signal)) {
  if (s == "\x1b[2J") {
    callback(SCR_CLEAR_SIGNAL);
    return 1;
  }
  return 0;
}
