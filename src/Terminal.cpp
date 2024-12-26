#include "Terminal.h"
#include "EscapeHandler.h"
#include "Helper.h"
#include "PTYHandler.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/gl.h>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>
#define GL_SILENCE_DEPRECATION
#include "Logger.h"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <filesystem>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

// Config definition to be ported in separate config manager
#define FONT_STEP 0.01
#define FONT_NAME "Nerd"
#define CURSOR "|"
// Global helpers
std::vector<std::string> __find_system_fonts(const std::string &font_name);

static std::string inputBuffer;
/* static std::string outputBuffer; */
static PTY_Payload_List *ptyPayload = nullptr;
static PTY_Payload_List *ptyPayloadTail = nullptr;
static bool __scroll_down = true;

static void glfw_error_callback(int error, const char *description);
static void glfw_key_callback(GLFWwindow *window, int key, int scancode,
                              int action, int mods);
static void glfw_window_resize_callback(GLFWwindow *window, int width,
                                        int height);
static void pty_handler_callback(PTY_Payload_List *payloadList);
static void escape_sequence_handler_callback(int signal);

static void __render_text_to_screen(PTY_Payload_List *payload);
Terminal *Terminal::instance = nullptr;

Terminal::Terminal() {
  pty = PTYHandler::get_instance();
  if (pty == nullptr)
    exit(1);
  // callback registration for output from the pty
  pty->set_output_callback(pty_handler_callback);
  if (!init())
    exit(1);
}
Terminal::~Terminal() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  if (instance)
    delete instance;
  instance = nullptr;
}
Terminal *Terminal::get_instance() {
  if (instance == nullptr)
    instance = new Terminal();
  return instance;
}

void Terminal::set_window_dim(uint32_t width, uint32_t height) {
  if (width > 0)
    windowWidth = width;
  if (height > 0)
    windowHeight = height;
}
PTYHandler *Terminal::get_pty() { return pty; }
uint32_t Terminal::get_height() { return windowHeight; }
uint32_t Terminal::get_width() { return windowWidth; }
GLFWwindow *Terminal::get_window() { return window; }
float Terminal::get_scroll_pos() { return scrollPos; }
int Terminal::get_character_width() { return characterWidth; }
int Terminal::get_character_height() { return characterHeight; }
int Terminal::get_window_width_in_characters() {
  return windowWidthInCharacters;
}
void Terminal::set_cursor_x(int x) { cursorX = x; }
void Terminal::set_cursor_y(int y) { cursorY = y; }
int Terminal::get_cursor_x() { return cursorX; }
int Terminal::get_cursor_y() { return cursorY; }

int Terminal::get_window_height_in_characters() {
  return windowHeightInCharacters;
}
bool Terminal::init() {

  if (!glfwInit())
    return false;

  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); // Keep window decorations
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  window =
      glfwCreateWindow(windowWidth, windowHeight, "Terminal", nullptr, nullptr);
  if (window == nullptr)
    return false;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Setting Callbacks
  glfwSetErrorCallback(glfw_error_callback);
  glfwSetKeyCallback(window, glfw_key_callback);
  glfwSetFramebufferSizeCallback(window, glfw_window_resize_callback);
  glfwSetInputMode(window, GLFW_LOCK_KEY_MODS,
                   GLFW_TRUE); // Enable caps on detection
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  /*---------- Hard Code Style Region Begin-----------------*/
  // TODO: Load the config from a config.json file
  // Customizing ImGui style
  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 0.0f;        // Round window corners
  style.FrameRounding = 5.0f;         // Round frame corners
  style.ItemSpacing = ImVec2(10, 10); // Spacing between items

  // Font customisation
  std::vector<std::string> fontFile = __find_system_fonts(FONT_NAME);
  if (!fontFile.empty()) {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(fontFile[0].c_str(), 128.0f);
    set_font_size(0.15f);
    pretty_log("FONT", "Font loaded correctly.");
  } else {
    pretty_log("FONT", "Specified font not found.", ERR);
  }
  // Customize colors
  style.Colors[ImGuiCol_WindowBg] =
      ImVec4(0.1f, 0.1f, 0.15f, 1.00f); // Dark background
  style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White text
  style.Colors[ImGuiCol_FrameBg] =
      ImVec4(0.2f, 0.2f, 0.25f, 1.0f); // Frame background
  style.Colors[ImGuiCol_Button] =
      ImVec4(0.3f, 0.5f, 0.8f, 1.00f); // Button color
  style.Colors[ImGuiCol_ButtonHovered] =
      ImVec4(0.4f, 0.6f, 1.0f, 1.00f); // Button hover color
  style.Colors[ImGuiCol_ButtonActive] =
      ImVec4(0.2f, 0.4f, 0.6f, 1.00f); // Button active color

  /*---------- Hard Code Style Region End-----------------*/

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  return true;
}

