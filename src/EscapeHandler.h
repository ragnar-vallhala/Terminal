#ifndef ESCAPE_HANDLER_H
#define ESCAPE_HANDLER_H
#include <string>

#define SCR_CLEAR_SIGNAL 0

int handle_escape_sequence(std::string s, void (*callback)(int signal));
int __handle_csi_escapes(std::string s, void (*callback)(int signal));
#endif // !ESCAPE_HANDLER_H
