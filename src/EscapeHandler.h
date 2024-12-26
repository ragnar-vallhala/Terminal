#ifndef ESCAPE_HANDLER_H
#define ESCAPE_HANDLER_H
#include <string>

// Signal to returns from escape_handlers
#define ESC_NO_HANDLE_SIGNAL -1
#define ESC_SCR_CLEAR_SIGNAL 0

int handle_escape_sequence(std::string s, void (*callback)(int signal));
int __handle_csi_escapes(std::string s, void (*callback)(int signal));
#endif // !ESCAPE_HANDLER_H
