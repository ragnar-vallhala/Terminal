#ifndef TERMINAL_H
#define TERMINAL_H

#include "GLFW/glfw3.h"

class Terminal {
public:
  static Terminal *get_instance();
  bool init();
  void render();
  void stop();
  bool is_running();

  uint32_t get_width();
  uint32_t get_height();
  void set_window_dim(uint32_t width = -1, uint32_t height = -1);
  GLFWwindow *get_window();

  ~Terminal();

private:
  Terminal();
  bool running = true;
  static Terminal *instance;
  int windowWidth = 800;
  int windowHeight = 600;
  GLFWwindow *window = nullptr;
};

#endif // !TERMINAL_H
