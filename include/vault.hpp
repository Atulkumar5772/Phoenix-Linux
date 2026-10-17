#ifndef VAULT_HPP
#define VAULT_HPP

#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include <filesystem>

namespace fs = std::filesystem;

// Structure to hold a file's security baseline
struct FileMetadata {
    std::string path;
    std::string sha256;
    mode_t permissions; // e.g., 0644, 0755
    uid_t uid;          // User Owner ID
    gid_t gid;          // Group Owner ID
    fs::file_time_type lastModified;
};

class Vault {
public:
    Vault(const std::string& vaultStorageDir = ".vault");

    // Compute SHA-256 hash using OpenSSL
    static std::string calculateSHA256(const std::string& filepath);

    // Capture file stats (permissions, uid, gid, hash)
    static bool captureMetadata(const std::string& filepath, FileMetadata& outMeta);

    // Initialize baseline snapshot for a directory
    bool initializeBaseline(const std::string& targetDir);

    // Create a secure backup copy inside vault
    bool createSnapshot(const std::string& sourceFile);

    // Check if live file matches baseline hash
    bool verifyIntegrity(const std::string& filepath, bool& outHashMismatch, bool& outPermMismatch);

    // Get baseline metadata for rollback
    bool getBaseline(const std::string& filepath, FileMetadata& outMeta) const;

    // Get backup copy path in vault
    std::string getVaultBackupPath(const std::string& originalPath) const;

private:
    std::string vaultDir_;
    std::unordered_map<std::string, FileMetadata> baselineDatabase_;
};

#endif // VAULT_HPP