void Terminal::stop() { running = false; }

bool Terminal::is_running() { return running; }
void Terminal::set_font_size(float size) {
  ImGui::GetIO().FontGlobalScale = size;
  fontSize = size;
}

float Terminal::get_font_size() { return fontSize; }

void Terminal::render() {

  auto start = std::chrono::high_resolution_clock::now();
  // Main loop
  while (running && !glfwWindowShouldClose(window)) {
    glfwPollEvents();
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.00f); // Background color
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("##", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    // ImGUI font dim setters
    if (characterWidth == 0 || characterHeight == 0) {
      characterWidth = ImGui::CalcTextSize("Q").x;
      characterHeight = ImGui::CalcTextSize("Q").y;
      pretty_log("FONT", "Font dimensions loaded");
      pretty_log("FONT", std::string("Font dimension: ") +
                             std::to_string(characterWidth) + 'X' +
                             std::to_string(characterHeight));
    }
    ImVec2 padding = ImGui::GetStyle().WindowPadding;
    windowWidthInCharacters =
        (ImGui::GetWindowWidth() - 3 * padding.x) / characterWidth;
    windowHeightInCharacters =
        (ImGui::GetWindowHeight() - 3 * padding.y) / characterHeight;
    // Spacing
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
    __render_text_to_screen(ptyPayload);

    ImGui::PopTextWrapPos();
    ImGui::PopStyleVar(3);
    if (__scroll_down) {
      ImGui::SetScrollHereY(1.0f);
      __scroll_down = false;
    }
    scrollPos = ImGui::GetScrollY();
    ImGui::End();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    ImGui::Begin("FPS");
    ImGui::Text("%s", std::to_string(1000.0 / duration.count()).c_str());
    ImGui::End();
    start = std::chrono::high_resolution_clock::now();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }
}

void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void glfw_window_resize_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
  Terminal::get_instance()->set_window_dim(width, height);
}

/*
 * Keyboard handling section below
 */

void __handle_key_up(int key, int mods);
void __handle_key_down(int key, int mods);
void __handle_key_held_down(int key, int mods);

void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action,
                       int mods) {
  if (action == GLFW_PRESS) {
    __handle_key_down(key, mods);
  } else if (action == GLFW_RELEASE) {
    __handle_key_up(key, mods);
  } else if (action == GLFW_REPEAT) {
    __handle_key_held_down(key, mods);
  }
}
// Action Key section started
void __handle_enter_key(int key, int mods) {
  if (GLFW_MOD_SHIFT & mods) {
    inputBuffer += '\n';
  } else {
    PTYHandler *pty = Terminal::get_instance()->get_pty();
    if (pty)
      pty->send(inputBuffer + '\n');
    inputBuffer.erase();
  }
}
// Action Key section ended

// Symbol section started
static std::unordered_map<int, char> __valid_symbols = {
    {39, '\''}, {44, ','}, {45, '-'},  {46, '.'}, {47, '/'}, {59, ';'},
    {61, '='},  {91, '['}, {92, '\\'}, {93, ']'}, {96, '`'}};

