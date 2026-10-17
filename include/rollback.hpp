#ifndef ROLLBACK_HPP
#define ROLLBACK_HPP

#include "vault.hpp"
#include <string>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <cstdint>

struct SecurityStats {
    uint64_t totalEvents = 0;
    uint64_t contentRollbacks = 0;
    uint64_t permissionRollbacks = 0;
    uint64_t deletionRollbacks = 0;
};

class RollbackEngine {
public:
    explicit RollbackEngine(Vault& vault);

    // Main self-healing function
    bool autoHeal(const std::string& filepath, uint32_t inotifyMask);

    // Anti-Loop Protection methods
    bool isSuppressed(const std::string& filepath);
    void suppressEvent(const std::string& filepath);
    void unsuppressEvent(const std::string& filepath);

    // Audit & Attack Statistics
    SecurityStats getStats() const;
    void printAuditSummary() const;
    void saveAuditReport(const std::string& reportPath = "phoenix-audit.log") const;

private:
    Vault& vault_;
    std::unordered_set<std::string> suppressedFiles_;
    std::mutex suppressMutex_;

    // Attack & Rollback Counters
    std::atomic<uint64_t> totalEvents_{0};
    std::atomic<uint64_t> contentRollbacks_{0};
    std::atomic<uint64_t> permissionRollbacks_{0};
    std::atomic<uint64_t> deletionRollbacks_{0};

    bool restoreContentAndMetadata(const std::string& filepath, const FileMetadata& baseline);
};

#endif // ROLLBACK_HPP
