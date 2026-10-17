#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>

enum class LogLevel {
    INFO,
    WARN,
    CRITICAL,
    RESTORE,
    SUCCESS
};

class Logger {
public:
    static void log(LogLevel level, const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void critical(const std::string& message);
    static void restore(const std::string& message);
    static void success(const std::string& message);
    static void banner();

private:
    static std::string getCurrentTime();
};

#endif // LOGGER_HPP
