#ifndef WATCHER_HPP
#define WATCHER_HPP

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <atomic>
#include <sys/inotify.h>

// Callback function type: receives (filepath, inotify_mask)
using EventCallback = std::function<void(const std::string& filepath, uint32_t mask)>;

class Watcher {
public:
    Watcher();
    ~Watcher();

    // Add a directory (and optionally its subdirectories) to inotify watch list
    bool addWatch(const std::string& directoryPath, bool recursive = true);

    // Register the callback function to handle events
    void setCallback(EventCallback callback);

    // Main event listener loop (runs until runningFlag becomes false)
    void startWatching(const std::atomic<bool>& runningFlag);

    // Stop listening and close inotify file descriptor
    void stopWatching();

private:
    int inotifyFd_;
    std::unordered_map<int, std::string> watchDescriptors_; // wd -> path
    std::unordered_map<std::string, int> pathDescriptors_;  // path -> wd
    EventCallback eventCallback_;

    void processEvents(char* buffer, ssize_t numBytes);
};

#endif // WATCHER_HPP
