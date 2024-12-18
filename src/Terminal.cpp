#include "Terminal.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <stdio.h>
#include <string>
#include <unordered_map>

static std::string inputBuffer;
static std::string outputBuffer;
static bool __scroll_down = true;

static void glfw_error_callback(int error, const char *description);
static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods);

Terminal *Terminal::instance = nullptr;

Terminal::Terminal() {}
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
uint32_t Terminal::get_height() { return windowHeight; }
uint32_t Terminal::get_width() { return windowWidth; }
GLFWwindow *Terminal::get_window() { return window; }
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
  glfwSetKeyCallback(window, key_callback);
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

void Terminal::render() {

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
    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
    ImGui::Text("%s", inputBuffer.c_str());
    ImGui::PopTextWrapPos();
    if (__scroll_down) {
      ImGui::SetScrollHereY(1.0f);
      __scroll_down = false;
    }
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }
}

void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

/*
 * Keyboard handling section below
 */

void __handle_key_up(int key, int mods);
void __handle_key_down(int key, int mods);
void __handle_key_held_down(int key, int mods);

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (action == GLFW_PRESS) {
    __handle_key_down(key, mods);
  } else if (action == GLFW_RELEASE) {
    __handle_key_up(key, mods);
  } else if (action == GLFW_REPEAT) {
    __handle_key_held_down(key, mods);
  }
}

bool __is_alpha(int key) {
  return (key >= (int)'A' && key <= (int)'Z') || (key >= 'a' && key <= 'z');
}

static std::unordered_map<int, char> __valid_symbols = {
    {39, '\''}, {44, ','}, {45, '-'},  {46, '.'}, {47, '/'}, {59, ';'},
    {61, '='},  {91, '['}, {92, '\\'}, {93, ']'}, {96, '`'}};

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

  } else {
  }
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

void __handle_key_down(int key, int mods) {
  // Internal function for the key_callback
  if (__is_alpha(key)) {
    // TODO: Just adding to the buffer. Fix later
    inputBuffer += __get_adapted_alpha(key, mods);
  }
  if (__is_symbol(key)) {
    inputBuffer += __get_adapted_symbol(key, mods);
  }
  if (key == GLFW_KEY_BACKSPACE && inputBuffer.length())
    inputBuffer.pop_back();
  if (key == GLFW_KEY_SPACE)
    inputBuffer += ' ';
  if (key == GLFW_KEY_ENTER)
    inputBuffer += '\n';
}

void __handle_key_up(int key, int mods) {
  // Internal function for the key_callback
}
void __handle_key_held_down(int key, int mods) { __handle_key_down(key, mods); }
/*
 * Keyboard handling section end
 */