static std::unordered_map<char, char> __valid_symbols_shift = {
    {'\'', '"'}, {',', '<'}, {'-', '_'},  {'.', '>'}, {'/', '?'}, {';', ':'},
    {'=', '+'},  {'[', '{'}, {'\\', '|'}, {']', '}'}, {'`', '~'}};

bool __is_symbol(int key) {
  for (std::pair<int, char> p : __valid_symbols) {
    if (p.first == key)
      return true;
  }
  return false;
}

char __get_adapted_symbol(int key, int mods) {
  if (!__is_symbol(key))
    throw "Key should be a symbol.";
  if (mods & GLFW_MOD_SHIFT) {
    return __valid_symbols_shift[__valid_symbols[key]];
  } else {
    return __valid_symbols[key];
  }
}
// Symbol section ended

// Number section started
static std::unordered_map<char, char> __valid_number_shift = {
    {'0', ')'}, {'1', '!'}, {'2', '@'}, {'3', '#'}, {'4', '$'},
    {'5', '%'}, {'6', '^'}, {'7', '&'}, {'8', '*'}, {'9', '('}};

bool __is_number(int key) { return key <= (int)'9' && key >= (int)'0'; }

char __get_adapted_number(int key, int mods) {
  if (!__is_number(key))
    throw "Key should be a number.";
  if (mods & GLFW_MOD_SHIFT) {
    return __valid_number_shift[(char)key];
  } else {
    return (char)key;
  }
}
// Number section ended

// Alphabet section start
bool __is_alpha(int key) {
  return (key >= (int)'A' && key <= (int)'Z') || (key >= 'a' && key <= 'z');
}

char __alpha_lower(char c) {
  if (!__is_alpha((int)c))
    throw "Key should be alphabet.";
  if (c <= 'Z')
    return c - 'A' + 'a';
  else
    return c;
}

char __alpha_upper(char c) {
  if (!__is_alpha((int)c))
    throw "Key should be alphabet.";
  if (c >= 'a')
    return c - 'a' + 'A';
  else
    return c;
}

char __get_adapted_alpha(int key, int mods) {
  if (!__is_alpha(key))
    throw "Key should be alphabet.";
  if (mods & GLFW_MOD_CAPS_LOCK) {
    if (mods & GLFW_MOD_SHIFT)
      return __alpha_lower(key);
    else
      return __alpha_upper(key);

  } else {
    if (mods & GLFW_MOD_SHIFT)
      return __alpha_upper(key);
    else
      return __alpha_lower(key);
  }
}
// Alphabet section ended

// Detect internal commands of the emulator

bool __is_internal_command(int key, int mods) {
  if (key == GLFW_KEY_EQUAL && mods & GLFW_MOD_SHIFT && mods & GLFW_MOD_SHIFT) {
    Terminal *term = Terminal::get_instance();
    term->set_font_size(term->get_font_size() + FONT_STEP);
    return true;
  } else if (key == GLFW_KEY_MINUS && mods & GLFW_MOD_SHIFT &&
             mods & GLFW_MOD_SHIFT) {
    Terminal *term = Terminal::get_instance();
    term->set_font_size(term->get_font_size() - FONT_STEP);
    return true;
  }
  return false;
}

// Act for C0 signals
bool __handle_C0_code(int key, int mods) {

  if (key == GLFW_KEY_C && mods & GLFW_MOD_CONTROL) {
    inputBuffer.erase();
    inputBuffer += '\x03';
    __handle_enter_key(key, mods);
    return true;
  }
  return false;
}

// Generic key functions start
void __handle_key_down(int key, int mods) {
  // Internal function for the glfw_key_callback
  __scroll_down = true;
  if (__handle_C0_code(key, mods)) {
    return;
  }
  if (__is_alpha(key)) {
    // TODO: Just adding to the buffer. Fix later
    inputBuffer += __get_adapted_alpha(key, mods);
  }

  if (__is_internal_command(key, mods))
    return;
  if (__is_symbol(key)) {
    inputBuffer += __get_adapted_symbol(key, mods);
  }
  if (__is_number(key)) {
    inputBuffer += __get_adapted_number(key, mods);
  }
  if (key == GLFW_KEY_BACKSPACE && inputBuffer.length())
    inputBuffer.pop_back();
  if (key == GLFW_KEY_SPACE)
    inputBuffer += ' ';
  if (key == GLFW_KEY_ENTER)
    __handle_enter_key(key, mods);
}

