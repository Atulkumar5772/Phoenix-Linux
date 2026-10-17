#include "logger.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

// ANSI Color Codes
#define RESET   "\033[0m"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define CYAN    "\033[1;36m"
#define BOLD    "\033[1m"

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Logger::log(LogLevel level, const std::string& message) {
    std::string timestamp = getCurrentTime();
    
    switch (level) {
        case LogLevel::INFO:
            std::cout << BLUE << "[" << timestamp << "] [INFO] " << RESET << message << "\n";
            break;
        case LogLevel::WARN:
            std::cout << YELLOW << "[" << timestamp << "] [WARN] " << RESET << message << "\n";
            break;
        case LogLevel::CRITICAL:
            std::cout << RED << "[" << timestamp << "] [CRITICAL] 🚨 " << RESET << BOLD << message << RESET << "\n";
            break;
        case LogLevel::RESTORE:
            std::cout << CYAN << "[" << timestamp << "] [RESTORE] 🔥 " << RESET << message << "\n";
            break;
        case LogLevel::SUCCESS:
            std::cout << GREEN << "[" << timestamp << "] [SUCCESS] ✅ " << RESET << message << "\n";
            break;
    }
}

void Logger::info(const std::string& msg)     { log(LogLevel::INFO, msg); }
void Logger::warn(const std::string& msg)     { log(LogLevel::WARN, msg); }
void Logger::critical(const std::string& msg) { log(LogLevel::CRITICAL, msg); }
void Logger::restore(const std::string& msg)  { log(LogLevel::RESTORE, msg); }
void Logger::success(const std::string& msg)  { log(LogLevel::SUCCESS, msg); }

void Logger::banner() {
    std::cout << CYAN << R"(
  ____  _                      _         _     _                  
 |  _ \| |__   ___   ___ _ __ (_)_  __  | |   (_)_ __  _   ___  __
 | |_) | '_ \ / _ \ / _ \ '_ \| \ \/ /  | |   | | '_ \| | | \ \/ /
 |  __/| | | | (_) |  __/ | | | |>  <   | |___| | | | | |_| |>  < 
 |_|   |_| |_|\___/ \___|_| |_|_/_/\_\  |_____|_|_| |_|\__,_/_/\_\
)" << RESET;
    std::cout << BOLD << " [ Phoenix-Linux: Kernel inotify Integrity & Self-Healing Daemon ]\n" << RESET;
    std::cout << "-------------------------------------------------------------------\n";
}
