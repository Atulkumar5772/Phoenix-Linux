#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>

struct AppConfig {
    std::string vaultDir = ".vault";
    bool autoRollback = true;
    std::vector<std::string> monitoredDirectories;
};

class ConfigParser {
public:
    static bool loadConfig(const std::string& configPath, AppConfig& outConfig);
};

#endif // CONFIG_HPP
