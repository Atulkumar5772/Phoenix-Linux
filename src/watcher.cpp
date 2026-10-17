#include "watcher.hpp"
#include "logger.hpp"
#include <unistd.h>
#include <poll.h>
#include <filesystem>
#include <cstring>
#include <iostream>

namespace fs = std::filesystem;

Watcher::Watcher() : inotifyFd_(-1) {
    inotifyFd_ = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (inotifyFd_ < 0) {
        Logger::critical("Failed to initialize Linux inotify kernel subsystem!");
    }
}

Watcher::~Watcher() {
    stopWatching();
}

void Watcher::stopWatching() {
    if (inotifyFd_ >= 0) {
        for (const auto& [wd, path] : watchDescriptors_) {
            inotify_rm_watch(inotifyFd_, wd);
        }
        watchDescriptors_.clear();
        pathDescriptors_.clear();
        close(inotifyFd_);
        inotifyFd_ = -1;
    }
}

bool Watcher::addWatch(const std::string& directoryPath, bool recursive) {
    if (inotifyFd_ < 0 || !fs::exists(directoryPath)) {
        return false;
    }

    uint32_t mask = IN_MODIFY | IN_DELETE | IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE | IN_DELETE_SELF;

    auto attachWatch = [this, mask](const std::string& path) {
        if (pathDescriptors_.find(path) != pathDescriptors_.end()) {
            return;
        }
        int wd = inotify_add_watch(inotifyFd_, path.c_str(), mask);
        if (wd >= 0) {
            watchDescriptors_[wd] = path;
            pathDescriptors_[path] = wd;
            Logger::info("  [Kernel Watch Attached] " + path + " (wd: " + std::to_string(wd) + ")");
        } else {
            Logger::warn("Could not attach watch to: " + path);
        }
    };

    attachWatch(directoryPath);

    if (recursive) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
                if (entry.is_directory()) {
                    attachWatch(entry.path().string());
                }
            }
        } catch (const std::exception& e) {
            Logger::warn("Recursive watch scan error: " + std::string(e.what()));
        }
    }

    return true;
}

void Watcher::setCallback(EventCallback callback) {
    eventCallback_ = callback;
}

void Watcher::processEvents(char* buffer, ssize_t numBytes) {
    ssize_t i = 0;
    while (i < numBytes) {
        struct inotify_event* event = reinterpret_cast<struct inotify_event*>(&buffer[i]);

        if (event->len > 0) {
            auto it = watchDescriptors_.find(event->wd);
            if (it != watchDescriptors_.end()) {
                std::string dirPath = it->second;
                std::string fullPath = (fs::path(dirPath) / event->name).string();

                if ((event->mask & IN_ISDIR) && (event->mask & IN_CREATE)) {
                    addWatch(fullPath, true);
                }

                if (eventCallback_) {
                    eventCallback_(fullPath, event->mask);
                }
            }
        }

        i += sizeof(struct inotify_event) + event->len;
    }
}

void Watcher::startWatching(const std::atomic<bool>& runningFlag) {
    if (inotifyFd_ < 0) {
        Logger::critical("Cannot start watching: inotify file descriptor is invalid.");
        return;
    }

    Logger::success("Kernel Inotify Event Loop active. Listening for real-time file tampering...");

    const size_t BUF_LEN = 1024 * (sizeof(struct inotify_event) + 16);
    char buffer[BUF_LEN];

    struct pollfd pfd;
    pfd.fd = inotifyFd_;
    pfd.events = POLLIN;

    while (runningFlag.load()) {
        int pollResult = poll(&pfd, 1, 200);

        if (pollResult > 0) {
            if (pfd.revents & POLLIN) {
                ssize_t numBytes = read(inotifyFd_, buffer, BUF_LEN);
                if (numBytes > 0) {
                    processEvents(buffer, numBytes);
                }
            }
        } else if (pollResult < 0 && errno != EINTR) {
            Logger::critical("Poll error in watcher loop!");
            break;
        }
    }

    Logger::info("Exiting Inotify Event Loop cleanly.");
}
