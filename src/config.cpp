#include "config.hpp"
#include "logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

static inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool ConfigParser::loadConfig(const std::string& configPath, AppConfig& outConfig) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        Logger::warn("Config file not found at " + configPath + ". Using defaults.");
        return false;
    }

    std::string line;
    std::string currentSection = "";

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }

        if (currentSection == "general") {
            auto delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = trim(line.substr(0, delimiterPos));
                std::string value = trim(line.substr(delimiterPos + 1));
                if (key == "vault_dir") {
                    outConfig.vaultDir = value;
                } else if (key == "auto_rollback") {
                    outConfig.autoRollback = (value == "true" || value == "1");
                }
            }
        } else if (currentSection == "directories") {
            outConfig.monitoredDirectories.push_back(line);
        }
    }

    return true;
}
