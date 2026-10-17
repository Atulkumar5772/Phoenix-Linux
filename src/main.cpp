#include "logger.hpp"
#include "vault.hpp"
#include "watcher.hpp"
#include "rollback.hpp"
#include "config.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running(true);

void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        Logger::warn("Shutdown signal received! Gracefully stopping Phoenix-Linux...");
        g_running = false;
    }
}

void printHelp() {
    Logger::banner();
    std::cout << "Usage: ./bin/phoenix-linux [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -c, --config <file>    Path to configuration file (default: config/phoenix.conf)\n";
    std::cout << "  -d, --dir <path>       Direct single directory to monitor\n";
    std::cout << "  -h, --help             Show this help menu\n";
    std::cout << "  -v, --version          Show version information\n\n";
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::string configFile = "config/phoenix.conf";
    std::string cliOverrideDir = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "Phoenix-Linux v1.0.0 (Linux Kernel inotify & Self-Healing Daemon)\n";
            return 0;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) configFile = argv[++i];
        } else if (arg == "-d" || arg == "--dir") {
            if (i + 1 < argc) cliOverrideDir = argv[++i];
        }
    }

    Logger::banner();
    Logger::info("Starting Phoenix-Linux Autonomous Security Daemon...");

    // 1. Load Configuration
    AppConfig config;
    ConfigParser::loadConfig(configFile, config);
    if (!cliOverrideDir.empty()) {
        config.monitoredDirectories.clear();
        config.monitoredDirectories.push_back(cliOverrideDir);
    }
    if (config.monitoredDirectories.empty()) {
        config.monitoredDirectories.push_back("/tmp/test_protected");
    }

    // 2. Initialize Vault & Baseline across all directories
    Vault vault(config.vaultDir);
    for (const auto& dir : config.monitoredDirectories) {
        vault.initializeBaseline(dir);
    }

    // 3. Initialize Rollback & Watcher Engines
    RollbackEngine rollback(vault);
    Watcher watcher;

    for (const auto& dir : config.monitoredDirectories) {
        watcher.addWatch(dir, true);
    }

    // 4. Set Event Callback Handler
    watcher.setCallback([&rollback, &config](const std::string& filepath, uint32_t mask) {
        if (config.autoRollback) {
            rollback.autoHeal(filepath, mask);
        }
    });

    // 5. Start Event Loop
    watcher.startWatching(g_running);

    // 6. Print Security Audit & Attack Summary Report on Shutdown
    rollback.printAuditSummary();
    rollback.saveAuditReport("phoenix-audit.log");

    Logger::success("Phoenix-Linux shut down safely. Audit record saved to phoenix-audit.log.");
    return 0;
}