void __handle_key_up(int key, int mods) {
  // Internal function for the glfw_key_callback
}
void __handle_key_held_down(int key, int mods) { __handle_key_down(key, mods); }
// Generic key functions end

/*
 * Keyboard handling section end
 */

/*
 *  PTY Handler section start
 * */
void pty_handler_callback(PTY_Payload_List *payloadList) {
  pretty_log("TERM", std::string("PTY Callback recieved"));
  if (!__scroll_down) {
    __scroll_down = true;
  }
  if (ptyPayload == nullptr) {
    ptyPayload = payloadList;
  } else {
    ptyPayloadTail->next = payloadList;
  }
  while (payloadList->next) {
    payloadList = payloadList->next;
  }
  ptyPayloadTail = payloadList;
}
/*
 *  PTY Handler section end
 * */

// Global Font functions
std::vector<std::string> __find_system_fonts(const std::string &font_name) {
  std::vector<std::string> font_paths;

  // List of common directories for fonts
  std::vector<std::string> font_dirs = {
      "/usr/share/fonts", "/usr/local/share/fonts",
      std::string(getenv("HOME")) + "/.fonts"};

  // File extensions for font files
  std::vector<std::string> font_extensions = {".ttf", ".otf"};

  // Scan each font directory
  for (const auto &dir : font_dirs) {
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
      continue; // Skip if directory doesn't exist
    }

    // Recursively search for matching font files
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(dir)) {
      if (entry.is_regular_file()) {
        std::string path = entry.path().string();
        std::string filename = entry.path().filename().string();

        // Check if the file matches the desired font name and extension
        for (const auto &ext : font_extensions) {
          if (filename.find(font_name) != std::string::npos &&
              filename.ends_with(ext)) {
            font_paths.push_back(path);
          }
        }
      }
    }
  }

  return font_paths;
}

void __print_character_to_screen(std::string text) {
  Terminal *terminal = Terminal::get_instance();
  int windowWidth = terminal->get_window_width_in_characters();
  int cursorX = terminal->get_cursor_x();

  for (char ch : text) {
    if (ch == '\n' || cursorX >= windowWidth) {
      ImGui::NewLine(); // Move to a new line
      cursorX = 0;      // Reset cursor position for the new line
    }
    if (ch != '\n') {
      ImGui::SameLine(0, 0);
      ImGui::TextUnformatted(&ch, &ch + 1);
      cursorX++;
    }
    if (ch == '\t')
      cursorX += 3;
  }
  terminal->set_cursor_x(cursorX);
}
// Payload Handler
void __render_text_to_screen(PTY_Payload_List *payload) {
  PTY_Payload_List *curr = payload;
  Terminal::get_instance()->set_cursor_x(0);
  while (curr != nullptr && curr->next != nullptr) {
    if (curr->curr->type == PAYLOAD_INS) {
      if (handle_escape_sequence(curr->curr->data,
                                 escape_sequence_handler_callback) ==
          ESC_SCR_CLEAR_SIGNAL) {
        curr = ptyPayload;
        break;
      }
    } else {
      __print_character_to_screen(curr->curr->data.c_str());
    }
    curr = curr->next;
  }
  if (curr != nullptr) {
    __print_character_to_screen(curr->curr->data);
  }
  __print_character_to_screen(inputBuffer + CURSOR);
}

// EscapeSequenceHandler section
void escape_sequence_handler_callback(int signal) {
  if (signal == ESC_SCR_CLEAR_SIGNAL) {
    ptyPayload = erase_PTY_Payload_List(ptyPayload);
    ptyPayloadTail = ptyPayload;
  }
}
