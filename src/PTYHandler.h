#ifndef PTYHANDLER_H
#define PTYHANDLER_H
#include "Helper.h"
#include <cstdint>
#include <string>
#include <thread>

class PTYHandler {
public:
  static PTYHandler *get_instance();
  ~PTYHandler();
  void init();
  void send(std::string input);
  void set_output_callback(void (*callback)(PTY_Payload_List *payloadList));
  void exit();

private:
  PTYHandler();
  void __init_slave();
  void __init_master();
  static PTYHandler *instance;
  int master_fd;
  int slave_fd;
  char slave_name[256];
  std::string inputBuffer;
  void (*output_callback)(PTY_Payload_List *payloadList) = nullptr;
  bool running = true;
  void __reader_thread();
  void __writer_thread();
  std::thread reader;
  std::thread writer;
};
#endif //! PTYHANDLER_H
