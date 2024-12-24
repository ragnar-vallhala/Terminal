#include "PTYHandler.h"
#include "Helper.h"
#include "Logger.h"
#include <cstddef>
#include <cstdint>
#include <pty.h>
#include <regex>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

std::pair<std::vector<std::string>, std::string>
__filter_escape_sequence(const char *arr);

PTYHandler *PTYHandler::instance = nullptr;
static bool input_signal = true;

PTYHandler *PTYHandler::get_instance() {
  if (instance == nullptr)
    instance = new PTYHandler();
  return instance;
}

PTYHandler::~PTYHandler() {
  close(master_fd);
  close(slave_fd);
  exit();
  pretty_log("PTY", "PTYHandler send exit signal.");
  writer.join();
  reader.join();
}
PTYHandler::PTYHandler() {
  pretty_log("PTY", "PTYHandler constructor called.");
  if (openpty(&master_fd, &slave_fd, slave_name, nullptr, nullptr) == -1) {
    throw "Can't open PTY";
  }
  pretty_log("PTY", std::string("Created PTY name ") + slave_name);
  init();
}
void PTYHandler::set_output_callback(void (*callback)(const char *output,
                                                      uint64_t size)) {
  output_callback = callback;
}
void PTYHandler::init() {
  pretty_log("PTY",
             std::string("Fork started for master and slave side of pty."));
  pid_t pid = fork();
  if (pid == -1) {
    pretty_log("PTY",
               std::string("Fork failed for master and slave side of pty."),
               ERR);
    throw "Can't Fork for Shell to start";
  }
  if (pid == 0) {
    // Slave thread running the shell
    __init_slave();
  } else {
    // master thread running IO
    __init_master();
  }
}
void PTYHandler::send(std::string input) {
  inputBuffer = input;
  input_signal = false;
  pretty_log("PTY", std::string("PTY recieved input data stream ") + '<' +
                        input + '>');
}
void PTYHandler::__init_master() {
  pretty_log("PTY", "Master PTY init started.");
  close(slave_fd);

  writer = std::thread(&PTYHandler::__writer_thread, this);

  reader = std::thread(&PTYHandler::__reader_thread, this);
}

void PTYHandler::__init_slave() {
  pretty_log("PTY", std::string("Slave PTY init started."));
  if (setsid() == -1) {
    pretty_log("PTY", std::string("Slave PTY SID set failed."), ERR);
    throw "Can't set sid for shell";
  }
  // Set the slave PTY as the controlling terminal
  if (ioctl(slave_fd, TIOCSCTTY, nullptr) == -1) {
    pretty_log("PTY", std::string("Slave PTY IOCTL transfer failed."), ERR);
    throw "Can't transfer IO control to shell";
  }
  close(master_fd);
  // Redirect standard input, output, and error to the slave
  pretty_log("PTY", std::string("Bash started on slave side."));
  dup2(slave_fd, STDIN_FILENO);
  dup2(slave_fd, STDOUT_FILENO);
  dup2(slave_fd, STDERR_FILENO);
  close(slave_fd);
  execlp("stdbuf", "stdbuf", "-o0", "bash", nullptr);
  pretty_log("PTY", std::string("Bash startup failed."), ERR);
}

void PTYHandler::exit() {
  pretty_log("PTY", "Exit called.", ERR);
  running = false;
}

void PTYHandler::__writer_thread() {
  pretty_log("PTY", "Master PTY fork for writer started.");
  // Writer thread
  while (running) {
    if (!input_signal) {
      pretty_log("PTY", "Writer got input.");
      size_t bytes_written =
          write(master_fd, inputBuffer.c_str(), inputBuffer.length());
      if (bytes_written == inputBuffer.length()) {
        pretty_log("PTY", "Writer input passed to shell.");
      } else if (bytes_written <= 0) {
        pretty_log("PTY", "Writer input passing to shell fragmented.", ERR);
      } else {
        pretty_log("PTY", "Writer input passing to shell failed.", ERR);
      }
      input_signal = true;
    }
  }
}
void PTYHandler::__reader_thread() {
  // Reader Thread
  pretty_log("PTY", "Master PTY fork for reader started.");
  char buffer[256];
  ssize_t bytes_read;
  bytes_read = read(master_fd, buffer, sizeof(buffer) - 1);
  while (running) {
    if (output_callback != nullptr && bytes_read > 0) {
      buffer[bytes_read] = '\0';
      auto res = __filter_escape_sequence(buffer);
      output_callback(res.second.c_str(), res.second.size());
      pretty_log("PTY", "Reader tranfered data to callback.");
      pretty_log("PTY", std::string("Reader got ") +
                            std::to_string(res.first.size()) +
                            std::string(" escape sequences."));
    }
    bytes_read = read(master_fd, buffer, sizeof(buffer) - 1);
  }
}
std::string escape_regex_special_chars(const std::string &str) {
  return std::regex_replace(str, std::regex("([\\^$.|?*+()\\[\\]{}\\\\])"),
                            "\\$1");
}

std::pair<std::vector<std::string>, std::string>
__filter_escape_sequence(const char *arr) {
  std::string input(arr);
  std::vector<std::string> controlSequences;
  std::regex ansi_escape("\x1b\\[[\\?\\d;]*[a-zA-Z]");

  // Iterator for matching all occurrences
  auto begin = std::sregex_iterator(input.begin(), input.end(), ansi_escape);
  auto end = std::sregex_iterator();

  // To build the filtered string
  std::string filteredString = input;

  // Iterate through all matches and store them in the vector
  for (auto it = begin; it != end; ++it) {
    controlSequences.push_back(it->str()); // Add each match to the vector
    // Remove the matched escape sequence from the filtered string
    filteredString = std::regex_replace(
        filteredString, std::regex(escape_regex_special_chars(it->str())), "");
  }
  /* if (controlSequences.size() == 0) { */
  /*   pretty_log("PAR", get_bytes(input.c_str(), input.size())); */
  /* } */

  return std::make_pair(controlSequences, filteredString);
}

