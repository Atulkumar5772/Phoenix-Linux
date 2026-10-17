#include "vault.hpp"
#include "logger.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <unistd.h>

Vault::Vault(const std::string& vaultStorageDir) : vaultDir_(vaultStorageDir) {
    if (!fs::exists(vaultDir_)) {
        fs::create_directories(vaultDir_);
    }
}

std::string Vault::calculateSHA256(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) {
        EVP_DigestUpdate(ctx, buffer, file.gcount());
    }
    if (file.gcount() > 0) {
        EVP_DigestUpdate(ctx, buffer, file.gcount());
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;
    EVP_DigestFinal_ex(ctx, hash, &lengthOfHash);
    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool Vault::captureMetadata(const std::string& filepath, FileMetadata& outMeta) {
    struct stat st;
    if (stat(filepath.c_str(), &st) != 0) {
        return false;
    }

    outMeta.path = filepath;
    outMeta.permissions = st.st_mode & 07777; // Capture permission bits + SUID/SGID
    outMeta.uid = st.st_uid;
    outMeta.gid = st.st_gid;
    outMeta.sha256 = calculateSHA256(filepath);
    
    try {
        outMeta.lastModified = fs::last_write_time(filepath);
    } catch (...) {
        // Fallback if filesystem call fails
    }
    return true;
}

std::string Vault::getVaultBackupPath(const std::string& originalPath) const {
    std::string cleanPath = originalPath;
    if (cleanPath.rfind("./", 0) == 0) {
        cleanPath = cleanPath.substr(2);
    }
    if (!cleanPath.empty() && cleanPath.front() == '/') {
        cleanPath = cleanPath.substr(1);
    }
    return (fs::path(vaultDir_) / cleanPath).string();
}

bool Vault::createSnapshot(const std::string& sourceFile) {
    if (!fs::exists(sourceFile)) return false;

    std::string backupTarget = getVaultBackupPath(sourceFile);
    fs::path targetDir = fs::path(backupTarget).parent_path();

    try {
        if (!fs::exists(targetDir)) {
            fs::create_directories(targetDir);
        }
        fs::copy_file(sourceFile, backupTarget, fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        Logger::critical("Failed to create snapshot for " + sourceFile + ": " + e.what());
        return false;
    }
}

bool Vault::initializeBaseline(const std::string& targetDir) {
    if (!fs::exists(targetDir)) {
        Logger::critical("Target directory does not exist: " + targetDir);
        return false;
    }

    Logger::info("Creating Security Baseline & Snapshots for: " + targetDir);
    size_t count = 0;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(targetDir)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();
                FileMetadata meta;
                if (captureMetadata(filePath, meta)) {
                    baselineDatabase_[filePath] = meta;
                    createSnapshot(filePath);
                    count++;
                    Logger::info("  [Baseline Recorded] " + filePath + " | Hash: " + meta.sha256.substr(0, 8) + "...");
                }
            }
        }
    } catch (const std::exception& e) {
        Logger::critical("Baseline scan error: " + std::string(e.what()));
        return false;
    }

    Logger::success("Baseline initialized! " + std::to_string(count) + " files securely registered.");
    return true;
}

bool Vault::verifyIntegrity(const std::string& filepath, bool& outHashMismatch, bool& outPermMismatch) {
    outHashMismatch = false;
    outPermMismatch = false;

    auto it = baselineDatabase_.find(filepath);
    if (it == baselineDatabase_.end()) {
        return false; // Not in baseline
    }

    FileMetadata currentMeta;
    if (!captureMetadata(filepath, currentMeta)) {
        // File may have been deleted!
        outHashMismatch = true;
        return true;
    }

    if (currentMeta.sha256 != it->second.sha256) {
        outHashMismatch = true;
    }
    if (currentMeta.permissions != it->second.permissions) {
        outPermMismatch = true;
    }

    return true;
}

bool Vault::getBaseline(const std::string& filepath, FileMetadata& outMeta) const {
    auto it = baselineDatabase_.find(filepath);
    if (it != baselineDatabase_.end()) {
        outMeta = it->second;
        return true;
    }
    return false;
}
