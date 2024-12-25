#ifndef HELPER_H
#define HELPER_H
#include <string>

// Helper functions
std::string get_bytes(const char *str, int size);

// Helper structures

#define PAYLOAD_INS 0 // instruction payload
#define PAYLOAD_STR 1 // string payload

typedef struct PTY_Payload {
  std::string data;
  int type;
  PTY_Payload(std::string paylaod, int payload_type);
} PTY_Payload;

typedef struct PTY_Payload_List {
  PTY_Payload *curr;
  PTY_Payload_List *next;
  PTY_Payload_List(PTY_Payload *node);
} PTY_Payload_List;

PTY_Payload_List *erase_PTY_Payload_List(PTY_Payload_List *node);
#endif
