#ifndef TERMINAL_H
#define TERMINAL_H

#include "GLFW/glfw3.h"
#include "PTYHandler.h"

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
  PTYHandler *get_pty();
  ~Terminal();
  float get_font_size();
  void set_font_size(float size);
  float get_scroll_pos();
  int get_character_width();
  int get_character_height();
  int get_window_height_in_characters();
  int get_window_width_in_characters();
  int get_cursor_x();
  int get_cursor_y();
  void set_cursor_x(int x);
  void set_cursor_y(int y);

private:
  Terminal();
  bool running = true;
  static Terminal *instance;
  int windowWidth = 800;
  int windowHeight = 600;
  GLFWwindow *window = nullptr;
  PTYHandler *pty = nullptr;
  float fontSize = 1.0f;
  float scrollPos = 0.0f;
  int characterWidth = 0;
  int characterHeight = 0;
  int windowWidthInCharacters = 0;
  int windowHeightInCharacters = 0;
  int cursorX = 0;
  int cursorY = 0;
};

#endif // !TERMINAL_H
